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
    src/flashpoint.cpp \
    src/flashpointinstall.cpp \
    src/importworker.cpp \
    src/launchbox.cpp \
    src/launchboxinstall.cpp \
    src/launchboxxml.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/flashpoint.h \
    src/flashpointinstall.h \
    src/importworker.h \
    src/launchbox.h \
    src/launchboxinstall.h \
    src/launchboxxml.h \
    src/mainwindow.h \
    src/version.h

FORMS += \
    res/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_FILE = resources.rc

LIBS += Version.lib # TODO: See if this can be removed with a static build of Qx

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-2-11_Qt_5-15-0
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-2-11_Qt_5-15-0d

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-2-11_Qt_5-15-0.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-2-11_Qt_5-15-0d.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-2-11_Qt_5-15-0.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-2-11_Qt_5-15-0d.lib

RESOURCES += \
    resources.qrc
