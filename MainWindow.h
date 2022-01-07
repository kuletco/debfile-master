#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPointer>

#include "DEBFile.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyReleaseEvent(QKeyEvent *e);

private slots:
    void work_updated(QString info);
    void on_PB_Build_clicked();

private:
    Ui::MainWindow *ui;
    QPointer<DEBFile> m_deb;
};
#endif // MAINWINDOW_H
