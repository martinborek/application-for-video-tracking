
# FILE avwriter.cpp
# AUTOR Martin Borek (mborekcz@gmail.com)
# DATE May, 2015

QT += core gui
QT += multimedia
QT += multimediawidgets
QT += help

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++0x

CONFIG(debug, debug|release) {
    message(Creating DEBUG version)
    #QMAKE_CXXFLAGS += /MTd
}

CONFIG(release, debug|release) {
    message(Creating RELEASE version)
    QMAKE_CXXFLAGS += -D NDEBUG # disable assert
    #QMAKE_CXXFLAGS += /MT
    DEFINES += QT_NO_DEBUG_OUTPUT # disable qDebug()
}



TARGET = videoanonymizer
TEMPLATE = app

INCLUDEPATH += $$PWD/3rd_party
INCLUDEPATH += $$PWD/headers
INCLUDEPATH += $$PWD/sources
INCLUDEPATH += $$PWD/forms
INCLUDEPATH += $$PWD/languages
INCLUDEPATH += $$PWD/help

SOURCES += \
    sources/avwriter.cpp \
    sources/colors.cpp \
    sources/ffmpegplayer.cpp \
    sources/imagelabel.cpp \
    sources/main.cpp \
    sources/mainwindow.cpp \
    sources/objectshape.cpp \
    sources/playerslider.cpp \
    sources/timelabel.cpp \
    sources/trackedobject.cpp \
    sources/trackingalgorithm.cpp \
    sources/videoframe.cpp \
    sources/videotracker.cpp \
    sources/videowidget.cpp \
    sources/anchoritem.cpp \
    sources/trajectoryitem.cpp \
    sources/helpbrowser.cpp

HEADERS += \
    headers/avwriter.h \
    headers/colors.h \
    headers/ffmpegplayer.h \
    headers/imagelabel.h \
    headers/mainwindow.h \
    headers/objectshape.h \
    headers/playerslider.h \
    headers/selection.h \
    headers/timelabel.h \
    headers/trackedobject.h \
    headers/trackingalgorithm.h \
    headers/videoframe.h \
    headers/videotracker.h \
    headers/videowidget.h \
    headers/anchoritem.h \
    headers/trajectoryitem.h \
    headers/helpbrowser.h

FORMS +=  forms/mainwindow.ui

TRANSLATIONS += languages/video_anonymizer_cs.ts


unix {

message(Platform: unix)

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

CONFIG += link_pkgconfig # Only with unix/linux?
PKGCONFIG += opencv

INCLUDEPATH += /usr/include/ffmpeg

#PKGCONFIG += ffmpeg
#OPENCV_PATH = /ucr/include/opencv
#OPENCV_PATH = /ucr/include/opencv2
#INCLUDEPATH += $$OPENCV_PATH

LIBS +=  \
    #-lopencv\
    -lopencv_core \
    -lopencv_highgui \
    -lopencv_imgproc \
    -lopencv_video \
\
    -lavdevice \
    -lavformat \
    -lavfilter \
    -lavcodec \
    -lswresample \
    -lswscale \
    -lavutil \
\
    -lm \
    -lz \
    -lpthread \
    -lpostproc
}

win32 { #MSVC compiler

message(Platform: win32)

INCLUDEPATH += $$PWD/3rd_party/ffmpeg_win32
INCLUDEPATH += $$PWD/3rd_party/opencv_win32
INCLUDEPATH += $$PWD/3rd_party/opencv_win32/opencv

LIBS += -L$$PWD/lib_win32/opencv \
    -lopencv_core2411 \
    -lopencv_highgui2411 \
    -lopencv_imgproc2411 \
    -lopencv_video2411 \
\
    -L$$PWD/lib_win32/ffmpeg \
    -lavdevice \
    -lavformat \
    -lavfilter \
    -lavcodec \
    -lswresample \
    -lswscale \
    -lavutil \

RC_FILE = $$PWD/icon/app.rc # loads the application icon
}

RESOURCES += \
    languages/resource.qrc

# translating: "lupdate" and "lrelease"

# for dynamic QT libraries, execute: "$path-to-qt\bin\qtenv2.bat" and "path-to-qt\bin\windeployqt $path-to-my-app/app.exe"

#D:\Qt\5.4\msvc2013_opengl\bin\qcollectiongenerator.exe C:\video_anonymizer\help\cs_helpcollection.qhcp -o C:\video_anonymizer\help\help_cs.qhc
#D:\Qt\5.4\msvc2013_opengl\bin\qcollectiongenerator.exe C:\video_anonymizer\help\en_helpcollection.qhcp -o C:\video_anonymizer\help\help_en.qhc
