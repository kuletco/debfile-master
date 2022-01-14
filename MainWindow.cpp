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

    m_date_updater = new QTimer();
    connect(m_date_updater, SIGNAL(timeout()), this, SLOT(update_date()));

    m_deb = new DEBFile();
    connect(m_deb, SIGNAL(work_started(QString)), this, SLOT(work_updated(QString)));
    connect(m_deb, SIGNAL(work_finished(QString)), this, SLOT(work_updated(QString)));
    connect(m_deb, SIGNAL(work_failed(QString)), this, SLOT(work_updated(QString)));

    m_fsmodel = new FileSystemModel();
    connect(m_fsmodel, SIGNAL(rootPathChanged(QString)), this, SLOT(fsmode_RootPathChanged(QString)));
    ui->treeView_Files->setModel(m_fsmodel);

    // Init defaults
    setWindowIcon(QIcon(":/icons/logo"));

    ui->statusbar->clearMessage();
    ui->LB_AppVer->setText(qApp->applicationVersion());

    update_date();
    qDebug().noquote() << "AppVer:" << qApp->applicationVersion();
    qDebug().noquote() << "Date:" << ui->LB_Date->text();

    m_date_updater->start(1000 * 60); // Update Date every minute
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_date_updater->isActive()) {
        m_date_updater->stop();
    }
    delete m_date_updater;
    delete m_deb;
    delete m_fsmodel;
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    QWidget::keyReleaseEvent(e);
    if (e->key() == Qt::Key_Escape) {
        qApp->quit();
    }
}

void MainWindow::update_date()
{
    QDate date = QDate::currentDate();
    QString date_str = date.toString("yyyy-MM-dd");
    ui->LB_Date->setText(date_str);
}

void MainWindow::work_updated(QString info)
{
    ui->statusbar->showMessage(info);
}

// UI Slots
void MainWindow::fsmode_RootPathChanged(const QString &newPath)
{
    Q_UNUSED(newPath)
    // TODO: Update TreeView manual
}

void MainWindow::on_tabWidget_Main_currentChanged(int index)
{
    switch (static_cast<Pages>(index)) {
    case Pages::Information: break;
    case Pages::Scripts: break;
    case Pages::Files: {
        if (m_fsmodel->rootPath().isEmpty() || (m_fsmodel->rootPath() != m_deb->buildroot())) {
            qDebug().noquote() << "Build Root:" << m_deb->buildroot();
            m_fsmodel->setRootPath(m_deb->buildroot());
            ui->treeView_Files->setRootIndex(m_fsmodel->index(m_deb->buildroot()));
        }
        break;
    }
    default: break;
    }
}

void MainWindow::on_PB_Build_clicked()
{
    ui->statusbar->clearMessage();

    // Get all settings from UI
    m_deb->m_name = ui->LE_Package->text();
    m_deb->m_version = ui->LE_Version->text();
    m_deb->m_maintainer = ui->LE_Maintainer->text();
    m_deb->m_homepage = ui->LE_HomePage->text();
    m_deb->m_summary = ui->LE_Summary->text();
    m_deb->m_description = ui->TE_Description->toPlainText();
    m_deb->m_depends = ui->LE_Depends->text();
    m_deb->m_predepends = ui->LE_PreDepends->text();
    m_deb->m_conflicts = ui->LE_Conflicts->text();
    m_deb->m_replaces = ui->LE_Replaces->text();
    m_deb->m_provides = ui->LE_Provides->text();
    m_deb->m_protected = ui->CB_Protected->isChecked();
    m_deb->m_type = static_cast<DEBAttrs::Type>(ui->ComboBox_PackageType->currentIndex());
    m_deb->m_architecture = static_cast<DEBAttrs::Architecture>(ui->ComboBox_Arch->currentIndex());
    m_deb->m_priority = static_cast<DEBAttrs::Priority>(ui->ComboBox_Priority->currentIndex());
    m_deb->m_section = static_cast<DEBAttrs::Section>(ui->ComboBox_Section->currentIndex());
    m_deb->m_contents_preinst = ui->TE_PreInst->toPlainText();
    m_deb->m_contents_postinst = ui->TE_PostInst->toPlainText();
    m_deb->m_contents_prerm = ui->TE_PreRM->toPlainText();
    m_deb->m_contents_postrm = ui->TE_PostRM->toPlainText();

    if (m_deb->m_name.isEmpty()) {
        ui->statusbar->showMessage(tr("Package name is empty!"));
        return;
    }

    if (m_deb->m_version.isEmpty()) {
        ui->statusbar->showMessage(tr("Package version is empty!"));
        return;
    }

    if (m_deb->m_architecture == DEBAttrs::Architecture::invalid) {
        ui->statusbar->showMessage(tr("Invalid architecture!"));
        return;
    }

    if (m_deb->m_summary.isEmpty()) {
        ui->statusbar->showMessage(tr("Package summary is empty!"));
        return;
    }

    if (m_deb->m_type == DEBAttrs::Type::invalid) {
        ui->statusbar->showMessage(tr("Invalid package type!"));
        return;
    }

    // Create Build Dir
    m_deb->CreateBuildDir();

    // Generate control files
    if (!m_deb->CreateControlFile()) {
        return;
    }

    if (!m_deb->CreateScriptFiles()) {
        return;
    }

    // Popup a dialog to save deb file
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/" + m_deb->filename(), tr("Debian Package (*.deb *.udeb)"));
    qDebug().noquote() << "Package File:" << filename;
    m_deb->CreatePackage(filename);
}

