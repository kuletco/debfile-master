#ifndef FILESYSTEMWORKTHREAD_H
#define FILESYSTEMWORKTHREAD_H

#include "qglobal.h"
#include <QtGlobal>
#include <QObject>
#include <QFileInfo>
#include <QThread>

#include "utils.h"

class FileSystemWorkThread : public QObject
{
    Q_OBJECT
public:
    enum class WorkType {
        WT_None,
        WT_List,
        WT_Calc,
        WT_Copy,

        Invalid = -1,
    };
    Q_ENUM(WorkType)

    explicit FileSystemWorkThread(QObject *parent = nullptr);
    ~FileSystemWorkThread();

    const QFileInfoList &filelist() const { return m_copy_list; }
    const QFileInfoList &failed() const { return m_copy_failed_list; }

    quint64 files() const { return m_sources_count; }
    quint64 entrysize() const { return m_sources_bytes; }

private:
    quint64 updateFileList(const QString &dir);

public slots:
    void update(const QString &src);
    void list(const QString &src);
    void copy(const QString &src, const QString &dest, bool overwrite = false);

signals:
    void progress_count(const QString &file, quint64 copied_count, quint64 total_count);
    void progress_bytes(const QString &file, quint64 copied_bytes, quint64 total_bytes);
    void work_finished(FileSystemWorkThread::WorkType type);

private:
    WorkType m_current_work;

    quint64 m_sources_bytes;
    quint64 m_sources_count;
    quint64 m_copied_bytes;
    quint64 m_copied_count;

    QFileInfoList m_entries;
    QFileInfoList m_linklist;
    QFileInfoList m_dirlist;
    QFileInfoList m_filelist;
    QFileInfoList m_copy_list;
    QFileInfoList m_copy_failed_list;
};

#endif // FILESYSTEMWORKTHREAD_H
