#include <QDebug>
#include <unistd.h>
#include "FileSystemModel.h"
#include "qnamespace.h"

#define NO_PERMISSION_STR  "---------"

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel{parent}
{
    m_sources_bytes = 0;
    m_sources_count = 0;
    m_copied_bytes = 0;
    m_copied_count = 0;

    // cppcheck-suppress useInitializationList
    m_worker = new FileSystemWorkThread();
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, SIGNAL(finished()), this, SLOT(work_thread_finished()));
    connect(m_worker, SIGNAL(work_finished(FileSystemWorkThread::WorkType)), this, SLOT(work_thread_finished(FileSystemWorkThread::WorkType)));
    connect(m_worker, SIGNAL(progress_count(QString,quint64,quint64)), this, SIGNAL(copy_work_progress_count(QString,quint64,quint64)));
    connect(m_worker, SIGNAL(progress_bytes(QString,quint64,quint64)), this, SIGNAL(copy_work_progress_bytes(QString,quint64,quint64)));
    connect(this, SIGNAL(do_calc(QString)), m_worker, SLOT(calc(QString)));
    connect(this, SIGNAL(do_copy(QString,QString,bool)), m_worker, SLOT(copy(QString,QString,bool)));
    m_thread.start();

    iconProvider()->setOptions(QFileIconProvider::DontUseCustomDirectoryIcons);
    setResolveSymlinks(false);
}

FileSystemModel::~FileSystemModel()
{
    if (m_thread.isRunning()) {
        m_thread.quit();
        m_thread.wait();
    }
    if (!m_worker.isNull()) {
        delete m_worker;
    }
}

QString FileSystemModel::permission2str(QFile::Permissions permissions) const
{
    QString str = NO_PERMISSION_STR;
    if (permissions & QFile::Permission::ReadOwner) { str[0] = 'r'; }
    if (permissions & QFile::Permission::WriteOwner) { str[1] = 'w'; }
    if (permissions & QFile::Permission::ExeOwner) { str[2] = 'x'; }

    if (permissions & QFile::Permission::ReadGroup) { str[3] = 'r'; }
    if (permissions & QFile::Permission::WriteGroup) { str[4] = 'w'; }
    if (permissions & QFile::Permission::ExeGroup) { str[5] = 'x'; }

    if (permissions & QFile::Permission::ReadOther) { str[6] = 'r'; }
    if (permissions & QFile::Permission::WriteOther) { str[7] = 'w'; }
    if (permissions & QFile::Permission::ExeOther) { str[8] = 'x'; }

    return str;
}

QFile::Permissions FileSystemModel::str2permission(const QString &str) const
{
    QFile::Permissions permissions;

    if (str.size() !=  QString(NO_PERMISSION_STR).size()) {
        return permissions;
    }

    if (str.at(0) == 'r') { permissions |= QFile::Permission::ReadOwner; }
    if (str.at(1) == 'r') { permissions |= QFile::Permission::WriteOwner; }
    if (str.at(2) == 'r') { permissions |= QFile::Permission::ExeOwner; }

    if (str.at(3) == 'r') { permissions |= QFile::Permission::ReadGroup; }
    if (str.at(4) == 'r') { permissions |= QFile::Permission::WriteGroup; }
    if (str.at(5) == 'r') { permissions |= QFile::Permission::ExeGroup; }

    if (str.at(6) == 'r') { permissions |= QFile::Permission::ReadOther; }
    if (str.at(7) == 'r') { permissions |= QFile::Permission::WriteOther; }
    if (str.at(8) == 'r') { permissions |= QFile::Permission::ExeOther; }

    return permissions;
}

void FileSystemModel::work_thread_finished(FileSystemWorkThread::WorkType type)
{
    qDebug().noquote() << "WorkFinished!";
    switch (type) {
    case FileSystemWorkThread::WorkType::WT_Calc:
        qDebug().noquote() << "Calc finished!" << "Entry Count:" << m_sources_count << "Size:" << m_sources_bytes;
        m_sources_count = m_worker->files();
        m_sources_bytes = m_worker->entrysize();
        break;
    case FileSystemWorkThread::WorkType::WT_Copy:
        emit copy_work_finished();
        break;
    default:
        break;
    }
}

void FileSystemModel::calc(const QString &src)
{
    if (m_sources_bytes == 0 || m_sources_count == 0) {
        emit do_calc(src);
    }
}

void FileSystemModel::copy(const QString &src, const QString &dest, bool overwrite)
{
    emit do_copy(src, dest, overwrite);
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    return ((parent.column() > 0) ? 0 : static_cast<int>(ExColumns::NumColumns));
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemFlag::NoItemFlags;
    }

    ExColumns column = static_cast<ExColumns>(index.column());

#ifdef FSM_SHOW_PERMISSION
    if (column == ExColumns::Permission) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
#endif

    return QFileSystemModel::flags(index);
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    ExColumns column = static_cast<ExColumns>(index.column());

    switch (column) {
#ifdef FSM_SHOW_PERMISSION
    case ExColumns::Permission: {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            QFile::Permissions perm = permissions(index);
            return QVariant(permission2str(perm));
        }
        default: break;
        }
        break;
    }
#endif
#ifdef FSM_SHOW_OWNER
    case ExColumns::Owner: {
        QFileInfo fi = fileInfo(index);
        return QVariant(fi.owner());
    }
#endif
#ifdef FSM_SHOW_GROUP
    case ExColumns::Group: {
        QFileInfo fi = fileInfo(index);
        return QVariant(fi.group());
    }
#endif
    default: break;
    }

    return QFileSystemModel::data(index, role);
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    ExColumns column = static_cast<ExColumns>(section);

    switch (column) {
    case ExColumns::Time: {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (orientation == Qt::Horizontal) {
                return tr("Modified Time");
            }
            break;
        }
        }
        break;
    }
#ifdef FSM_SHOW_PERMISSION
    case ExColumns::Permission: {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (orientation == Qt::Horizontal) {
                return tr("Permission");
            }
            break;
        }
        }
        break;
    }
#endif
#ifdef FSM_SHOW_OWNER
    case ExColumns::Owner: {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (orientation == Qt::Horizontal) {
                return tr("Owner");
            }
            break;
        }
        }
        break;
    }
#endif
#ifdef FSM_SHOW_GROUP
    case ExColumns::Group: {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (orientation == Qt::Horizontal) {
                return tr("Group");
            }
            break;
        }
        }
        break;
    }
#endif
    default: break;
    }

    return QFileSystemModel::headerData(section, orientation, role);
}
