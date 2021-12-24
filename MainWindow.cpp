
#include <QDate>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <stdlib.h>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "utils.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_protected = false;
    m_type = Type::deb;
    m_architecture = Architecture::all;
    m_priority = Priority::standard;
    m_section = Section::misc;

    m_worker = new QProcess;
    connect(m_worker, SIGNAL(started()), this, SLOT(worker_started()));
    connect(m_worker, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(worker_finished(int,QProcess::ExitStatus)));
    connect(m_worker, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(worker_error(QProcess::ProcessError)));

    setWindowIcon(QIcon(":/icons/logo"));

    ui->statusbar->clearMessage();
    ui->LB_AppVer->setText(qApp->applicationVersion());

    QDate date = QDate::currentDate();
    QString date_str = date.toString("yyyy-MM-dd");
    qDebug().noquote() << "AppVer:" << qApp->applicationVersion();
    qDebug().noquote() << "Date:" << date_str;
    ui->LB_Date->setText(date_str);
}

MainWindow::~MainWindow()
{
    delete ui;

    if (m_worker->state() != QProcess::NotRunning) {
        m_worker->terminate();
        if (m_worker->state() != QProcess::NotRunning) {
            m_worker->kill();
        }
    }
    delete m_worker;
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        qApp->quit();
    }
}

void MainWindow::initTempDir()
{
    char tempname[] = "DEBMAKER.XXXXXX";
    if (m_tempdir.isEmpty()) {
        if (!mkdtemp(tempname)) {
            qWarning().noquote() << "Cannot create a uniquely-named tempdir!";
        }
        m_tempdir = tempname;
    }
    QDir temp(tempname);
    if (!temp.exists()) {
        temp.mkpath("DEBIAN");
    }
    m_debiandir = temp.absoluteFilePath("DEBIAN");
}

bool MainWindow::updateInfo()
{
    // Get all settings from UI
    m_name = ui->LE_Package->text();
    m_version = ui->LE_Version->text();
    m_maintainer = ui->LE_Maintainer->text();
    m_homepage = ui->LE_HomePage->text();
    m_summary = ui->LE_Summary->text();
    m_description = ui->TE_Description->toPlainText();
    m_depends = ui->LE_Depends->text();
    m_predepends = ui->LE_PreDepends->text();
    m_conflicts = ui->LE_Conflicts->text();
    m_replaces = ui->LE_Replaces->text();
    m_provides = ui->LE_Provides->text();
    m_protected = ui->CB_Protected->isChecked();
    m_type = static_cast<Type>(ui->ComboBox_PackageType->currentIndex());
    m_architecture = static_cast<Architecture>(ui->ComboBox_Arch->currentIndex());
    m_priority = static_cast<Priority>(ui->ComboBox_Priority->currentIndex());
    m_section = static_cast<Section>(ui->ComboBox_Section->currentIndex());
    m_contents_preinst = ui->TE_PreInst->toPlainText();
    m_contents_postinst = ui->TE_PostInst->toPlainText();
    m_contents_prerm = ui->TE_PreRM->toPlainText();
    m_contents_postrm = ui->TE_PostRM->toPlainText();

    if (m_name.isEmpty()) {
        ui->statusbar->showMessage(tr("Package name is empty!"));
        return false;
    }

    if (m_version.isEmpty()) {
        ui->statusbar->showMessage(tr("Package version is empty!"));
        return false;
    }

    if (m_architecture == Architecture::invalid) {
        ui->statusbar->showMessage(tr("Invalid architecture!"));
        return false;
    }

    if (m_summary.isEmpty()) {
        ui->statusbar->showMessage(tr("Package summary is empty!"));
        return false;
    }

    if (m_type == Type::invalid) {
        ui->statusbar->showMessage(tr("Invalid package type!"));
        return false;
    }

    // Popup a dialog to save deb file
    QString filename = QString("%1_%2_%3.%4").arg(m_name, m_version, Utils::EnumConvert(m_architecture), Utils::EnumConvert(m_type));
    m_filename = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/" + filename, tr("Debian Package (*.deb *.udeb)"));
    qDebug().noquote() << "Package File:" << m_filename;

    return true;
}

bool MainWindow::genControlFile()
{
    QStringList contents;
    contents << QString("Package: %1").arg(m_name);
    contents << QString("Version: %1").arg(m_version);
    contents << QString("Size: 100");
    if (m_architecture != Architecture::invalid)
        contents << QString("Architecture: %1").arg(Utils::EnumConvert(m_architecture).constData());
    if (m_section != Section::invalid)
        contents << QString("Section: %1").arg(Utils::EnumConvert(m_section).constData());
    if (m_priority != Priority::invalid)
        contents << QString("Priority: %1").arg(Utils::EnumConvert(m_priority).constData());
    if (m_protected)
        contents << QString("Protected: yes");
    if (!m_maintainer.isEmpty())
        contents << QString("Maintainer: %1").arg(m_maintainer);
    if (!m_homepage.isEmpty())
        contents << QString("Homepage: %1").arg(m_homepage);
    if (!m_predepends.isEmpty())
        contents << QString("Pre-Depends: %1").arg(m_predepends);
    if (!m_depends.isEmpty())
        contents << QString("Depends: %1").arg(m_depends);
    if (!m_conflicts.isEmpty())
        contents << QString("Conflicts: %1").arg(m_conflicts);
    if (!m_replaces.isEmpty())
        contents << QString("Replaces: %1").arg(m_replaces);
    if (!m_provides.isEmpty())
        contents << QString("Provides: %1").arg(m_provides);
    if (!m_summary.isEmpty())
        contents << QString("Description: %1").arg(m_summary);
    // TODO: Need format long-description contents (https://manpages.debian.org/testing/dpkg-dev/deb-control.5.en.html#Description:)
    if (!m_description.isEmpty())
        contents << QString(" %1").arg(m_description);

    contents << "";

    QString control_file = Utils::BuildPath({m_debiandir, "control"});
    QFile file(control_file);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(control_file));
        return false;
    }
    qDebug().noquote() << "Write:" << file.write(contents.join("\n").toUtf8()) << "Bytes to" << control_file;
    file.flush();
    file.close();

    return true;
}

bool MainWindow::genScriptFile(const QString &scriptfile, const QString &contents)
{
    if (scriptfile.isEmpty()) {
        qCritical().noquote() << "Error: Empty of script file path!";
        return false;
    }

    if (contents.isEmpty()) {
        qInfo().noquote() << "Info: Empty of script contents!";
        return true;
    }

    QString writeable_contents = contents;
    QFile file(scriptfile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical().noquote() << "Error:" << QString("Open file %1 failed!").arg(scriptfile);
        ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(scriptfile));
        return false;
    }
    if (!writeable_contents.endsWith("\n")) {
        writeable_contents.append('\n');
    }
    qDebug().noquote() << "Write:" << file.write(writeable_contents.toUtf8()) << "Bytes to" << scriptfile;
    // Set permission 0755
    file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser |
                        QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                        QFileDevice::ReadOther | QFileDevice::ExeOther);
    file.flush();
    file.close();

    return true;
}

void MainWindow::worker_started()
{
    qDebug().noquote() << m_worker->program() << m_worker->arguments();
    ui->statusbar->showMessage(tr("Building..."));
}

void MainWindow::worker_finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        ui->statusbar->showMessage(tr("Done. File saved to ") + m_filename);
    } else {
        QString errstr;
        if (exitStatus != QProcess::NormalExit) {
            errstr = m_worker->errorString().trimmed();
        }
        if (errstr.isEmpty()) {
            errstr = m_worker->readAllStandardError().trimmed();
            if (errstr.isEmpty()) {
                errstr = m_worker->readAllStandardOutput().trimmed();
            }
        }
        qDebug().noquote() << "Exit:" << exitCode << exitStatus << "Error:" << errstr;
        ui->statusbar->showMessage(tr("Error: ") + errstr);
    }
    QDir temp(m_tempdir);
    temp.removeRecursively();
}

void MainWindow::worker_error(QProcess::ProcessError error)
{
    qDebug().noquote() << "Error:" << error << m_worker->errorString();
    Q_UNUSED(error)

    ui->statusbar->showMessage(tr("Error: ") + m_worker->errorString());
}

// UI Slots
void MainWindow::on_PB_Build_clicked()
{
    ui->statusbar->clearMessage();

    if (!updateInfo()) {
        return;
    }

    // Construct build entries
    if (QFileInfo::exists(m_debiandir)) {
        initTempDir();
    }

    // Generate control files
    if (!genControlFile()) {
        return;
    }

    if (!m_contents_preinst.isEmpty()) {
        genScriptFile(Utils::BuildPath({m_debiandir, "preinst"}), m_contents_preinst);
    }

    if (!m_contents_postinst.isEmpty()) {
        genScriptFile(Utils::BuildPath({m_debiandir, "postinst"}), m_contents_postinst);
    }

    if (!m_contents_prerm.isEmpty()) {
        genScriptFile(Utils::BuildPath({m_debiandir, "prerm"}), m_contents_prerm);
    }

    if (!m_contents_postrm.isEmpty()) {
        genScriptFile(Utils::BuildPath({m_debiandir, "postrm"}), m_contents_postrm);
    }

    // Build package file
    QStringList args;
    args << "/usr/bin/dpkg-deb" << "--build" << m_tempdir << m_filename;
    m_worker->start("/usr/bin/fakeroot", args);
}

