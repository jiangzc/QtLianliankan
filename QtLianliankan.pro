#-------------------------------------------------
#
# Project created by QtCreator 2016-07-11T10:49:28
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtLianliankan
TEMPLATE = app


SOURCES += main.cpp\
        main_game_window.cpp \
    game_model.cpp \
    startdialog.cpp

HEADERS  += main_game_window.h \
    game_model.h \
    startdialog.h

FORMS    += main_game_window.ui \
    startdialog.ui

RESOURCES += \
    res.qrc

DISTFILES += \
    res/image/background/DrawCrowd.jpeg \
    res/image/background/仙侠1.jpg \
    res/image/background/仙侠2.jpg \
    res/image/background/仙侠3.jpg \
    res/image/background/仙侠4.jpg \
    res/image/background/仙侠5.jpg \
    res/image/background/仙侠6.jpg \
    res/image/background/仙侠7.jpg \
    res/image/background/仙侠8.jpg \
    res/image/background/仙侠9.jpg \
    res/image/background/剑灵1.jpg \
    res/image/background/仙侠10.png \
    res/image/youtube-8.png \
    res/image/youtube-6.png \
    res/image/youtube-5.png \
    res/image/youtube-2.png
