/**
 * The MIT License (MIT)
 * Copyright (c) 2017-2018 Kirill Lebedev
**/

#include "ffmpegdriver.h"

#define STRINGIFY(s) #s

#if defined(_WIN32)
#include <tchar.h>
#include <windows.h>
#define DL_LIBRARY(win, lin) LoadLibrary(win)
#define DL_FUNCTION(lib, fun) (decltype(fun)*)GetProcAddress(lib, STRINGIFY(fun))
#elif defined(__linux__)
#include <dlfcn.h>
#define DL_LIBRARY(win, lin) dlopen(lin, RTLD_LAZY)
#define DL_FUNCTION(lib, fun) (decltype(fun)*)dlsym(lib, STRINGIFY(fun))
#endif

AVUtilDll::AVUtilDll() {
    auto handle = DL_LIBRARY(_T("avutil-55.dll"), "libOpenCL.so");
    if (handle
        && ((p_av_get_media_type_string = DL_FUNCTION(handle, av_get_media_type_string)) != nullptr)
        && ((p_av_dict_set = DL_FUNCTION(handle, av_dict_set)) != nullptr)
        && ((p_av_frame_alloc = DL_FUNCTION(handle, av_frame_alloc)) != nullptr)
        && ((p_av_frame_free = DL_FUNCTION(handle, av_frame_free)) != nullptr)
        && ((p_av_strerror = DL_FUNCTION(handle, av_strerror)) != nullptr)
        && ((p_av_frame_unref = DL_FUNCTION(handle, av_frame_unref)) != nullptr)
        && ((p_av_frame_get_side_data = DL_FUNCTION(handle, av_frame_get_side_data)) != nullptr)
        && ((p_av_opt_next = DL_FUNCTION(handle, av_opt_next)) != nullptr)
        && ((p_av_opt_get = DL_FUNCTION(handle, av_opt_get)) != nullptr)
        && ((p_av_free = DL_FUNCTION(handle, av_free)) != nullptr)
        && ((p_av_frame_get_best_effort_timestamp = DL_FUNCTION(handle, av_frame_get_best_effort_timestamp)) != nullptr)
        && ((p_av_version_info = DL_FUNCTION(handle, av_version_info)) != nullptr)
       )
        bInit = true;
}

const AVUtilDll& AVUtilDll::getInstance() {
    static const AVUtilDll inst;
    return inst;
}

// -------------------------------------------------------------------------------------------

AVFormatDll::AVFormatDll() {
    auto handle = DL_LIBRARY(_T("avformat-57.dll"), "libOpenCL.so");
    if (handle
        && ((p_av_register_all = DL_FUNCTION(handle, av_register_all)) != nullptr)
        && ((p_avformat_open_input = DL_FUNCTION(handle, avformat_open_input)) != nullptr)
        && ((p_avformat_find_stream_info = DL_FUNCTION(handle, avformat_find_stream_info)) != nullptr)
        && ((p_avformat_close_input = DL_FUNCTION(handle, avformat_close_input)) != nullptr)
        && ((p_av_dump_format = DL_FUNCTION(handle, av_dump_format)) != nullptr)
        && ((p_av_find_best_stream = DL_FUNCTION(handle, av_find_best_stream)) != nullptr)
        && ((p_av_read_frame = DL_FUNCTION(handle, av_read_frame)) != nullptr)
        && ((p_av_seek_frame = DL_FUNCTION(handle, av_seek_frame)) != nullptr)
       )
        bInit = true;
}

const AVFormatDll& AVFormatDll::getInstance() {
    static const AVFormatDll inst;
    return inst;
}

// -------------------------------------------------------------------------------------------

AVCodecDll::AVCodecDll() {
    auto handle = DL_LIBRARY(_T("avcodec-57.dll"), "libOpenCL.so");
    if (handle
        && ((p_avcodec_find_decoder = DL_FUNCTION(handle, avcodec_find_decoder)) != nullptr)
        && ((p_avcodec_receive_frame = DL_FUNCTION(handle, avcodec_receive_frame)) != nullptr)
        && ((p_avcodec_alloc_context3 = DL_FUNCTION(handle, avcodec_alloc_context3)) != nullptr)
        && ((p_avcodec_parameters_to_context = DL_FUNCTION(handle, avcodec_parameters_to_context)) != nullptr)
        && ((p_avcodec_open2 = DL_FUNCTION(handle, avcodec_open2)) != nullptr)
        && ((p_avcodec_free_context = DL_FUNCTION(handle, avcodec_free_context)) != nullptr)
        && ((p_av_packet_unref = DL_FUNCTION(handle, av_packet_unref)) != nullptr)
        && ((p_avcodec_send_packet = DL_FUNCTION(handle, avcodec_send_packet)) != nullptr)
        && ((p_avcodec_flush_buffers = DL_FUNCTION(handle, avcodec_flush_buffers)) != nullptr)
        && ((p_av_init_packet = DL_FUNCTION(handle, av_init_packet)) != nullptr)
       )
        bInit = true;
}


const AVCodecDll& AVCodecDll::getInstance() {
    static const AVCodecDll inst;
    return inst;
}
