QT       += core gui axcontainer
QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

#INCLUDEPATH += CppIncludes_mystk
#LIBS += -L"D:\Program Files (x86)\AGI\STK 9\lib" \
#        -lAGI.STKUtil.Interop \
#        -lAGI.STKX.Interop \
#        -lstkobjects \
#        -lstkgraphics \
#        -lstkutil \
#        -lstkx

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    PacketProcess.cpp \
    QSTKEarth.cpp \
    QT_STK.cpp \
    TimeManager.cpp \
    UdpProcess.cpp \
    main.cpp \
    stk.cpp

HEADERS += \
    PacketDefine.h \
    PacketHandlerClass.h \
    QSTKEarth.h \
    QT_STK.h \
    TimeManager.h \
    stk.h

FORMS += \
    QT_STK.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Icon.qrc
