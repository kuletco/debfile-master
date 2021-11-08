#ifndef UTILS_H
#define UTILS_H

#include <QtGlobal>
#include <QCoreApplication>
#include <QMetaType>
#include <QMetaEnum>
#include <QPointer>
#include <QObject>
#include <QHash>
#include <QMutex>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QDebug>

#include <QTimer>
#include <time.h>
#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <pthread.h>
#include <cxxabi.h>


#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef ReleasePtr
#define ReleasePtr(x)   \
    if (x) {            \
        delete x;       \
        x = nullptr;    \
    }
#endif
#ifndef ReleasePtrWith
#define ReleasePtrWith(x,func)  \
    if (x) {                    \
        func(x);                \
        x = nullptr;            \
    }
#endif

#ifndef RegisterMetaTypes
#define RegisterMetaTypes(X) {\
    qRegisterMetaType<X>(#X); \
    qRegisterMetaType<X>(#X"&"); \
    qRegisterMetaType< QVector<X> >("QVector<"#X">"); \
    qRegisterMetaType< QVector<X> >("QVector<"#X">&"); \
}
#endif

#ifndef RegisterMetaTypeStreamOperators
#define RegisterMetaTypeStreamOperators(X) {\
    qRegisterMetaTypeStreamOperators<X>(#X); \
    qRegisterMetaTypeStreamOperators<X>(#X"&"); \
    qRegisterMetaTypeStreamOperators< QVector<X> >("QVector<"#X">"); \
    qRegisterMetaTypeStreamOperators< QVector<X> >("QVector<"#X">&"); \
}
#endif

#ifndef CC_CALLBACK
#define CC_CALLBACK
#define CC_CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define CC_CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define CC_CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define CC_CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)
#endif

#ifndef L_MOVE
#define L_MOVE(m,n) ((m)<<(n))
#endif
#ifndef R_MOVE
#define R_MOVE(m,n) ((m)>>(n))
#endif

// Utils
class Utils : public QObject
{
    Q_OBJECT
public:
    Utils(QObject* parent = nullptr) : QObject(parent) {};
    ~Utils() {};

    /* Path Utils */
    static QString BuildPath(std::initializer_list<QString> subs);

    /* Translation */
    static void InitTranslator(const QStringList &translator_files = QStringList());

    /* Default paths */
    static QStringList GetStdConfDirs(const QString &AppName=QCoreApplication::applicationName());
    static QStringList GetConfDirs(const QString &AppName=QCoreApplication::applicationName());
    static QStringList GetDataDirs(const QString &AppName=QCoreApplication::applicationName());
    static QStringList GetResDir(const QString &AppName=QCoreApplication::applicationName());

    static QString GetDefaultConfDir();
    static QString GetStdDefaultConfDir();
    static QString GetDefaultConfFile(const QString &filename);
    static QString GetStdDefaultConfFile(const QString &filename);

    static QString findStdProfile(const QString& Profile, const QString &AppName=QCoreApplication::applicationName());
    static QString findProfile(const QString& Profile, const QString &AppName=QCoreApplication::applicationName());
    static QString findFromDataDir(const QString& Target, const QString &AppName=QCoreApplication::applicationName());
    static QString findFromResDir(const QString& Target, const QString &AppName=QCoreApplication::applicationName());

    /* File Utils */
    static QByteArray loadFile(const QString &absFile);
    static QString loadText(const QString &absFile);
    static qint64 saveFile(const QString &absFile, QByteArray contents);

    /* System Utils */
    static bool mkdir(const QString &target, bool force=true);
    static void m_sleep(quint32 ms);

    /* Enum Convertor */
    /* NOTE: When you want to use this convertor, your enum must have a menber hold the -1 */
    template <typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = true >
    static T EnumConvert(const char* t) {
        Q_STATIC_ASSERT_X(QtPrivate::IsQEnumHelper<T>::Value, "Type is not a enumerator or not declared with Q_ENUM, Q_ENUM_NS, Q_FLAG or Q_FLAG_NS");
        QMetaEnum convertor = QMetaEnum::fromType<T>();
        return static_cast<T>(convertor.keyToValue(t));
    }
    template <typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = true >
    static T EnumConvert(const QString &t) {
        Q_STATIC_ASSERT_X(QtPrivate::IsQEnumHelper<T>::Value, "Type is not a enumerator or not declared with Q_ENUM, Q_ENUM_NS, Q_FLAG or Q_FLAG_NS");
        QMetaEnum convertor = QMetaEnum::fromType<T>();
        return static_cast<T>(convertor.keyToValue(t.toUtf8()));
    }
    template <typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = true >
    static QByteArray EnumConvert(T t) {
        Q_STATIC_ASSERT_X(QtPrivate::IsQEnumHelper<T>::Value, "Type is not a enumerator or not declared with Q_ENUM, Q_ENUM_NS, Q_FLAG or Q_FLAG_NS");
        QMetaEnum convertor = QMetaEnum::fromType<T>();
        return convertor.valueToKey(static_cast<int>(t));
    }

    /* Storage capacity conversion (human readable) */
    static QByteArray SizeToHumanReadable(long double size, bool base_1024 = true);

    /* Char*[] to QStringList */
    static const QStringList ConvStringList(const char **array);

private:
    static QString _findFromDir(const QStringList& SearchList, const QString& Profile = "", const QString& AppName = QCoreApplication::applicationName());
};

#ifndef RetryFunction
#define RetryFunction(Func, nRetry) { int n_retry = nRetry; while (n_retry-- > 0) { if (Func()) { break; } Utils::m_sleep(500); } if (n_retry <= 0) return; }
#endif

#endif // UTILS_H
