#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
#include "mpeg2core_common.h"
/**
 * case 1.mpeg1: no have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE. mpeg2core_pes.h
 *               file: ../media/h265_mpeg1.ps
 * case 2.mpeg1: no have psm, pes type-->PRIVATE_STREAM_1\PADDING_STREAM\PRIVATE_STREAM_2\PES_H264_ID\PES_AC3_ID\PES_DTS_ID\PES_LPCM_ID\PES_SUB_ID. mpeg2core_type.h
 *               file: ../media/h264_mpeg1.ps
 * case 3.mpeg2: have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE . mpeg2core_pes.h
 *               file: ../media/h264_mpeg2.ps
 */

// case 1 or case 3
int mpeg2_ps_media_parse(mpeg2_ps_context *context, uint8_t *pes_buffer, int pes_buffer_len){
    mpeg2_pes_header pes_header;
    int media_pos = 0;
    if(context->ps_header.is_mpeg2){
        if(mpeg2_pes_packet_parse(&pes_header, pes_buffer, pes_buffer_len, &media_pos) < 0){
            return -1;
        }
    }
    else{
        if(mpeg2_pes_mpeg1_packet_parse(&pes_header, pes_buffer, pes_buffer_len, &media_pos) < 0){
            return -1;
        }
    }
    if(media_pos <= 0){
        return 0;
    }
    int find_flag = 0;
    for(int i = 0; i < context->psm.psm_stream_array_num; i++){
        mpeg2_psm_stream psm_stream =  context->psm.psm_stream_array[i];
        if(psm_stream.elementary_stream_id == pes_header.stream_id){
            switch (psm_stream.elementary_stream_id){ // case 3
                case PES_VIDEO:
                case PES_VIDEO_PRIVATE:
                    find_flag = 1;
                    if(context->video_read_callback){
                        context->video_read_callback(psm_stream.stream_type, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg);
                    }
                    break;
                case PES_AUDIO:
                case PES_AUDIO_PRIVATE:
                    find_flag = 1;
                    if(context->audio_read_callback){
                        context->audio_read_callback(psm_stream.stream_type, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    if(find_flag == 0){ // case 1
        switch (pes_header.stream_id){
            case PES_VIDEO:
            case PES_VIDEO_PRIVATE:
                find_flag = 1;
                if(context->video_read_callback){
                    context->video_read_callback(0, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg); // 0 unknow type
                }
                break;
            case PES_AUDIO:
            case PES_AUDIO_PRIVATE:
                find_flag = 1;
                if(context->audio_read_callback){
                    context->audio_read_callback(0, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg); // 0 unknow type
                }
                break;
            default:
                break;
        }
    }
    return 0;
}
// case 2
int mpeg2_ps_mpeg1_media_parse(mpeg2_ps_context *context, uint8_t *pes_buffer, int pes_buffer_len){
    mpeg2_pes_header pes_header;
    int media_pos = 0;
    if(context->ps_header.is_mpeg2){
        if(mpeg2_pes_packet_parse(&pes_header, pes_buffer, pes_buffer_len, &media_pos) < 0){
            return -1;
        }
    }
    else{
        if(mpeg2_pes_mpeg1_packet_parse(&pes_header, pes_buffer, pes_buffer_len, &media_pos) < 0){
            return -1;
        }
    }
    if(media_pos <= 0){
        return 0;
    }
    switch (pes_header.stream_id){
        case PES_H264_ID:
            if(context->video_read_callback){
                context->video_read_callback(pes_header.stream_id, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg);
            }
            break;
        case PES_AC3_ID:
        case PES_DTS_ID:
        case PES_LPCM_ID:
        case PES_SUB_ID:
            if(context->audio_read_callback){
                context->audio_read_callback(pes_header.stream_id, pes_header.pts, pes_header.dts, pes_buffer + media_pos, pes_buffer_len - media_pos, context->arg);
            }
            break;
        default:
            break;
    }
    return 0;
}