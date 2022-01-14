#include <QApplication>
#include <QLocale>
#include <QTranslator>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationVersion(QString(APP_VERSION));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();

    return a.exec();
}
