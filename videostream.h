/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <cstdint>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
class QImage;

class VideoStream final {
public:
    VideoStream(const char *fname);
    ~VideoStream();
    size_t getFramesCount() const {
        return total_frame_;
    }
    size_t getWidth() const {
        return w_;
    }
    size_t getHeight() const {
        return h_;
    }
    bool getNextFrame(QImage &img);
    bool seek(int64_t t);

private:
    int decode_packet(const AVPacket *pkt, QImage &img);

    AVFormatContext *fmt_ctx_ = nullptr;
    AVCodecContext *video_dec_ctx_ = nullptr;
    AVFrame *frame_ = nullptr;
    size_t total_frame_ = 0, cur_frame_ = 0, w_ = 0, h_ = 0;
    int video_stream_idx_ = -1;
    int64_t pts_ = -1;
};

#endif // VIDEOSTREAM_H
