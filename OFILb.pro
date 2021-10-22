QT       += core gui xml sql winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/clifp.cpp \
    src/fe-data.cpp \
    src/fe-install.cpp \
    src/flashpoint/fp-db.cpp \
    src/flashpoint/fp-install.cpp \
    src/flashpoint/fp-items.cpp \
    src/flashpoint/fp-json.cpp \
    src/flashpoint/fp-macro.cpp \
    src/import-worker.cpp \
    src/launchbox/lb-install.cpp \
    src/launchbox/lb-xml.cpp \
    src/launchbox/lb-items.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/clifp.h \
    src/fe-data.h \
    src/fe-install.h \
    src/flashpoint/fp-db.h \
    src/flashpoint/fp-install.h \
    src/flashpoint/fp-items.h \
    src/flashpoint/fp-json.h \
    src/flashpoint/fp-macro.h \
    src/import-worker.h \
    src/launchbox/lb-install.h \
    src/launchbox/lb-xml.h \
    src/launchbox/lb-items.h \
    src/mainwindow.h \
    src/version.h

FORMS += \
    res/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_FILE = resources.rc

LIBS += Version.lib # TODO: See if this can be removed with a static build of QxW

contains(QT_ARCH, i386) {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static32_0-0-7-9_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static32_0-0-7-9_Qt_5-15-2d
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static64_0-0-7-9_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static64_0-0-7-9_Qt_5-15-2d
}

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

contains(QT_ARCH, i386) {
    win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static32_0-0-7-9_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static32_0-0-7-9_Qt_5-15-2d.lib
} else {
    win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static64_0-0-7-9_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static64_0-0-7-9_Qt_5-15-2d.lib
}

RESOURCES += \
    resources_cmn.qrc

contains(QT_ARCH, i386) {
RESOURCES += \
    resources_32.qrc
} else {
RESOURCES += \
    resources_64.qrc
}
