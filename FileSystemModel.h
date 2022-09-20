#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include "qglobal.h"
#include <QObject>
#include <QPointer>
#include <QMetaType>
#include <QMetaEnum>
#include <QFile>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QThread>

#include "FileSystemWorkThread.h"

#define FSM_SHOW_PERMISSION
//#define FSM_SHOW_OWNER
//#define FSM_SHOW_GROUP

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT
public:
    enum class ExColumns {
        Display = 0,
        Size,
        Type,
        Time,
#ifdef FSM_SHOW_PERMISSION
        Permission,
#endif
#ifdef FSM_SHOW_OWNER
        Owner,
#endif
#ifdef FSM_SHOW_GROUP
        Group,
#endif
        NumColumns,
    };
    Q_ENUM(ExColumns)

    explicit FileSystemModel(QObject *parent = nullptr);
    ~FileSystemModel();

    quint64 files() const { return m_sources_count; }
    quint64 entrysize() const { return m_sources_bytes; }

    void calc(const QString &src);

    void copy(const QString &src, const QString &dest, bool overwrite = false);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

protected:
    // Override
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QString permission2str(QFile::Permissions permissions) const;
    QFile::Permissions str2permission(const QString &str) const;

private slots:
    void work_thread_finished();
    void work_thread_finished(FileSystemWorkThread::WorkType type);

signals:
    void do_calc(const QString &src);
    void do_copy(const QString &src, const QString &dest, bool overwrite);
    void copy_work_progress_count(const QString &file, quint64 copied, quint64 count);
    void copy_work_progress_bytes(const QString &file, quint64 copied, quint64 totalsize);
    void copy_work_finished();

private:
    quint64 m_sources_bytes;
    quint64 m_sources_count;
    quint64 m_copied_bytes;
    quint64 m_copied_count;

    QThread m_thread;
    QPointer<FileSystemWorkThread> m_worker;
};

#endif // FILESYSTEMMODEL_H
