#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>

#include "DEBFile.h"

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
    void on_tabWidget_Main_currentChanged(int index);
    void on_PB_Build_clicked();

private:
    Ui::MainWindow *ui;
    QPointer<QTimer> m_date_updater;
    QPointer<DEBFile> m_deb;
};
#endif // MAINWINDOW_H
