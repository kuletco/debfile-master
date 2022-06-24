#include <QDate>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <stdlib.h>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "utils.h"

#define DEBUG_MODE

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QLabel space;
    space.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->statusbar->addPermanentWidget(&space);

    m_progress = new QProgressBar(ui->statusbar);
    m_progress->setMaximumWidth(150);
    m_progress->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_progress->setRange(0, 100);
    ui->statusbar->addPermanentWidget(m_progress);
    m_progress->hide();

    m_date_updater = new QTimer();
    connect(m_date_updater, SIGNAL(timeout()), this, SLOT(update_date()));

    m_deb = new DEBFile();
    connect(m_deb, SIGNAL(work_started()), this, SLOT(work_started()));
    connect(m_deb, SIGNAL(work_finished()), this, SLOT(work_finished()));
    connect(m_deb, SIGNAL(work_failed()), this, SLOT(work_failed()));
    connect(m_deb, SIGNAL(work_update(QString)), this, SLOT(work_update(QString)));
    connect(m_deb, SIGNAL(buildroot_cleared()), this, SLOT(clear_buildroot()));
    connect(m_deb, SIGNAL(buildroot_changed(QString)), this, SLOT(change_buildroot(QString)));

    // Files TreeView Settings
    ui->treeView_Files->setEditTriggers(QTreeView::EditKeyPressed);
    ui->treeView_Files->setExpandsOnDoubleClick(true);
//    ui->treeView_Files->header()->setStretchLastSection(false);
//    ui->treeView_Files->header()->setSectionResizeMode(0, QHeaderView::Stretch);

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
        // FIXME: ESC will exit app either!!!!
        qDebug().noquote() << "Window Modality:" << this->windowModality();
        if (this->isActiveWindow()) {
            qApp->quit();
        }
    }
}

void MainWindow::update_date()
{
    QDate date = QDate::currentDate();
    QString date_str = date.toString("yyyy-MM-dd");
    ui->LB_Date->setText(date_str);
}

void MainWindow::work_started()
{
    // qDebug().noquote() << "Work Started";
}

void MainWindow::work_finished()
{
    // qDebug().noquote() << "Work Finished";
}

void MainWindow::work_failed()
{
    // qDebug().noquote() << "Work Failed";
}

void MainWindow::work_update(QString info)
{
    ui->statusbar->showMessage(info);
}

void MainWindow::copy_progress(const QString &file, quint64 copied, quint64 count)
{
    int persent = copied / count * 100;

    if (persent < 100) {
        QFileInfo fi(file);
        QString msg = QString(tr("Copying file (%1/%2): %3")).arg(QString::number(copied), QString::number(count), fi.fileName());

        ui->statusbar->showMessage(msg);
        m_progress->setValue(copied / count * 100);
        m_progress->show();
    } else {
        ui->statusbar->clearMessage();
        m_progress->hide();
    }
}

void MainWindow::init_fsmodel()
{
    clear_fsmodel();
    m_fsmodel = new FileSystemModel();
    connect(m_fsmodel, SIGNAL(rootPathChanged(QString)), this, SLOT(fsmodel_RootPathChanged(QString)));
    connect(m_fsmodel, SIGNAL(directoryLoaded(QString)), this, SLOT(fsmodel_DirectoryLoaded(QString)));
    connect(m_fsmodel, SIGNAL(copy_work_progress_count(QString,quint64,quint64)), this, SLOT(copy_progress(QString,quint64,quint64)));
    ui->treeView_Files->setModel(m_fsmodel);
}

void MainWindow::clear_fsmodel()
{
    if (!m_fsmodel.isNull()) {
        clear_file_treeview();

//        m_fsmodel->setRootPath("");
//        m_fsmodel->setReadOnly(true);
        m_fsmodel->disconnect();
        delete m_fsmodel;
    }
}

// UI Slots
void MainWindow::clear_file_treeview()
{
    ui->treeView_Files->setModel(nullptr);
    ui->treeView_Files->reset();
}

void MainWindow::clear_buildroot()
{
    if (!m_fsmodel.isNull()) {
        clear_file_treeview();
    }
}

void MainWindow::change_buildroot(const QString &buildroot)
{
    qDebug().noquote() << "Change BuildRoot:" << buildroot;
    if (m_fsmodel.isNull()) {
        init_fsmodel();
    }
    if (m_fsmodel->rootPath().isEmpty() || (m_fsmodel->rootPath() != buildroot)) {
        qDebug().noquote() << "Build Root:" << buildroot;
        m_fsmodel->setRootPath(buildroot);
        m_fsmodel->setReadOnly(false);
        ui->treeView_Files->setRootIndex(m_fsmodel->index(buildroot));
        ui->treeView_Files->selectionModel()->setCurrentIndex(m_fsmodel->index(buildroot), QItemSelectionModel::Select);
        ui->treeView_Files->resizeColumnToContents(3);
    }
}

void MainWindow::fsmodel_RootPathChanged(const QString &newPath)
{
    Q_UNUSED(newPath)
    // TODO: Update TreeView manual
}

void MainWindow::fsmodel_DirectoryLoaded(const QString &path)
{
    qDebug().noquote() << "Directory Loaded:" << path;

    for (int column = 0; column < m_fsmodel->columnCount(); column++) {
        ui->treeView_Files->resizeColumnToContents(column);
    }
}

void MainWindow::on_tabWidget_Main_currentChanged(int index)
{
    switch (static_cast<Pages>(index)) {
    case Pages::Information: break;
    case Pages::Scripts: break;
    case Pages::Files: {
        change_buildroot(m_deb->buildroot());
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

void MainWindow::on_TB_Add_Dir_clicked()
{
    if (m_fsmodel.isNull()) { return; }

    QItemSelectionModel *model = ui->treeView_Files->selectionModel();
    QModelIndex index = model->currentIndex();
    qDebug().noquote() << "Current Index:" << m_fsmodel->filePath(index);
    if (!index.isValid()) {
        model->setCurrentIndex(m_fsmodel->index(m_deb->buildroot()), QItemSelectionModel::Select);
        index = model->currentIndex();
    }
    if (!index.isValid()) {
        qCritical().noquote() << "Invalid Selection";
        return;
    }
    QModelIndex new_index = m_fsmodel->mkdir(index, tr("New Folder"));
    model->setCurrentIndex(new_index, QItemSelectionModel::QItemSelectionModel::Select);
    qDebug().noquote() << "Current Index:" << m_fsmodel->filePath(model->currentIndex());
    ui->treeView_Files->edit(new_index);
}

void MainWindow::on_TB_Import_File_clicked()
{
    if (m_fsmodel.isNull()) { return; }
    QStringList sources;
    QString title(tr("Select files"));
    // Pop-up a dialog to select source files
#if 0
    QFileDialog dialog(this, title, QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setOptions(QFileDialog::ReadOnly | QFileDialog::DontResolveSymlinks);
    if (dialog.exec()) {
        sources = dialog.selectedFiles();
    }
#else
    sources = QFileDialog::getOpenFileNames(this, title, QDir::homePath());
#endif
    qDebug().noquote() << "Sources Dirs:";
    qDebug().noquote() << sources.join("\n");

    QItemSelectionModel *model = ui->treeView_Files->selectionModel();
    qDebug().noquote() << "Copy Files to BuildDir:" << sources << "->" << m_fsmodel->filePath(model->currentIndex());
    for (const QString &source : qAsConst(sources)) {
        m_fsmodel->copy(source, m_fsmodel->filePath(model->currentIndex()), true);
    }
}

void MainWindow::on_TB_Import_Dir_clicked()
{
    if (m_fsmodel.isNull()) { return; }
    QStringList sources;
    QString title(tr("Select a folder"));
    // Pop-up a dialog to select source folders
#if 0
    QFileDialog dialog(this, title, QDir::homePath());
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOptions(QFileDialog::ReadOnly | QFileDialog::DontResolveSymlinks);
    if (dialog.exec()) {
        sources = dialog.selectedFiles();
        qDebug().noquote() << "Select:" << dialog.selectedUrls() << dialog.selectedNameFilter();
    }
#else
    sources << QFileDialog::getExistingDirectory(this, title, QDir::homePath());
#endif
    qDebug().noquote() << "Sources Dirs:";
    qDebug().noquote() << sources.join("\n");

    QItemSelectionModel *model = ui->treeView_Files->selectionModel();
    qDebug().noquote() << "Copy Folder to BuildDir:" << sources << "->" << m_fsmodel->filePath(model->currentIndex());
    for (const QString &source : qAsConst(sources)) {
        m_fsmodel->copy(source, m_fsmodel->filePath(model->currentIndex()), true);
    }
}

void MainWindow::on_TB_Remove_clicked()
{
    if (m_fsmodel.isNull()) { return; }

    QModelIndex index = ui->treeView_Files->selectionModel()->currentIndex();
    QModelIndex root = m_fsmodel->index(m_deb->buildroot());
    qDebug().noquote() << "Root Path:" << m_fsmodel->filePath(root) << ", Selected Path:" << m_fsmodel->filePath(index);
    if (index != root) {
        if (m_fsmodel->isDir(index)) {
            qDebug().noquote() << "Remove Dir:" << m_fsmodel->filePath(index);
#ifdef DEBUG_MODE
            m_fsmodel->rmdir(index);
#endif
        } else {
            qDebug().noquote() << "Remove File:" << m_fsmodel->filePath(index);
#ifdef DEBUG_MODE
            m_fsmodel->remove(index);
#endif
        }
    }
}

