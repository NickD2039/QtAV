TEMPLATE = lib

CONFIG(debug, debug|release) {
    macx:TARGET = QtAV_debug
    !macx {   
        TARGET = QtAVd
        }
}
CONFIG(release, debug|release) {
    TARGET = QtAV
}

CONFIG +=  warn_on debug_and_release x86
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets
}

VPATH = ../src
win32: VPATH *= ../src/compat/msvc

RESOURCES += ../i18n/QtAV.qrc \
    shaders/shaders.qrc

win32 {
    #RC_FILE = $${PROJECTROOT}/res/QtAV.rc
    VPATH *= ../src/compat/msvc
}

OTHER_FILES += $$RC_FILE

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += compat/msvc
}

DEFINES += __STDC_CONSTANT_MACROS
LIBS += -lavcodec -lavformat -lavutil -lswscale
CONFIG *= config_avfilter config_swresample
config_avfilter {
    DEFINES += QTAV_HAVE_AVFILTER=1
    SOURCES += LibAVFilter.cpp
    HEADERS += QtAV/LibAVFilter.h
    android: LIBS += -lavfilter-4
    else: LIBS += -lavfilter
}
config_swresample {
    DEFINES += QTAV_HAVE_SWRESAMPLE=1
    SOURCES += AudioResamplerFF.cpp
    android: LIBS += -lswresample-0
    else: LIBS += -lswresample
}
config_avresample {
    DEFINES += QTAV_HAVE_AVRESAMPLE=1
    SOURCES += AudioResamplerLibav.cpp
    android: LIBS += -lavresample-1
    else: LIBS += -lavresample
}
#config_portaudio {
#    SOURCES += AudioOutputPortAudio.cpp
#    DEFINES *= QTAV_HAVE_PORTAUDIO=1
#    LIBS *= -lportaudio
#    #win32: LIBS *= -lwinmm #-lksguid #-luuid
#}
SOURCES += \
    QtAV_Compat.cpp \
    QtAV_Global.cpp \
    utils/GPUMemCopy.cpp \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioFormat.cpp \
    AudioFrame.cpp \
    AudioOutput.cpp \
    AudioOutputTypes.cpp \
    AudioResampler.cpp \
    AudioResamplerTypes.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    ColorTransform.cpp \
    Frame.cpp \
    Filter.cpp \
    FilterContext.cpp \
    FilterManager.cpp \
    GraphicsItemRenderer.cpp \
    ImageConverter.cpp \
    ImageConverterFF.cpp \
    QPainterRenderer.cpp \
    OSD.cpp \
    OSDFilter.cpp \
    Packet.cpp \
    AVError.cpp \
    AVPlayer.cpp \
    VideoCapture.cpp \
    VideoFormat.cpp \
    VideoFrame.cpp \
    VideoRenderer.cpp \
    VideoRendererTypes.cpp \
    VideoOutput.cpp \
    VideoOutputEventFilter.cpp \
    WidgetRenderer.cpp \
    AVOutput.cpp \
    OutputSet.cpp \
    AVClock.cpp \
    Statistics.cpp \
    VideoDecoder.cpp \
    VideoDecoderTypes.cpp \
    VideoDecoderFFmpeg.cpp \
    VideoDecoderFFmpegHW.cpp \
    VideoThread.cpp \
    QAVIOContext.cpp \
    CommonTypes.cpp

SDK_HEADERS *= \
    QtAV/QtAV.h \
    QtAV/dptr.h \
    QtAV/QtAV_Global.h \
    QtAV/AudioResampler.h \
    QtAV/AudioResamplerTypes.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioFormat.h \
    QtAV/AudioFrame.h \
    QtAV/AudioOutput.h \
    QtAV/AudioOutputTypes.h \
    QtAV/AVDecoder.h \
    QtAV/AVDemuxer.h \
    QtAV/BlockingQueue.h \
    QtAV/CommonTypes.h \
    QtAV/Filter.h \
    QtAV/FilterContext.h \
    QtAV/Frame.h \
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageConverter.h \
    QtAV/ImageConverterTypes.h \
    QtAV/QPainterRenderer.h \
    QtAV/OSD.h \
    QtAV/OSDFilter.h \
    QtAV/Packet.h \
    QtAV/AVError.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/VideoRendererTypes.h \
    QtAV/VideoOutput.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/VideoDecoderTypes.h \
    QtAV/VideoFormat.h \
    QtAV/VideoFrame.h \
    QtAV/FactoryDefine.h \
    QtAV/Statistics.h \
    QtAV/SurfaceInterop.h \
    QtAV/version.h


HEADERS *= \
    $$SDK_HEADERS \
    utils/GPUMemCopy.h \
    QtAV/prepost.h \
    QtAV/AVDemuxThread.h \
    QtAV/AVThread.h \
    QtAV/AudioThread.h \
    QtAV/VideoThread.h \
    QtAV/ColorTransform.h \
    QtAV/VideoOutputEventFilter.h \
    QtAV/OutputSet.h \
    QtAV/QtAV_Compat.h \
    QtAV/QAVIOContext.h \
    QtAV/singleton.h \
    QtAV/factory.h \
    QtAV/FilterManager.h \
    QtAV/private/AudioOutput_p.h \
    QtAV/private/AudioResampler_p.h \
    QtAV/private/AVThread_p.h \
    QtAV/private/AVDecoder_p.h \
    QtAV/private/AVOutput_p.h \
    QtAV/private/Filter_p.h \
    QtAV/private/Frame_p.h \
    QtAV/private/GraphicsItemRenderer_p.h \
    QtAV/private/ImageConverter_p.h \
    QtAV/private/VideoDecoder_p.h \
    QtAV/private/VideoDecoderFFmpegHW_p.h \
    QtAV/VideoDecoderFFmpegHW.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/private/QPainterRenderer_p.h \
    QtAV/private/WidgetRenderer_p.h

INCLUDEPATH += $$VPATH $$PWD/../../ffmpeg-lgpl/include

LIBS += -L$$PWD/../../ffmpeg-lgpl/lib
LIBS += -lavutil \
        -lavcodec \
        -lavformat \
        -lswscale

DEFINES += BUILD_QTAV_LIB IGNORE_SEEK_TIMER

DESTDIR = $$PWD/lib
