/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include <QtAV/VideoDecoder.h>
#include <QtAV/private/VideoDecoder_p.h>
#include <QtCore/QSize>
#include "QtAV/factory.h"
#include <QtDebug>

namespace QtAV {

FACTORY_DEFINE(VideoDecoder)

extern void RegisterVideoDecoderFFmpeg_Man();
extern void RegisterVideoDecoderDXVA_Man();

void VideoDecoder_RegisterAll()
{
    RegisterVideoDecoderFFmpeg_Man();
#if QTAV_HAVE(DXVA)
    RegisterVideoDecoderDXVA_Man();
#endif //QTAV_HAVE(DXVA)
}


VideoDecoder::VideoDecoder()
    :AVDecoder(*new VideoDecoderPrivate())
{
}

VideoDecoder::VideoDecoder(VideoDecoderPrivate &d):
    AVDecoder(d)
{
}

QString VideoDecoder::name() const
{
    return QString(VideoDecoderFactory::name(id()).c_str());
}

bool VideoDecoder::prepare()
{
    return AVDecoder::prepare();
}

//TODO: use ipp, cuda decode and yuv functions. is sws_scale necessary?
bool VideoDecoder::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoder);
    AVPacket packet;
#if NO_PADDING_DATA
    /*!
      larger than the actual read bytes because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
      The end of the input buffer avpkt->data should be set to 0 to ensure that no overreading happens for damaged MPEG streams
     */
    // auto released by av_buffer_default_free
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
#else
    av_init_packet(&packet);
    packet.size = encoded.size();
    packet.data = (uint8_t*)encoded.constData();
#endif //NO_PADDING_DATA
//TODO: use AVPacket directly instead of Packet?
    //AVStream *stream = format_context->streams[stream_idx];

    //TODO: some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
    int ret = avcodec_decode_video2(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    //qDebug("pic_type=%c", av_get_picture_type_char(d.frame->pict_type));
    d.undecoded_size = qMin(encoded.size() - ret, encoded.size());
    //TODO: decoded format is YUV420P, YUV422P?
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[VideoDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("no frame could be decompressed: %s", av_err2str(ret));
        return false;
    }
    if (!w || !h)
        return false;
    //qDebug("codec %dx%d, frame %dx%d", w, h, d.frame->width, d.frame->height);
    d.width = d.frame->width;
    d.height = d.frame->height;
    //avcodec_align_dimensions2(d.codec_ctx, &d.width_align, &d.height_align, aligns);
    return true;
}

void VideoDecoder::resizeVideoFrame(const QSize &size)
{
    resizeVideoFrame(size.width(), size.height());
}

/*
 * width, height: the decoded frame size
 * 0, 0 to reset to original video frame size
 */
void VideoDecoder::resizeVideoFrame(int width, int height)
{
    DPTR_D(VideoDecoder);
    d.width = width;
    d.height = height;
}

int VideoDecoder::width() const
{
    return d_func().width;
}

int VideoDecoder::height() const
{
    return d_func().height;
}

VideoFrame VideoDecoder::frame()
{
    DPTR_D(VideoDecoder);
    if (d.width <= 0 || d.height <= 0 || !d.codec_ctx)
        return VideoFrame(0, 0, VideoFormat(VideoFormat::Format_Invalid));
    //DO NOT make frame as a memeber, because VideoFrame is explictly shared!
    float displayAspectRatio = 0;
    if (d.codec_ctx->sample_aspect_ratio.den > 0)
        displayAspectRatio = ((float)d.frame->width / (float)d.frame->height) *
            ((float)d.codec_ctx->sample_aspect_ratio.num / (float)d.codec_ctx->sample_aspect_ratio.den);

    VideoFrame frame(d.frame->width, d.frame->height, VideoFormat((int)d.codec_ctx->pix_fmt));
    frame.setDisplayAspectRatio(displayAspectRatio);
    frame.setBits(d.frame->data);
    frame.setBytesPerLine(d.frame->linesize);
    return frame;
}

QImage VideoDecoder::toImage(QImage::Format fmt, const QSize& outSize_)
{
    DPTR_D(VideoDecoder);

    AVPixelFormat ffmt = (AVPixelFormat) d.frame->format;
    qDebug() << (int) ffmt;

    int w = d.codec_ctx->width;
    int h = d.codec_ctx->height;

    if (!d.rgb_frame) // first time setup
    {
       d.rgb_frame = av_frame_alloc();
       Q_ASSERT(d.rgb_frame);
       if (!d.rgb_frame)
           return QImage();

       // Determine required buffer size and allocate rgb_buffer
       int numBytes = avpicture_get_size(PIX_FMT_RGB24, w, h);
       d.rgb_buffer = new uint8_t[numBytes];

       // Assign appropriate parts of rgb_buffer to image planes in rgb_frame
       avpicture_fill((AVPicture *)d.rgb_frame, d.rgb_buffer, PIX_FMT_RGB24, w, h);

    }

    // Convert the image format (init the context the first time)
    d.scale_ctx = sws_getCachedContext(d.scale_ctx,w, h, d.codec_ctx->pix_fmt, w, h, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    if (d.scale_ctx)
    {
        sws_scale(d.scale_ctx, d.frame->data, d.frame->linesize, 0, h, d.rgb_frame->data, d.rgb_frame->linesize);

        QSize inSize(w, h);
        QSize outSize = outSize_.isNull() ? inSize : outSize_;

        // keep previous outImage if dimensions and format are unchanged
        if ((d.outImage.size() != outSize) || (d.outImage.format() != fmt))
            d.outImage = QImage(outSize, fmt);

        if (inSize == outSize) // straightforward RGB24 to RGB32 conversion
        {
            for (int y=0; y<h; ++y)
            {
                QRgb* outLine = (QRgb*) d.outImage.scanLine(y);
                uchar* inLine = d.rgb_frame->data[0]+y*d.rgb_frame->linesize[0];
                for (int x=0; x<w; ++x)
                {
                    uchar* inRgb = inLine + (x*3);
                    outLine[x] = qRgb(inRgb[0], inRgb[1], inRgb[2]);
                }
            }
        }
        else // scale to outSize
        {
            int outHeight = outSize.height();
            int outWidth = outSize.width();
            int inHeight = inSize.height();
            int inWidth = inSize.width();
            for (int y=0; y<outHeight; ++y)
            {
                int yy = y * inHeight / outHeight;
                Q_ASSERT(yy < inHeight);
                QRgb* outLine = (QRgb*) d.outImage.scanLine(y);
                uchar* inLine = d.rgb_frame->data[0]+yy*d.rgb_frame->linesize[0];
                for (int x=0; x<outWidth; ++x)
                {
                    int xx = x * inWidth / outWidth;
                    Q_ASSERT(xx < inWidth);
                    uchar* inRgb = inLine + (xx*3);
                    outLine[x] = qRgb(inRgb[0], inRgb[1], inRgb[2]);
                }
            }
        }
    }

    return d.outImage;
}


} //namespace QtAV
