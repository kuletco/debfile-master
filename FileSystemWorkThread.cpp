#include <QDebug>
#include <QDir>
#include <unistd.h>

#include "FileSystemWorkThread.h"

QString G_QDir_NameFilter;
QDir::SortFlags G_QDir_SortFlags = QDir::SortFlag::Name | QDir::SortFlag::IgnoreCase;
QDir::Filters G_QDir_Filters = QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot;

FileSystemWorkThread::FileSystemWorkThread(QObject *parent) : QObject(parent)
{
    RegisterMetaTypes(FileSystemWorkThread::WorkType);

    m_current_work = WorkType::WT_None;
    m_sources_bytes = 0;
    m_sources_count = 0;
    m_copied_bytes = 0;
    m_copied_count = 0;
}

FileSystemWorkThread::~FileSystemWorkThread()
{
}

quint64 FileSystemWorkThread::updateFileList(const QString &dir)
{
    quint64 bytes = 0;

    QFileInfo top(dir);
    if (!top.exists() && !top.isSymLink()) {
        qCritical().noquote() << "UpdateFileList Error:" << QString("Source [%1] not exist!").arg(dir);
        return bytes;
    }

    m_entries.append(top);
    if (top.isSymLink()) {
        m_linklist.append(top);
    } else if (top.isFile()) {
        m_filelist.append(top);
    } else if (top.isDir()) {
        m_dirlist.append(top);
        QDir top_dir(top.absoluteFilePath());
        const QFileInfoList entrylist = top_dir.entryInfoList(G_QDir_Filters);
        for (const QFileInfo &node : entrylist) {
            // cppcheck-suppress useStlAlgorithm
            bytes += updateFileList(node.absoluteFilePath());
        }
    }
    bytes += top.size();

    return bytes;
}

void FileSystemWorkThread::update(const QString &src)
{
    m_current_work = WorkType::WT_Calc;

    m_sources_bytes = updateFileList(src);
    m_sources_count = m_entries.size();

    emit work_finished(m_current_work);
}

void FileSystemWorkThread::list(const QString &src)
{
    m_current_work = WorkType::WT_List;

    qDebug().noquote() << "ThreadID:" << QThread::currentThreadId() << "Func:" << __FUNCTION__;
    updateFileList(src);
    qDebug().noquote() << "Files:";
    for (const QFileInfo &node : qAsConst(m_entries)) {
        qDebug().noquote() << "  " << node.absoluteFilePath();
    }
    qDebug().noquote() << "Total:" << m_filelist.size() << "Files," << m_dirlist.size() << "Directories," << m_linklist.size() << "Links";

    emit work_finished(m_current_work);
}

void FileSystemWorkThread::copy(const QString &src, const QString &dest, bool overwrite)
{
    update(src);
    qDebug().noquote() << "ThreadID:" << QThread::currentThreadId() << "Func:" << __FUNCTION__ << "TotalFiles:" << m_sources_count << "TotalBytes:" << m_sources_bytes;
    qDebug().noquote() << "Copy:" << "Source:" << src;
    qDebug().noquote() << "     " << "  Dest:" << dest;

    m_current_work = WorkType::WT_Copy;

    m_copied_bytes = 0;
    m_copied_count = 0;
    m_copy_failed_list.clear();

    QString basedir;
    QFileInfo si(src);
    QDir src_dir(src);
    QDir dest_dir(dest);

    basedir = si.absoluteFilePath();
    qDebug().noquote() << "BaseDir:" << basedir;
    if (!src.endsWith("/")) {
        QString dirname = src_dir.dirName();
        basedir.remove(dirname);
    }
    qDebug().noquote() << "BaseDir:" << basedir;

    qDebug().noquote()<< "DestDir:" << dest_dir.absolutePath();
    for (const QFileInfo &entry : qAsConst(m_entries)) {
        QString source = entry.absoluteFilePath();
        QString source_entry = entry.absoluteFilePath().remove(basedir);
        QString destination = dest_dir.absoluteFilePath(source_entry);
        if (QFile::exists(destination) && !overwrite) {
            qWarning().noquote() << QString("Target [%1] exist skip copy it!").arg(destination);
            m_copy_failed_list.append(entry);
        } else {
            QFile action;
            QFileInfo fi(destination);
            if (fi.exists() || fi.isSymLink()) {
                if (!overwrite) {
                    qWarning().noquote() << QString("Target [%1] exist, skip copy!").arg(destination);
                    m_copy_failed_list.append(entry);
                    continue;
                }
                if (fi.isDir()) {
                    QDir _dir(destination);
                    if (!_dir.removeRecursively()) {
                        qCritical().noquote() << "Copy Error:" << QString("Remove [%1] failed!").arg(destination);
                        m_copy_failed_list.append(entry);
                        continue;
                    }
                } else {
                    if (!action.remove()) {
                        qCritical().noquote() << QString("Target [%1] exist and remove it failed!").arg(destination) << "ErrCode:" << action.error() << action.errorString();
                        m_copy_failed_list.append(entry);
                        continue;
                    }
                }
            }
            if (entry.isSymLink()) {
                QByteArray linkTarget(4096, '\0');
                ssize_t len = readlink(entry.absoluteFilePath().toUtf8(), linkTarget.data(), linkTarget.size() - 1);
                if (linkTarget.size() < len) {
                    qCritical().noquote() << "Copy Error:" << QString("Get link [%1] target failed!").arg(entry.absoluteFilePath());
                    continue;
                }
                action.setFileName(linkTarget);
                qDebug().noquote() << "Link:" << QString("[%1] -> [%2]").arg(destination, QString(linkTarget));
                if (!action.link(destination)) {
                    qCritical().noquote() << "Copy Error:" << QString("Create Link [%1] failed!").arg(destination) << "ErrCode:" << action.error() << action.errorString();
                    m_copy_failed_list.append(entry);
                    continue;
                }
            } else if (entry.isFile()) {
                action.setFileName(source);
                qDebug().noquote() << "Copy:" << QString("[%1] -> [%2]").arg(source, destination);
                if (!action.copy(destination)) {
                    qCritical().noquote() << QString("Copy [%1] to [%2] failed!").arg(source, destination) << "Copy Error:" << action.error() << action.errorString();
                    m_copy_failed_list.append(entry);
                }
            } else if (entry.isDir()) {
                QFileInfo _fi(destination);
                QDir _dir(_fi.absolutePath());
                _dir.mkpath(_fi.fileName());
                qDebug().noquote() << "Create Dir:" << destination;
            } else {
                qCritical().noquote() << "Copy Error:" << "Unsupported file type!";
                return;
            }
        }
        m_copied_count++;
        m_copied_bytes += entry.size();
        emit progress_count(entry.absoluteFilePath(), m_copied_count, m_sources_count);
        emit progress_bytes(entry.absoluteFilePath(), m_copied_bytes, m_sources_bytes);
    }

    emit work_finished(m_current_work);
}
