
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

void MainWindow::on_PB_Build_clicked()
{
    ui->statusbar->clearMessage();

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
    m_preinst = ui->TE_PreInst->toPlainText();
    m_postinst = ui->TE_PostInst->toPlainText();
    m_prerm = ui->TE_PreRM->toPlainText();
    m_postrm = ui->TE_PostRM->toPlainText();

    if (m_name.isEmpty()) {
        ui->statusbar->showMessage(tr("Package name is empty!"));
        return;
    }

    if (m_version.isEmpty()) {
        ui->statusbar->showMessage(tr("Package version is empty!"));
        return;
    }

    if (m_architecture == Architecture::invalid) {
        ui->statusbar->showMessage(tr("Invalid architecture!"));
        return;
    }

    if (m_summary.isEmpty()) {
        ui->statusbar->showMessage(tr("Package summary is empty!"));
        return;
    }

    if (m_type == Type::invalid) {
        ui->statusbar->showMessage(tr("Invalid package type!"));
        return;
    }

    // Popup a dialog to save deb file
    QString filename = QString("%1_%2_%3.%4").arg(m_name, m_version, Utils::EnumConvert(m_architecture), Utils::EnumConvert(m_type));
    m_filename = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/" + filename, tr("Debian Package (*.deb *.udeb)"));
    qDebug().noquote() << "Package File:" << m_filename;

    // Construct build entries
    char tempname[] = "DEBMAKER.XXXXXX";
    if (!mkdtemp(tempname)) {
        qWarning().noquote() << "Cannot create a uniquely-named tempdir!";
    }
    m_tempdir = tempname;
    QDir temp(tempname);
    temp.mkpath("DEBIAN");
    QString debian_dir = temp.absoluteFilePath("DEBIAN");

    // Generate control files
    QStringList control_file_contents;
    control_file_contents << QString("Package: %1").arg(m_name);
    control_file_contents << QString("Version: %1").arg(m_version);
    control_file_contents << QString("Size: 100");
    if (m_architecture != Architecture::invalid)
        control_file_contents << QString("Architecture: %1").arg(Utils::EnumConvert(m_architecture).constData());
    if (m_section != Section::invalid)
        control_file_contents << QString("Section: %1").arg(Utils::EnumConvert(m_section).constData());
    if (m_priority != Priority::invalid)
        control_file_contents << QString("Priority: %1").arg(Utils::EnumConvert(m_priority).constData());
    if (m_protected)
        control_file_contents << QString("Protected: yes");
    if (!m_maintainer.isEmpty())
        control_file_contents << QString("Maintainer: %1").arg(m_maintainer);
    if (!m_homepage.isEmpty())
        control_file_contents << QString("Homepage: %1").arg(m_homepage);
    if (!m_predepends.isEmpty())
        control_file_contents << QString("Pre-Depends: %1").arg(m_predepends);
    if (!m_depends.isEmpty())
        control_file_contents << QString("Depends: %1").arg(m_depends);
    if (!m_conflicts.isEmpty())
        control_file_contents << QString("Conflicts: %1").arg(m_conflicts);
    if (!m_replaces.isEmpty())
        control_file_contents << QString("Replaces: %1").arg(m_replaces);
    if (!m_provides.isEmpty())
        control_file_contents << QString("Provides: %1").arg(m_provides);
    if (!m_summary.isEmpty())
        control_file_contents << QString("Description: %1").arg(m_summary);
    // TODO: Need format long-description contents (https://manpages.debian.org/testing/dpkg-dev/deb-control.5.en.html#Description:)
    if (!m_description.isEmpty())
        control_file_contents << QString(" %1").arg(m_description);

    control_file_contents << "";

    QString control_file = Utils::BuildPath({debian_dir, "control"});
    QFile file(control_file);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(control_file));
        return;
    }
    qDebug().noquote() << "Write:" << file.write(control_file_contents.join("\n").toUtf8()) << "Bytes to" << control_file;
    file.flush();
    file.close();

    if (!m_preinst.isEmpty()) {
        QString preinst_file = Utils::BuildPath({debian_dir, "preinst"});
        QFile file(preinst_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(preinst_file));
            return;
        }
        if (!m_preinst.endsWith("\n")) {
            m_preinst.append('\n');
        }
        qDebug().noquote() << "Write:" << file.write(m_preinst.toUtf8()) << "Bytes to" << preinst_file;
        // Set permission 0755
        file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);
        file.flush();
        file.close();
    }

    if (!m_postinst.isEmpty()) {
        QString postinst_file = Utils::BuildPath({debian_dir, "postinst"});
        QFile file(postinst_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(postinst_file));
            return;
        }
        if (!m_postinst.endsWith("\n")) {
            m_postinst.append('\n');
        }
        qDebug().noquote() << "Write:" << file.write(m_postinst.toUtf8()) << "Bytes to" << postinst_file;
        // Set permission 0755
        file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);
        file.flush();
        file.close();
    }

    if (!m_prerm.isEmpty()) {
        QString prerm_file = Utils::BuildPath({debian_dir, "prerm"});
        QFile file(prerm_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(prerm_file));
            return;
        }
        if (!m_prerm.endsWith("\n")) {
            m_prerm.append('\n');
        }
        qDebug().noquote() << "Write:" << file.write(m_prerm.toUtf8()) << "Bytes to" << prerm_file;
        // Set permission 0755
        file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);
        file.flush();
        file.close();
    }

    if (!m_postrm.isEmpty()) {
        QString postrm_file = Utils::BuildPath({debian_dir, "postrm"});
        QFile file(postrm_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ui->statusbar->showMessage(QString(tr("Open file %1 failed!")).arg(postrm_file));
            return;
        }
        if (!m_postrm.endsWith("\n")) {
            m_postrm.append('\n');
        }
        qDebug().noquote() << "Write:" << file.write(m_postrm.toUtf8()) << "Bytes to" << postrm_file;
        // Set permission 0755
        file.setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);
        file.flush();
        file.close();
    }

    // Build package file
    QStringList args;
    args << "/usr/bin/dpkg-deb" << "--build" << m_tempdir << m_filename;
    m_worker->start("/usr/bin/fakeroot", args);
}

