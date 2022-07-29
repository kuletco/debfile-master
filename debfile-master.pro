VERSION = 1.1.2

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += c++14
QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS += -Wno-deprecated-copy
linux {
    load(ccache)
    # suppress the default RPATH if you wish
    QMAKE_LFLAGS_RPATH =
    # add your own with quoting gyrations to make sure $ORIGIN gets to the command line unexpanded
    QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"
}

DEFINES += APP_VERSION="\\\"$${VERSION}\\\""

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DESTDIR = $${PWD}
BUILDDIR = $${DESTDIR}/.tmp/$${TARGET}

CONFIG(debug, debug|release) {
    win32: BUILDDIR = $${BUILDDIR}/win32-debug
    linux: BUILDDIR = $${BUILDDIR}/linux-debug
} else {
    win32: BUILDDIR = $${BUILDDIR}/win32-release
    linux: BUILDDIR = $${BUILDDIR}/linux-release
}

MOC_DIR += $${BUILDDIR}
OBJECTS_DIR += $${BUILDDIR}/obj
UI_DIR += $${BUILDDIR}
RCC_DIR += $${BUILDDIR}

INCLUDEPATH += $${UI_DIR}

SOURCES += \
    FileSystemModel.cpp \
    FileSystemWorkThread.cpp \
    main.cpp \
    signature.cpp \
    utils.cpp \
    MainWindow.cpp \
    DEBFile.cpp

HEADERS += \
    FileSystemModel.h \
    FileSystemWorkThread.h \
    signature.h \
    utils.h \
    MainWindow.h \
    DEBFile.h

FORMS += \
    MainWindow.ui

TRANSLATIONS += \
    debfile-master.zh_CN.ts

RESOURCES += \
    debfile-master.qrc

OTHER_FILES += \
    logo.png \
    debfile-master.desktop \
    build_deb.sh \
    debian/control

DEBIAN.target = deb
DEBIAN.commands = $$PWD/build_deb.sh

QMAKE_EXTRA_TARGETS += DEBIAN

# Update Translations
QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input         = TRANSLATIONS
lrelease.output        = ${QMAKE_FILE_BASE}.qm
lrelease.commands      = $$[QT_HOST_BINS]/lrelease ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
lrelease.CONFIG       += no_link target_predeps
