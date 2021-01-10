/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#ifndef FFMPEGDRIVER_H
#define FFMPEGDRIVER_H

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/motion_vector.h"
#include "libavutil/opt.h"

class AVUtilDll {
public:
    decltype(av_get_media_type_string) *p_av_get_media_type_string = nullptr;
    decltype(av_dict_set) *p_av_dict_set = nullptr;
    decltype(av_frame_alloc) *p_av_frame_alloc = nullptr;
    decltype(av_frame_free) *p_av_frame_free = nullptr;
    decltype(av_strerror) *p_av_strerror = nullptr;
    decltype(av_frame_unref) *p_av_frame_unref = nullptr;
    decltype(av_frame_get_side_data) *p_av_frame_get_side_data = nullptr;
    decltype(av_opt_next) *p_av_opt_next = nullptr;
    decltype(av_opt_get) *p_av_opt_get = nullptr;
    decltype(av_free) *p_av_free = nullptr;
    decltype(av_frame_get_best_effort_timestamp) *p_av_frame_get_best_effort_timestamp = nullptr;
    decltype(av_version_info) *p_av_version_info = nullptr;

    bool isInited() const {
        return bInit;
    }
    static const AVUtilDll& getInstance();
private:
    AVUtilDll();
    AVUtilDll(const AVUtilDll &) = delete;

    bool bInit = false;
};

class AVFormatDll {
public:
    decltype(av_register_all) *p_av_register_all = nullptr;
    decltype(avformat_open_input) *p_avformat_open_input = nullptr;
    decltype(avformat_find_stream_info) *p_avformat_find_stream_info = nullptr;
    decltype(avformat_close_input) *p_avformat_close_input = nullptr;
    decltype(av_dump_format) *p_av_dump_format = nullptr;
    decltype(av_find_best_stream) *p_av_find_best_stream = nullptr;
    decltype(av_read_frame) *p_av_read_frame = nullptr;
    decltype(av_seek_frame) *p_av_seek_frame = nullptr;

    bool isInited() const {
        return bInit;
    }
    static const AVFormatDll& getInstance();
private:
    AVFormatDll();
    AVFormatDll(const AVFormatDll &) = delete;

    bool bInit = false;
};

class AVCodecDll {
public:
    decltype(avcodec_find_decoder) *p_avcodec_find_decoder = nullptr;
    //decltype(avcodec_decode_video2) *p_avcodec_decode_video2 = nullptr;
    decltype(avcodec_receive_frame) *p_avcodec_receive_frame = nullptr;
    decltype(avcodec_alloc_context3) *p_avcodec_alloc_context3 = nullptr;
    decltype(avcodec_parameters_to_context) *p_avcodec_parameters_to_context = nullptr;
    decltype(avcodec_open2) *p_avcodec_open2 = nullptr;
    decltype(avcodec_free_context) *p_avcodec_free_context = nullptr;
    decltype(av_packet_unref) *p_av_packet_unref = nullptr;
    decltype(avcodec_send_packet) *p_avcodec_send_packet = nullptr;
    decltype(avcodec_flush_buffers) *p_avcodec_flush_buffers = nullptr;
    decltype(av_init_packet) *p_av_init_packet = nullptr;

    bool isInited() const {
        return bInit;
    }
    static const AVCodecDll& getInstance();
private:
    AVCodecDll();
    AVCodecDll(const AVCodecDll &) = delete;

    bool bInit = false;
};

#endif // FFMPEGDRIVER_H
