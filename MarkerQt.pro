TEMPLATE = app
TARGET = MarkerQt
DEPENDPATH += .
INCLUDEPATH += .
#CONFIG += rtti exceptions
#QMAKE_CFLAGS += -O3
#QMAKE_CXXFLAGS += -std=c++11 -O3
QMAKE_CXXFLAGS += -D_UNICODE
CONFIG += c++14
win32-g++ {
	QMAKE_LFLAGS += -Wl,--dynamicbase -Wl,--nxcompat
}

#! [0]
RESOURCES = MarkerQt.qrc
#! [0]

# Input
HEADERS += base.h \
    mainwindow.h \
    renderarea.h \
    renderthread.h \
    worker.h \
    ffmpegdriver.h \
    videostream.h \
    cornergrabber.h

SOURCES += mainwindow.cpp \
    renderarea.cpp \
    renderthread.cpp \
    worker.cpp \
    markerqt.cpp \
    ffmpegdriver.cpp \
    videostream.cpp \
    cornergrabber.cpp

QT += widgets

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../dlib-19.7/build/vc2017/dlib/release/ -ldlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../dlib-19.7/build/vc2017/dlib/debug/ -ldlib

INCLUDEPATH += $$PWD/../dlib-19.7
DEPENDPATH += $$PWD/../dlib-19.7

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../opencv-3.3.0/build/vc2017/lib/release/ -lopencv_core330 -lopencv_imgproc330
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../opencv-3.3.0/build/vc2017/lib/debug/ -lopencv_core330d -lopencv_imgproc330d

INCLUDEPATH += $$PWD/../opencv-3.3.0/include
INCLUDEPATH += $$PWD/../opencv-3.3.0/build/vc2017
INCLUDEPATH += $$PWD/../opencv-3.3.0/modules/core/include
INCLUDEPATH += $$PWD/../opencv-3.3.0/modules/imgcodecs/include
INCLUDEPATH += $$PWD/../opencv-3.3.0/modules/imgproc/include
INCLUDEPATH += "D:/Program Files/ffmpeg/bin"
DEPENDPATH += $$PWD/../opencv-3.3.0/include
