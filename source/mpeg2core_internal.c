#include "mpeg2core_internal.h"
int mpeg2_is_video(int stream_type){
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MP3:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
            break;
        case STREAM_TYPE_VIDEO_H264:
        case STREAM_TYPE_VIDEO_HEVC:
            return 1;
            break;
        default:
            break;
    }
    return 0;
}
int mpeg2_is_audio(int stream_type){
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MP3:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
            return 1;
            break;
        case STREAM_TYPE_VIDEO_H264:
        case STREAM_TYPE_VIDEO_HEVC:
            break;
        default:
            break;
    }
    return 0;
}