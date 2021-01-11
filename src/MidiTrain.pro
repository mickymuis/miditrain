<F10># QMake Project file
CONFIG += c++11

QT = core gui widgets network
CONFIG += console debug
TEMPLATE = app
TARGET = MidiTrain

include(../externals/QMidi/src/QMidi.pri)


SOURCES += $$PWD/main.cpp \
	$$PWD/mainwindow.cpp \
	$$PWD/scorewidget.cpp \
        $$PWD/composition.cpp \
        $$PWD/playhead.cpp \
        $$PWD/playthread.cpp \
        $$PWD/eventqueue.cpp

HEADERS += $$PWD/miditrain.h \
        $$PWD/mainwindow.h \
	$$PWD/scorewidget.h \
        $$PWD/composition.h \
        $$PWD/playhead.h \
        $$PWD/playthread.h \
        $$PWD/eventqueue.h

