#ifndef DEBFILE_H
#define DEBFILE_H

#include <QObject>
#include <QPointer>
#include <QFile>
#include <QProcess>

namespace DEBAttrs
{
    Q_NAMESPACE

    enum class Type {
        deb,
        udeb,
        invalid = -1,
    };
    Q_ENUM_NS(Type)

    enum class Architecture {
        all,
        amd64,
        i386,
        arm64,
        armel,
        arm,
        powerpc64,
        powerpc,
        mips64el,
        mips,
        invalid = -1,
    };
    Q_ENUM_NS(Architecture)

    enum class Priority {
        required,
        standard,
        optional,
        extra,
        invalid = -1,
    };
    Q_ENUM_NS(Priority)

    enum class Section {
        admin,
        comm,
        database,
        debug,
        devel,
        doc,
        editors,
        education,
        electronics,
        embedded,
        fonts,
        games,
        graphics,
        interpreters,
        kernel,
        libdevel,
        libs,
        localization,
        mail,
        math,
        metapackages,
        misc,
        net,
        news,
        otherosfs,
        science,
        shells,
        sound,
        text,
        translations,
        utils,
        vcs,
        video,
        web,
        invalid = -1,
    };
    Q_ENUM_NS(Section)

    enum class CompressTyp {
        zstd,
        xz,
        gzip,
        none,
        invalid = -1,
    };
    Q_ENUM_NS(CompressTyp)
} // namespace DEBAttrs

class DEBFile : public QObject
{
    Q_OBJECT
public:

    QString m_name;
    QString m_version;
    QString m_maintainer;
    QString m_homepage;
    QString m_summary;
    QString m_description;
    QString m_predepends;
    QString m_depends;
    QString m_recommends;
    QString m_suggests;
    QString m_conflicts;
    QString m_replaces;
    QString m_provides;
    quint64 m_installed_size;
    bool m_protected;
    DEBAttrs::Type m_type;
    DEBAttrs::Architecture m_architecture;
    DEBAttrs::Priority m_priority;
    DEBAttrs::Section m_section;
    DEBAttrs::CompressTyp m_compresstype;

    QString m_contents_control;
    QString m_contents_preinst;
    QString m_contents_postinst;
    QString m_contents_prerm;
    QString m_contents_postrm;

    explicit DEBFile(QObject *parent = nullptr);
    ~DEBFile();

    qint32 error() const { return m_error; }
    const QString &errStr() const { return m_errstr; }

    const QString &buildroot();
    const QString &filename();

    qint64 CreateControlFile();
    qint64 CreateScriptFiles();

    void ClearBuildDir();
    void CreateBuildDir(bool force = false);
    void CreatePackage(const QString &debfile);

protected:
    qint64 CreateTextFile(const QString &file, QFileDevice::Permissions permission, const QString &contents);
    qint64 CreateScriptFile(const QString &script_file, const QString &contents);

private slots:
    void worker_started();
    void worker_finished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    void worker_error(QProcess::ProcessError error);

signals:
    void work_started();
    void work_finished();
    void work_failed();
    void work_update(QString info);
    void buildroot_cleared();
    void buildroot_changed(QString buildroot);

private:
    qint32 m_error;
    QString m_errstr;

    QString m_buildroot;
    QString m_filename;
    QString m_file;

    QPointer<QProcess> m_worker;
};

#endif // DEBFILE_H
