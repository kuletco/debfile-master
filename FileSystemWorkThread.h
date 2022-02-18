#ifndef FILESYSTEMWORKTHREAD_H
#define FILESYSTEMWORKTHREAD_H

#include <QtGlobal>
#include <QObject>
#include <QFileInfo>
#include <QThread>

class FileSystemWorkThread : public QObject
{
    Q_OBJECT
public:
    explicit FileSystemWorkThread(QObject *parent = nullptr);
    ~FileSystemWorkThread();

    const QFileInfoList &filelist() const { return m_copy_list; }
    const QFileInfoList &failed() const { return m_copy_failed_list; }

private:
    quint64 updateFileList(const QString &dir);

public slots:
    void list(const QString &src);
    void copy(const QString &src, const QString &dest, bool overwrite = false);

signals:
    void progress_count(const QString &file, quint64 copied_count, quint64 total_count);
    void progress_bytes(const QString &file, quint64 copied_bytes, quint64 total_bytes);
    void all_finished();

private:
    quint64 m_copy_bytes;
    quint64 m_copy_count;
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
