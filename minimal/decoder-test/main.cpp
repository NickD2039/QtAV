#include <QApplication>
#include <QtGui>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Packet.h>
#include <QtAV/VideoDecoder.h>

#include <QtAV/AudioDecoder.h>

#include <QtDebug>

using namespace QtAV;

typedef unsigned char ubyte;
struct SampleValue
{
    ubyte low, high;

    SampleValue(): low(255), high(0) {}
    bool valid() const {return high >= low;}
    void merge(const SampleValue& rhs)
    {
        low = std::min(low, rhs.low);
        high = std::max(high, rhs.high);
    }
};

struct Sample
{
    SampleValue left, right;
    void merge(const Sample& rhs)
    {
        left.merge(rhs.left);
        right.merge(rhs.right);
    }
};

struct SampleBlock
{
    enum {Size = 1024};
    Sample data[Size];
};

struct SampleBuffer
{
    SampleBuffer()
        : _size(0)
    {
    }
    SampleBuffer(const SampleBuffer& rhs, int div)
        : _size(0)
    {
        Q_ASSERT(div > 0);
        if (div > 0)
        {
            Sample* pTo = 0;
            for (ulong i=0; i<rhs.size(); ++i)
            {
                int iTo = i / div;
                if (iTo == size())
                    pTo = &append();
                Q_ASSERT(pTo);
                const Sample& from = rhs.get(i);
                pTo->merge(from);
            }
        }
    }
    ulong size() const {return _size;}
    Sample& get(ulong idx)
    {
        Q_ASSERT(idx < _size);
        return _blocks[idx / SampleBlock::Size]->data[idx % SampleBlock::Size];
    }
    const Sample& get(ulong idx) const {return const_cast<SampleBuffer*>(this)->get(idx);}
    Sample& append()
    {
        if (!(_size % SampleBlock::Size))
            _blocks.append(BlockPtr(new SampleBlock));
        ++_size;
        return get(_size-1);
    }
    void dbgDump(int mx = -1)
    {
        QString line(64, QChar(' '));
        for (ulong i=0; (i<size()) && (i<ulong(mx)); ++i)
        {
            const Sample& s = get(i);
            int low = s.left.low / 4;
            int high = s.left.high / 4;
            for (int c=0; c<64; ++c)
                line[c] = ((c >= low) && (c <= high)) ? QChar('*') : QChar(' ');
            qDebug() << "|" << qPrintable(line) << "|";
        }
    }

private:
    typedef QSharedPointer<SampleBlock> BlockPtr;
    QVector<BlockPtr> _blocks;
    ulong _size;
};

#include <libavcodec/avcodec.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    typedef float sample_type;

#if 1 // test audio decode
    QString file("C:/Users/Nick/Clarity/media/door_knock.mp3");
    AVDemuxer demux;
    if (demux.loadFile(file) && demux.audioCodecContext())
    {
        qDebug() << demux.frames() << demux.audioCodecContext()->frame_size;
        AudioDecoder* pad = new AudioDecoder;
        pad->setCodecContext(demux.audioCodecContext());
        pad->prepare();
        pad->open();
        int nDecoded = 0;
        SampleBuffer buf;
        while (!demux.atEnd())
        {
            if (!demux.readFrame())
                continue;
            if (demux.stream() != demux.audioStream())
                continue;
            
            QtAV::Packet* pPacket = demux.packet();
            //qDebug() << pPacket->pts;
            if (pad->decode(pPacket->data))
            {
                ++nDecoded;
                QByteArray decoded = pad->data();
                for (int pos = 0; pos < decoded.size(); pos += 8)
                {
                    sample_type l = *((sample_type*) (decoded.data() + pos));
                    sample_type r = *((sample_type*) (decoded.data() + pos+4));
                    Sample& s = buf.append();
                    s.left.low = s.left.high = ubyte((l/2 + 0.5) * 255);
                    s.right.low = s.right.high = ubyte((r/2 + 0.5) * 255);
                }
            }
        }
        SampleBuffer bufDiv8(buf, 8);
        SampleBuffer bufDiv64(bufDiv8, 8);
        SampleBuffer bufDiv512(bufDiv64, 8);
        bufDiv512.dbgDump();
        buf.dbgDump(1500);
        qDebug() << "decoded " << nDecoded << " audio packets; sample buffer size=" << buf.size();
    }
#else
    QString file = QFileDialog::getOpenFileName(0, "Open Movie");
#if 0
    QString file = "test.avi";
    int idx = a.arguments().indexOf("-f");
    if (idx > 0)
        file = a.arguments().at(idx + 1);
#endif
    QString decName("FFmpeg");
    //idx = a.arguments().indexOf("-vc");
    //if (idx > 0)
    //    decName = a.arguments().at(idx + 1);

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

    int nReps = 1;
    for (int rep=0; rep<nReps;++rep)
    {
        int frameNum = 0;
        while (!demux.atEnd()) {
            if (!demux.readFrame())
                continue;
            if (demux.stream() != demux.videoStream())
                continue;
            //qDebug() << demux.packet()->pts;
            QtAV::Packet* pPacket = demux.packet();
            //qDebug() << pPacket->pts;
            if (dec->decode(pPacket->data))
            {
#if 1
                QImage out = dec->toImage(QSize(320, 240));
                //qDebug() << out.width() << out.height();
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
    qreal secs = elapsed/1000.0;
    qDebug("decoded frames: %d, time: %f, average speed: %fms", count, secs, count ? qreal(elapsed)/count : 0);

    delete dec;
#endif
    return 0;
}

