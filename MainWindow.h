#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QProgressBar>

#include "DEBFile.h"
#include "FileSystemModel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum class Pages {
        Information = 0,
        Scripts,
        Files,
        NumPages,
    };
    Q_ENUM(Pages)

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyReleaseEvent(QKeyEvent *e);

private slots:
    void update_date();
    void work_updated(QString info);
    void copy_progress(const QString &file, quint64 copied, quint64 count);

    // UI Slots
    void fsmodel_RootPathChanged(const QString &newPath);
    void fsmodel_DirectoryLoaded(const QString &path);
    void on_tabWidget_Main_currentChanged(int index);
    void on_PB_Build_clicked();
    void on_TB_Add_Dir_clicked();
    void on_TB_Import_File_clicked();
    void on_TB_Import_Dir_clicked();
    void on_TB_Remove_clicked();

private:
    Ui::MainWindow *ui;
    QPointer<QProgressBar> m_progress;
    QPointer<QTimer> m_date_updater;
    QPointer<DEBFile> m_deb;
    QPointer<FileSystemModel> m_fsmodel;
};
#endif // MAINWINDOW_H
