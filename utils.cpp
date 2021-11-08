#include <QCoreApplication>
#include <QLibraryInfo>
#include <QTranslator>
#include <QFileInfo>
#include <QDir>
#include <QString>

#include "utils.h"

QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
QList<QPointer<QTranslator>> translators;
bool loadded_qt_translator = false;

/* Private */
QString Utils::_findFromDir(const QStringList& SearchList, const QString& Target, const QString& AppName)
{
    if (SearchList.isEmpty() || Target.isEmpty() || AppName.isEmpty()) {
        return "";
    }

    QFileInfo fi;
    for (QString dir : SearchList) {
        // qDebug().noquote() << DEBUG_PREFIX_BASE << "Search:" << dir;
        dir = BuildPath({ dir, Target });
        fi.setFile(dir);
        if (fi.exists()) {
            if ((Target.isEmpty() && fi.isDir()) || (!Target.isEmpty() && fi.isFile())) {
                // qDebug().noquote() << DEBUG_PREFIX_BASE << "Found:" << dir;
                return QDir::toNativeSeparators(dir);
            }
        }
    }

    return "";
}

/* Public */

/* Path Utils */
QString Utils::BuildPath(std::initializer_list<QString> subs)
{
    QStringList items, tmp;
    bool isRoot = false;

    for (QString sub : subs) {
        sub.replace("\\", "/");
        if (items.isEmpty() && sub.startsWith("/")) {
            isRoot = true;
        }
        if (sub != "/") {
            tmp = sub.split("/");
            tmp.removeAll("");
            items << tmp;
        }
    }

    if (isRoot) {
        items.insert(0, "");
    }

    return QDir::toNativeSeparators(items.join("/"));
}

/* Translation */
static void CleanupTranslator()
{
    while (!translators.isEmpty()) {
        QPointer<QTranslator> translator = translators.takeFirst();
        if (!translator.isNull()) {
            translator->deleteLater();
        }
    }
}

void Utils::InitTranslator(const QStringList &translator_files)
{
    if (!loadded_qt_translator) {
        qAddPostRoutine(CleanupTranslator);

        QStringList qt_translator_files;
        qt_translator_files << QString("qt_%1").arg(QLocale::system().name());
        qt_translator_files << QString("qtbase_%1").arg(QLocale::system().name());
        for (const QString &translator_file : qAsConst(qt_translator_files)) {
            QPointer<QTranslator>translator = new QTranslator();
            translators.append(translator);
            //qDebug().noquote() << DEBUG_PREFIX_BASE << "QMFile:" << translator_file;
            if (translator->load(translator_file, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
                if (!qApp->installTranslator(translator)) {
                    qCritical().noquote() << QString("Install translator %1 failed!").arg(translator_file);
                }
            //} else {
            //    qCritical().noquote() << QString("Load Translations %1 failed!").arg(translator_file);
            }
        }
        loadded_qt_translator = true;
    }

    if (!translator_files.isEmpty()) {
        for (const QString &translator_file : qAsConst(translator_files)) {
            QPointer<QTranslator>translator = new QTranslator();
            translators.append(translator);
            //qDebug().noquote() << DEBUG_PREFIX_BASE << "QMFile:" << translator_file;
            if (translator->load(translator_file)) {
                if (!qApp->installTranslator(translator)) {
                    qCritical().noquote() << QString("Install translator %1 failed!").arg(translator_file);
                }
            //} else {
            //    qCritical().noquote() << QString("Load Translations %1 failed!").arg(translator_file);
            }
        }
    }
}

/* Default paths */
QStringList Utils::GetStdConfDirs(const QString &AppName)
{
    //    Linux                   |   Win
    //  $HOME/.config/{AppName}   | %UserProfile%/.config/{AppName}
    //  /etc/{AppName}            | %AppData%/{AppName}
    //  /usr/share/{AppName}      | %LocalAppData%/{AppName}

    QStringList SearchPaths;
#if defined(Q_OS_LINUX)
    SearchPaths << BuildPath({ env.value("HOME"), ".config", AppName });
    SearchPaths << BuildPath({ "/etc", AppName });
    SearchPaths << BuildPath({ "/usr/share", AppName });
#elif defined(Q_OS_WIN)
    SearchPaths << BuildPath({ env.value("UserProfile"), ".config", AppName });
    SearchPaths << BuildPath({ env.value("AppData"), AppName });
    SearchPaths << BuildPath({ env.value("LocalAppData"), AppName });
#endif

    return SearchPaths;
}

QStringList Utils::GetConfDirs(const QString &AppName)
{
    //    Linux                   |   Win
    //  AppDir                    | AppDir
    //  $HOME/.config/{AppName}   | %UserProfile%/.config/{AppName}
    //  /etc/{AppName}            | %AppData%/{AppName}
    //  /usr/share/{AppName}      | %LocalAppData%/{AppName}

    QStringList SearchPaths(QCoreApplication::applicationDirPath());
#if defined(Q_OS_LINUX)
    SearchPaths << BuildPath({ env.value("HOME"), ".config", AppName });
    SearchPaths << BuildPath({ "/etc", AppName });
    SearchPaths << BuildPath({ "/usr/share", AppName });
#elif defined(Q_OS_WIN)
    SearchPaths << BuildPath({ env.value("UserProfile"), ".config", AppName });
    SearchPaths << BuildPath({ env.value("AppData"), AppName });
    SearchPaths << BuildPath({ env.value("LocalAppData"), AppName });
#endif

    return SearchPaths;
}

QStringList Utils::GetDataDirs(const QString &AppName)
{
    //    Linux              |   Win
    //  AppDir               | AppDir
    //  /usr/share/{AppName} | %AppData%/{AppName}
    //  /opt/{AppName}       | %LocalAppData%/{AppName}

    QStringList SearchPaths(QCoreApplication::applicationDirPath());
#if defined Q_OS_LINUX
    SearchPaths << BuildPath({ "/usr/share", AppName });
    SearchPaths << BuildPath({ "/opt", AppName });
#elif defined Q_OS_WIN
    SearchPaths << BuildPath({ env.value("AppData"), AppName });
    SearchPaths << BuildPath({ env.value("LocalAppData"), AppName });
#endif

    return SearchPaths;
}

QStringList Utils::GetResDir(const QString &AppName)
{
    //    Linux                        |   Win
    //  AppDir/Resources               | AppDir/Resources
    //  /usr/share/{AppName}/Resources | %AppData%/{AppName}/Resources
    //  /opt/{AppName}/Resources       | %LocalAppData%/{AppName}/Resources

    QStringList SearchPaths(BuildPath({ QCoreApplication::applicationDirPath(), "Resources" }));
#if defined Q_OS_LINUX
    SearchPaths << BuildPath({ "/usr/share", AppName, "Resources" });
    SearchPaths << BuildPath({ "/opt", AppName, "Resources" });
#elif defined Q_OS_WIN
    SearchPaths << BuildPath({ env.value("AppData"), AppName, "Resources" });
    SearchPaths << BuildPath({ env.value("LocalAppData"), AppName, "Resources" });
#endif

    return SearchPaths;
}

QString Utils::GetDefaultConfDir()
{
    QStringList dirs = GetConfDirs();
    return dirs.first();
}

QString Utils::GetStdDefaultConfDir()
{
    QStringList dirs = GetStdConfDirs();
    return dirs.first();
}

QString Utils::GetDefaultConfFile(const QString &filename)
{
    QStringList dirs = GetConfDirs();
    QString path = BuildPath({dirs.first(), filename});
    return path;
}

QString Utils::GetStdDefaultConfFile(const QString &filename)
{
    QStringList dirs = GetStdConfDirs();
    QString path = BuildPath({dirs.first(), filename});
    return path;
}

QString Utils::findStdProfile(const QString &Profile, const QString &AppName)
{
    if (AppName.isEmpty() || Profile.isEmpty())
        return "";

    QStringList SearchPaths = GetStdConfDirs(AppName);

    return _findFromDir(SearchPaths, Profile);
}

QString Utils::findProfile(const QString &Profile, const QString &AppName)
{
    if (AppName.isEmpty() || Profile.isEmpty())
        return "";

    QStringList SearchPaths = GetConfDirs(AppName);

    return _findFromDir(SearchPaths, Profile);
}

QString Utils::findFromDataDir(const QString& Target, const QString &AppName)
{
    if (AppName.isEmpty() || Target.isEmpty())
        return "";

    QStringList SearchPaths = GetDataDirs(AppName);

    return _findFromDir(SearchPaths, Target);
}

QString Utils::findFromResDir(const QString& Target, const QString& AppName)
{
    if (AppName.isEmpty() || Target.isEmpty())
        return "";

    QStringList SearchPaths = GetResDir(AppName);

    return _findFromDir(SearchPaths, Target);
}

/* File Utils */
QByteArray Utils::loadFile(const QString &absFile)
{
    QByteArray contents;
    QFile file(absFile);
    if (file.open(QIODevice::ReadOnly)) {
        //qDebug().noquote() << DEBUG_PREFIX_BASE << "Load File:" << QDir::toNativeSeparators(file.fileName());
        contents = file.readAll();
        file.close();
        if (file.error() != QFileDevice::NoError) {
            qCritical().noquote() << QString("Read File [%1] failed! %2").arg(absFile, file.errorString());
        }
    }
    return contents;
}

QString Utils::loadText(const QString &absFile)
{
    return QString(loadFile(absFile));
}

qint64 Utils::saveFile(const QString &absFile, QByteArray contents)
{
    qint64 w_bytes = -1;
    QFileInfo fi(absFile);
    if (fi.exists() && fi.isDir()) {
        qCritical().noquote() << QString("Save File[%s] failed, because file exists and is a directory!").arg(absFile);
        return w_bytes;
    }
    QString absDir = fi.absolutePath();
    QFileInfo di(absDir);
    if (di.exists() && !di.isDir()) {
        qCritical().noquote() << QString("Save File[%1] failed, the file's path exists a non-directory destination!").arg(absFile);
        return w_bytes;
    } else if (!di.exists()) {
        QDir dir;
        if (!dir.mkpath(fi.absolutePath())) {
            qCritical().noquote() << QString("Create Path[%1] failed!").arg(absDir);
            return w_bytes;
        }
    }
    QFile file(absFile);
    if (file.open(QIODevice::WriteOnly)) {
        w_bytes = file.write(contents);
        if (w_bytes >= 0) {
            file.flush();
            qDebug().noquote() << "Write" << w_bytes << "bytes to file:" << absFile;
        } else {
            qCritical().noquote() << QString("Save File[%1] failed! %2").arg(absFile, file.errorString());
        }
        file.close();
    }

    return w_bytes;
}

/* System Utils */
bool Utils::mkdir(const QString &target, bool force)
{
    QDir dir(target);
    QFileInfo fi(target);
    if (fi.exists() && force) {
        if (fi.isDir()) {
            if (!dir.rmdir(target)) return false;
        } else {
            if (!dir.remove(target)) return false;
        }
    }
    return dir.mkpath(target);
}

void Utils::m_sleep(quint32 ms)
{
    if (ms > 0) {
#if defined (Q_OS_WIN)
        Sleep(ms);
#else
        usleep(ms * 1000);
#endif
    }
}

/* Storage capacity conversion (human readable) */
QByteArray Utils::SizeToHumanReadable(long double size, bool base_1024)
{
    quint32 base = base_1024 ? 1024 : 1000;
    const char units[] = { 0, 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
    const int exponent_max = sizeof(units) - 1;
    long double e = 1;
    int exponent = 0;

    while (e * base <= size && exponent < exponent_max) {
        e *= base;
        exponent++;
    }

    size /= e;
    const char *suffix = (base_1024 && exponent) ? "iB" : "B";

    return QString::asprintf("%.2Lf %c%s", size, units[exponent], suffix).toUtf8();
}

const QStringList Utils::ConvStringList(const char ** array)
{
    QStringList list;
    const char** tmp = array;
    while (*tmp != nullptr) {
        list << *(tmp++);
    }
    return list;
}
