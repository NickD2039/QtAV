#if 1
#include <QtAV/VideoDecoder.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>


#include <QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Packet.h>

#include <QtDebug>

using namespace QtAV;

class VideoDecoderAccess: public VideoDecoder
{
    VideoDecoderAccess();
public:
    static AVCodecContext* getAvContext(const VideoDecoder& dec)
    {
        return reinterpret_cast<const VideoDecoderAccess*>(&dec)->getAvContext();
    }
    static AVFrame* getFrame(const VideoDecoder& dec)
    {
        return reinterpret_cast<const VideoDecoderAccess*>(&dec)->getFrame();
    }
private:
    AVCodecContext* getAvContext() const;
    AVFrame* getFrame() const;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString file = "test.avi";
    int idx = a.arguments().indexOf("-f");
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString decName("FFmpeg");
    idx = a.arguments().indexOf("-vc");
    if (idx > 0)
        decName = a.arguments().at(idx + 1);

    VideoDecoderId cid = VideoDecoderFactory::id(decName.toStdString());
    if (cid <= 0) {
        qWarning("Can not find decoder: %s", decName.toUtf8().constData());
        return 1;
    }
    VideoDecoder *dec = VideoDecoderFactory::create(cid);
    AVDemuxer demux;
    if (!demux.loadFile(file)) {
        qWarning("Failed to load file: %s", file.toUtf8().constData());
        return 1;
    }

    dec->setCodecContext(demux.videoCodecContext());
    dec->prepare();
    dec->open();
    QElapsedTimer timer;
    timer.start();
    int count = 0;
    VideoFrame frame;

    for (int rep=0; rep<10;++rep)
    {
        int frameNum = 0;
        while (!demux.atEnd()) {
            if (!demux.readFrame())
                continue;
            if (dec->decode(demux.packet()->data))
            {
                QImage out = dec->toImage();
                qDebug() << out.width() << out.height();
#if 0
                if (!rep) // write image so we can have a look-see
                    out.save(QString("qtavtest-%1.png").arg(++frameNum));
#endif



                /*
                 * TODO: may contains more than 1 frames
                 * map from gpu or not?
                 */
                //frame = dec->frame().clone();
                count++;
            }
        }
        demux.seek(0LL);
    }

    qint64 elapsed = timer.elapsed();
    int msec = elapsed/1000LL;
    qDebug("decoded frames: %d, time: %d, average speed: %d", count, msec, msec ? count/msec : 0);

    delete dec;

    return 0;
}


// ND: need to add QtAV/src/compat/msvc and ffmpeg/include to INCLUDES, plus define __STDC_CONSTANT_MACROS for this
#include <QtAV/private/AVDecoder_p.h>

AVCodecContext* VideoDecoderAccess::getAvContext() const
{
    return dptr.pri<AVDecoderPrivate>().codec_ctx;
}

AVFrame* VideoDecoderAccess::getFrame() const
{
    return dptr.pri<AVDecoderPrivate>().frame;
}


#else
#include <QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AVDemuxer.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/Packet.h>

using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString file = "test.avi";
    int idx = a.arguments().indexOf("-f");
    if (idx > 0)
        file = a.arguments().at(idx + 1);
    QString decName("FFmpeg");
    idx = a.arguments().indexOf("-vc");
    if (idx > 0)
        decName = a.arguments().at(idx + 1);

    VideoDecoderId cid = VideoDecoderFactory::id(decName.toStdString());
    if (cid <= 0) {
        qWarning("Can not find decoder: %s", decName.toUtf8().constData());
        return 1;
    }
    VideoDecoder *dec = VideoDecoderFactory::create(cid);
    AVDemuxer demux;
    if (!demux.loadFile(file)) {
        qWarning("Failed to load file: %s", file.toUtf8().constData());
        return 1;
    }

    dec->setCodecContext(demux.videoCodecContext());
    dec->prepare();
    dec->open();
    QElapsedTimer timer;
    timer.start();
    int count = 0;
    VideoFrame frame;
    while (!demux.atEnd()) {
        if (!demux.readFrame())
            continue;
        if (dec->decode(demux.packet()->data)) {
            /*
             * TODO: may contains more than 1 frames
             * map from gpu or not?
             */
            //frame = dec->frame().clone();
            count++;
        }
    }
    qint64 elapsed = timer.elapsed();
    int msec = elapsed/1000LL;
    qDebug("decoded frames: %d, time: %d, average speed: %d", count, msec, msec ? count/msec : 0);
    return 0;
}
#endif