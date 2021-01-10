/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "videostream.h"
#include "ffmpegdriver.h"
#include <QImage>

#include <cassert>
#include <iostream>

namespace
{

template<typename trait>
bool decode_yuv(uint8_t * const pRGB, const uint32_t rgb_stride, const uint8_t * const pY, const uint32_t y_stride, const uint8_t * const pU, const uint32_t u_stride, const uint8_t * const pV, const uint32_t v_stride, const uint32_t width, const uint32_t height, const uint8_t alpha=0xff)
{
    if (0!=(width&1) || width<2 || 0!=(height&1) || height<2 || !pRGB || !pY || !pU || !pV)
        return false;

    int32_t Y00{}, Y01{}, Y10{}, Y11{};
    int32_t V{}, U{};
    int32_t tR{}, tG{}, tB{};

    for (uint32_t h{}; h < height; h += 2) {
        const uint8_t *y0 = pY + y_stride * h;
        const uint8_t *y1 = y0 + y_stride;
        const uint8_t *u0 = pU + u_stride * (h >> 1);
        const uint8_t *v0 = pV + v_stride * (h >> 1);
        uint8_t *dst0 = pRGB + rgb_stride * h;
        uint8_t *dst1 = dst0 + rgb_stride;
        for (uint32_t w{}; w < width; w += 2) {
            Y00 = std::max((*y0++) - 16, 0) * 298;  Y01 = std::max((*y0++) - 16, 0) * 298;
            Y10 = std::max((*y1++) - 16, 0) * 298;  Y11 = std::max((*y1++) - 16, 0) * 298;

            trait::loadvu(U, V, u0, v0);

            tR = 128 + 409 * V;
            tG = 128 - 100 * U - 208 * V;
            tB = 128 + 516 * U;

            trait::store_pixel(dst0, Y00 + tR, Y00 + tG, Y00 + tB, alpha);
            trait::store_pixel(dst0, Y01 + tR, Y01 + tG, Y01 + tB, alpha);
            trait::store_pixel(dst1, Y10 + tR, Y10 + tG, Y10 + tB, alpha);
            trait::store_pixel(dst1, Y11 + tR, Y11 + tG, Y11 + tB, alpha);
        }
    }
    return true;
}

class YUVtoBGR {
public:
    enum { bytes_per_pixel = 4 };
    static void loadvu(int &U, int &V, const uint8_t *&u, const uint8_t *&v) {
        U = (*u++) - 128;
        V = (*v++) - 128;
    }
    static void store_pixel(uint8_t *&dst, int iR, int iG, int iB, const uint8_t alpha) {
        *dst++ = static_cast<uint8_t>(std::min(std::max(iB >> 8, 0), 0xFF));
        *dst++ = static_cast<uint8_t>(std::min(std::max(iG >> 8, 0), 0xFF));
        *dst++ = static_cast<uint8_t>(std::min(std::max(iR >> 8, 0), 0xFF));
        *dst++ = alpha;
    }
};

bool yuv_to_bgr(uint8_t * const pRGB, const uint32_t rgb_stride, const uint8_t * const pY, const uint32_t y_stride, const uint8_t * const pU, const uint32_t u_stride, const uint8_t * const pV, const uint32_t v_stride, const uint32_t width, const uint32_t height) {
    return decode_yuv<YUVtoBGR>(pRGB, rgb_stride, pY, y_stride, pU, u_stride, pV, v_stride, width, height);
}

} // namespace unnamed

VideoStream::VideoStream(const char *fname) {
    if (AVFormatDll::getInstance().p_avformat_open_input(&fmt_ctx_, fname, nullptr, nullptr) < 0) {
        std::cout << "Could not open source file" << std::endl;
    }
    if (AVFormatDll::getInstance().p_avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        std::cout << "Could not find stream information" << std::endl;
    }
    AVCodec *dec = nullptr;
    int ret = AVFormatDll::getInstance().p_av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        std::cout << "Could not find " << AVUtilDll::getInstance().p_av_get_media_type_string(AVMEDIA_TYPE_VIDEO) << " stream in input file" << std::endl;
    }
    video_stream_idx_ = ret;
    AVStream *st = fmt_ctx_->streams[video_stream_idx_];
    total_frame_ = st->nb_frames;
    std::cout << "Frames: " << st->nb_frames << std::endl;
    std::cout << "Start time: " << st->start_time << std::endl;
    video_dec_ctx_ = AVCodecDll::getInstance().p_avcodec_alloc_context3(dec);
    if (!video_dec_ctx_) {
        std::cout << "Failed to allocate codec" << std::endl;
    }
    ret = AVCodecDll::getInstance().p_avcodec_parameters_to_context(video_dec_ctx_, st->codecpar);
    if (ret < 0) {
        std::cout << "Failed to copy codec parameters to codec context" << std::endl;
    }
    if ((ret = AVCodecDll::getInstance().p_avcodec_open2(video_dec_ctx_, dec, nullptr)) < 0) {
        std::cout << "Failed to open " << AVUtilDll::getInstance().p_av_get_media_type_string(AVMEDIA_TYPE_VIDEO) << " codec" << std::endl;
    }

    frame_ = AVUtilDll::getInstance().p_av_frame_alloc();
    w_ = video_dec_ctx_->width;
    h_ = video_dec_ctx_->height;
    std::cout << w_ << "x" << h_ << "  "  << frame_ << std::endl;
}

VideoStream::~VideoStream() {
    AVUtilDll::getInstance().p_av_frame_free(&frame_);
    AVCodecDll::getInstance().p_avcodec_free_context(&video_dec_ctx_);
    AVFormatDll::getInstance().p_avformat_close_input(&fmt_ctx_);
}

int VideoStream::decode_packet(const AVPacket *pkt, QImage &img) {
    std::cout << "decode_packet" << std::endl;
    char buf[AV_ERROR_MAX_STRING_SIZE] = { };
    int ret = AVCodecDll::getInstance().p_avcodec_send_packet(video_dec_ctx_, pkt);
    if (ret < 0) {
        AVUtilDll::getInstance().p_av_strerror(ret, buf, AV_ERROR_MAX_STRING_SIZE);
        std::cout << "Error while sending a packet to the decoder: " << buf << std::endl;
        return ret;
    }

    while (ret >= 0) {
        ret = AVCodecDll::getInstance().p_avcodec_receive_frame(video_dec_ctx_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            std::cout << "Again" << std::endl;
            break;
        }
        else if (ret < 0) {
            AVUtilDll::getInstance().p_av_strerror(ret, buf, AV_ERROR_MAX_STRING_SIZE);
            std::cout << "Error while receiving a frame from the decoder: " << buf << std::endl;
            return ret;
        }
        pts_ = AVUtilDll::getInstance().p_av_frame_get_best_effort_timestamp(frame_);
        std::cout << "pts: " << pts_ << "  " << frame_->key_frame << std::endl;

        std::cout << frame_->linesize[0] << " - " << frame_->linesize[1] << " - " << frame_->linesize[2] << std::endl;
        if (AV_PIX_FMT_YUV420P == frame_->format) {
            yuv_to_bgr(img.bits(), img.bytesPerLine(), frame_->data[0], frame_->linesize[0], frame_->data[1], frame_->linesize[1], frame_->data[2], frame_->linesize[2], frame_->width, frame_->height);
        }

        //img = new QImage(frame_->data[0], frame_->width, frame_->height, frame_->linesize[0], QImage::Format_Grayscale8);
        /*for (int y{}; y < frame_->height; ++y) {
            const uint8_t *pY = frame_->data[0] + y * frame_->linesize[0];
        }*/
        AVUtilDll::getInstance().p_av_frame_unref(frame_);
        std::cout << "decode_packet exit 1" << std::endl;
        return 1;
    }
    std::cout << "decode_packet exit 0" << std::endl;
    return 0;
}

bool VideoStream::getNextFrame(QImage &img) {
    assert(this->getWidth() == img.width() && this->getHeight() == img.height());
    std::cout << "getNextFrame" << std::endl;
    if (frame_) {
        AVPacket pkt = { };
        AVCodecDll::getInstance().p_av_init_packet(&pkt);
        int res{};
        while (AVFormatDll::getInstance().p_av_read_frame(fmt_ctx_, &pkt) >= 0) {
            if (pkt.stream_index == video_stream_idx_) {
                res = decode_packet(&pkt, img);
            }
            AVCodecDll::getInstance().p_av_packet_unref(&pkt);
            if (1 == res) {
                std::cout << "True" << std::endl;
                return true;
            }
            if (res < 0) {
                break;
            }
        }
        // flush cached frames
        decode_packet(&pkt, img);
    }
    else {
        std::cout << "Could not allocate frame" << std::endl;
    }
    std::cout << "False" << std::endl;
    return false;
}

bool VideoStream::seek(int64_t t) {
    std::cout << "seek" << std::endl;
    int ret = AVFormatDll::getInstance().p_av_seek_frame(fmt_ctx_, -1, pts_ + t, AVSEEK_FLAG_ANY);
    if (ret < 0) {
        std::cout << "Seek error" << std::endl;
    }
    else {
        pts_ += t;
    }
    AVCodecDll::getInstance().p_avcodec_flush_buffers(video_dec_ctx_);
    return ret >= 0;
}
