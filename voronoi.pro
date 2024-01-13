#-------------------------------------------------
#
# Project created by QtCreator 2018-03-06T15:03:17
#
#-------------------------------------------------

QT       += core gui opengl openglextensions
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = voronoi
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32: LIBS += opengl32.lib

SOURCES += \
    bounds2d.cpp \
    cglwidget.cpp \
    dxf.cpp \
        main.cpp \
        dialog.cpp \
    clipper.cpp \
    svg.cpp

HEADERS += \
    bounds2d.h \
    cglwidget.h \
        dialog.h \
    dxf.h \
    jc_voronoi.h \
    clipper.h \
    point.h \
    polysegs.h \
    seg2f.h \
    spline.h \
    svg.h \
    utils.h

FORMS += \
        dialog.ui

RESOURCES += \
    res.qrc
