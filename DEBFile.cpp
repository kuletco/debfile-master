#include <QDebug>
#include <QFileDialog>

#include "utils.h"
#include "DEBFile.h"

DEBFile::DEBFile(QObject *parent) : QObject{parent}, m_worker{new QProcess()}
{
    m_protected = false;
    m_type = DEBAttrs::Type::deb;
    m_architecture = DEBAttrs::Architecture::all;
    m_priority = DEBAttrs::Priority::standard;
    m_section = DEBAttrs::Section::misc;

    connect(m_worker, SIGNAL(started()), this, SLOT(worker_started()));
    connect(m_worker, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(worker_finished(int,QProcess::ExitStatus)));
    connect(m_worker, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(worker_error(QProcess::ProcessError)));
}

DEBFile::~DEBFile()
{
    ClearBuildDir();

    if (m_worker->state() != QProcess::NotRunning) {
        m_worker->terminate();
        if (m_worker->state() != QProcess::NotRunning) {
            m_worker->kill();
        }
    }
    delete m_worker;
}

const QString &DEBFile::buildroot()
{
    if (m_buildroot.isEmpty()) {
        CreateBuildDir();
    }

    return m_buildroot;
}

const QString &DEBFile::filename()
{
    if (m_filename.isEmpty()) {
        m_filename = QString("%1_%2_%3.%4").arg(m_name, m_version, Utils::EnumConvert(m_architecture), Utils::EnumConvert(m_type));
    }

    return m_filename;
}

void DEBFile::ClearBuildDir()
{
    if (!m_buildroot.isEmpty()) {
        QDir dir(m_buildroot);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
}

void DEBFile::CreateBuildDir(bool force)
{
    if (m_buildroot.isEmpty()) {
        char tempname[] = "DEBTEMP.XXXXXX";
        if (!mkdtemp(tempname)) {
            qWarning().noquote() << "Cannot create a uniquely-named tempdir!";
        }

        QDir dir(tempname);
        dir.mkpath("DEBIAN");

        m_buildroot = dir.absolutePath();
    } else if (force) {
        ClearBuildDir();
        CreateBuildDir();
    }
//    if (!force) {
//        qDebug().noquote() << "BuildDir:" << m_buildroot;
//    }
}

qint64 DEBFile::CreateTextFile(const QString &file, QFile::Permissions permission, const QString &contents)
{
    qint64 n_write = 0;
    QFile _file(file);
    if (!_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        goto CreateFileError;
    }

    n_write = _file.write(contents.toUtf8());
    if (n_write < 0) {
        goto CreateFileError;
    }
    qDebug().noquote() << "Write:" << n_write << "Bytes to" << file;

    if (!_file.setPermissions(permission)) {
        goto CreateFileError;
    }

    if (!_file.flush()) {
        goto CreateFileError;
    }
    _file.close();

    return m_error;

CreateFileError:
    m_error = _file.error();
    m_errstr = _file.errorString();
    qCritical().noquote() << "Error:" << m_error << " | " << m_errstr;
    emit work_failed(m_errstr);

    return m_error;
}

qint64 DEBFile::CreateScriptFile(const QString &script_file, const QString &contents)
{
    QFile::Permissions permission = QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ExeUser | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther;

    QString writeable_contents = contents;
    if (!writeable_contents.endsWith("\n")) {
        writeable_contents.append('\n');
    }

    return CreateTextFile(script_file, permission, writeable_contents.toUtf8());
}

qint64 DEBFile::CreateControlFile()
{
    QFile::Permissions permission = QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ReadGroup | QFileDevice::ReadOther;

    QStringList contents;
    contents << QString("Package: %1").arg(m_name);
    contents << QString("Version: %1").arg(m_version);
    contents << QString("Size: 100");
    if (m_architecture != DEBAttrs::Architecture::invalid)
        contents << QString("Architecture: %1").arg(Utils::EnumConvert(m_architecture).constData());
    if (m_section != DEBAttrs::Section::invalid)
        contents << QString("Section: %1").arg(Utils::EnumConvert(m_section).constData());
    if (m_priority != DEBAttrs::Priority::invalid)
        contents << QString("Priority: %1").arg(Utils::EnumConvert(m_priority).constData());
    if (m_protected)
        contents << QString("Protected: yes");
    if (!m_maintainer.isEmpty())
        contents << QString("Maintainer: %1").arg(m_maintainer);
    if (!m_homepage.isEmpty())
        contents << QString("Homepage: %1").arg(m_homepage);
    if (!m_predepends.isEmpty())
        contents << QString("Pre-Depends: %1").arg(m_predepends);
    if (!m_depends.isEmpty())
        contents << QString("Depends: %1").arg(m_depends);
    if (!m_conflicts.isEmpty())
        contents << QString("Conflicts: %1").arg(m_conflicts);
    if (!m_replaces.isEmpty())
        contents << QString("Replaces: %1").arg(m_replaces);
    if (!m_provides.isEmpty())
        contents << QString("Provides: %1").arg(m_provides);
    if (!m_summary.isEmpty())
        contents << QString("Description: %1").arg(m_summary);
    // TODO: Need format long-description contents (https://manpages.debian.org/testing/dpkg-dev/deb-control.5.en.html#Description:)
    if (!m_description.isEmpty())
        contents << QString(" %1").arg(m_description);

    contents << "";
    m_contents_control = contents.join("\n").toUtf8();

    return CreateTextFile(Utils::BuildPath({m_buildroot, "DEBIAN", "control"}), permission, m_contents_control.toUtf8());
}

qint64 DEBFile::CreateScriptFiles()
{
    if (!m_contents_preinst.isEmpty() && !CreateScriptFile(Utils::BuildPath({m_buildroot, "DEBIAN", "preinst"}), m_contents_preinst.toUtf8())) {
        return m_error;
    }
    if (!m_contents_postinst.isEmpty() && !CreateScriptFile(Utils::BuildPath({m_buildroot, "DEBIAN", "postinst"}), m_contents_postinst.toUtf8())) {
        return m_error;
    }
    if (!m_contents_prerm.isEmpty() && !CreateScriptFile(Utils::BuildPath({m_buildroot, "DEBIAN", "prerm"}), m_contents_prerm.toUtf8())) {
        return m_error;
    }
    if (!m_contents_postrm.isEmpty() && !CreateScriptFile(Utils::BuildPath({m_buildroot, "DEBIAN", "postrm"}), m_contents_postrm.toUtf8())) {
        return m_error;
    }

    return m_error;
}

void DEBFile::CreatePackage(const QString &debfile)
{
    m_file = debfile;
    QStringList args;
    args << "/usr/bin/dpkg-deb" << "--build" << m_buildroot << m_file;
    m_worker->start("/usr/bin/fakeroot", args);
}

void DEBFile::worker_started()
{
    qDebug().noquote() << m_worker->program() << m_worker->arguments();
    emit work_started(tr("Building..."));
}

void DEBFile::worker_finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        emit work_finished(tr("Done. File saved to ") + m_file);
    } else {
        QString errstr;
        if (exitStatus != QProcess::NormalExit) {
            errstr = m_worker->errorString().trimmed();
        }
        if (errstr.isEmpty()) {
            errstr = m_worker->readAllStandardError().trimmed();
            if (errstr.isEmpty()) {
                errstr = m_worker->readAllStandardOutput().trimmed();
            }
        }
        qCritical().noquote() << "Exit:" << exitCode << exitStatus << "Error:" << errstr;
        m_errstr = tr("Error: ") + errstr;
        emit work_failed(m_errstr);
    }
    this->ClearBuildDir();
}

void DEBFile::worker_error(QProcess::ProcessError error)
{
    qCritical().noquote() << "Error:" << error << m_worker->errorString();
    m_errstr = tr("Error: ") + m_worker->errorString();
    emit work_failed(m_errstr);
}
