#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QObject>
#include <QMetaType>
#include <QMetaEnum>
#include <QFile>
#include <QFileSystemModel>
#include <QFileIconProvider>

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

protected:
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

public:
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    QString permission2str(QFile::Permissions permissions) const;
    QFile::Permissions str2permission(const QString &str) const;
};

#endif // FILESYSTEMMODEL_H
