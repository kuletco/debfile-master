#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class Type {
        deb,
        udeb,
        invalid = -1,
    };
    Q_ENUM(Type)

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
    Q_ENUM(Architecture)

    enum class Priority {
        required,
        standard,
        optional,
        extra,
        invalid = -1,
    };
    Q_ENUM(Priority)

    enum class Section {
        database,
        editors,
        electronics,
        embedded,
        fonts,
        games,
        graphics,
        interpreters,
        mail,
        misc,
        net,
        news,
        science,
        sound,
        text,
        utils,
        video,
        web,
        invalid = -1,
    };
    Q_ENUM(Section)

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyReleaseEvent(QKeyEvent *e);

private:
    void initTempDir();
    bool updateInfo();
    bool genControlFile();
    bool genScriptFile(const QString &scriptfile, const QString &contents);

private slots:
    void worker_started();
    void worker_finished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    void worker_error(QProcess::ProcessError error);

    // UI Slots
    void on_PB_Build_clicked();

private:
    Ui::MainWindow *ui;

    QProcess *m_worker;

    QString m_tempdir;
    QString m_debiandir;
    QString m_controlfile;
    QString m_filename;

    QString m_name;
    QString m_version;
    QString m_maintainer;
    QString m_homepage;
    QString m_summary;
    QString m_description;
    QString m_depends;
    QString m_predepends;
    QString m_conflicts;
    QString m_replaces;
    QString m_provides;
    bool m_protected;
    Type m_type;
    Architecture m_architecture;
    Priority m_priority;
    Section m_section;

    QString m_contents_preinst;
    QString m_contents_postinst;
    QString m_contents_prerm;
    QString m_contents_postrm;
};
#endif // MAINWINDOW_H
