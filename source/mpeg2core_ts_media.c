#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
#include "mpeg2core_common.h"
static uint8_t h264_nalu_aud[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0xF0};
static uint8_t h265_nalu_aud[7] = {0x00, 0x00, 0x00, 0x01, 0x46, 0x01, 0x50};
int mpeg2_ts_audio_parse(mpeg2_ts_context *context, int type, int stream_pid){
    if(context == NULL){
        return -1;
    }
    if((context->ts_header.payload == NULL) || (context->ts_header.payload_len <= 0)){
        return 0;
    }
    if(context->ts_header.payload_unit_start_indicator == 1){ // pointer_field only in the first package
        // pes解析
        mpeg2_pes_header pes_header;
        int media_pos = 0;
        if(mpeg2_pes_packet_parse(&pes_header, context->pes_buffer_a, context->pes_buffer_pos_a, &media_pos) < 0){
            return -1;
        }
        if(context->audio_read_callback && (media_pos > 0)){
            context->audio_read_callback(context->pmt.program_number, stream_pid, type, pes_header.pts, pes_header.dts, context->pes_buffer_a + media_pos, context->pes_buffer_pos_a - media_pos, context->arg);
        }
        context->pes_buffer_pos_a = 0;
    }
    memcpy(context->pes_buffer_a + context->pes_buffer_pos_a, context->ts_header.payload, context->ts_header.payload_len);
    context->pes_buffer_pos_a += context->ts_header.payload_len;
    return 0;
}
int mpeg2_ts_video_parse(mpeg2_ts_context *context, int type, int stream_pid){
    if(context == NULL){
        return -1;
    }
    if((context->ts_header.payload == NULL) || (context->ts_header.payload_len <= 0)){
        return 0;
    }
    if(context->ts_header.payload_unit_start_indicator == 1){ // pointer_field only in the first package
        // pes解析
        mpeg2_pes_header pes_header;
        int media_pos = 0;
        if(mpeg2_pes_packet_parse(&pes_header, context->pes_buffer_v, context->pes_buffer_pos_v, &media_pos) < 0){
            return -1;
        }
        if(context->video_read_callback && (media_pos > 0)){
            context->video_read_callback(context->pmt.program_number, stream_pid, type, pes_header.pts, pes_header.dts, context->pes_buffer_v + media_pos, context->pes_buffer_pos_v - media_pos, context->arg);
        }
        context->pes_buffer_pos_v = 0;
    }
    memcpy(context->pes_buffer_v + context->pes_buffer_pos_v, context->ts_header.payload, context->ts_header.payload_len);
    context->pes_buffer_pos_v += context->ts_header.payload_len;
    return 0;
}
int mpeg2_is_video(int stream_type){
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MPEG2:
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
        case STREAM_TYPE_AUDIO_MPEG2:
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
int mpeg2_ts_media_pack(mpeg2_ts_context *context, mpeg2_pmt *pmt, mpeg2_pmt_stream *pmt_stream){
    if(!context){
        return -1;
    }
    uint8_t *ptr = NULL;
    int ptr_len = 0;
    int pos = 0;
    if(mpeg2_increase_stream_frame_cnt(context, pmt_stream->elementary_PID, &pmt, &pmt_stream) < 0){
        return -1;
    }
    int frame_cnt = pmt_stream->frame_cnt;
    int pcr_flag = 0;
    if(mpeg2_is_audio(pmt_stream->stream_type)){
        ptr = context->pes_buffer_a;
        ptr_len = context->pes_buffer_pos_a;
    }
    else if(mpeg2_is_video(pmt_stream->stream_type)){
        ptr = context->pes_buffer_v;
        ptr_len = context->pes_buffer_pos_v;
    }
    else{
        return -1;
    }
    if((pmt_stream->elementary_PID == pmt->pcr_pid) && ((frame_cnt % context->pcr_period == 0) || (frame_cnt == 0))){
        pcr_flag = 1;
    }
    mpeg2_ts_header ts_header;
    memset(&ts_header, 0, sizeof(mpeg2_ts_header));
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 1;
    ts_header.pid = pmt_stream->elementary_PID;
    ts_header.transport_priority = 0;
    ts_header.transport_scrambling_control = 0;
    ts_header.continuity_counter = pmt_stream->stream_continuity_counter;
    int ts_header_len = 0;
    int stuffing_bytes = 0;
    int available_bytes = sizeof(context->ts_buffer) - 4/*ts fixed len*/ - 2/*adaptation fixed header len*/;
    
    if(pcr_flag){
        available_bytes -= 6/*PCR*/;
    }
    if(available_bytes >= ptr_len){
        ts_header.adaptation_field_control = 3; // adaptation(may have stuff, video have PCR) + payload
        if(context->key_flag){ // random_access_indicator
            ts_header.adaptation.random_access_indicator = 1;
        }
        if(pcr_flag){
            // PCR
            int64_t pcr;
            pcr = context->pts;
            if(context->dts > 0){
                pcr = context->dts;
            }
            ts_header.adaptation.PCR_flag = 1;
            ts_header.PCR = (pcr - PCR_DELAY) * 300; // timebase 90kHz -> 27MHz
        }

        stuffing_bytes = available_bytes - ptr_len;
        ts_header_len = mpeg2_ts_header_pack(context->ts_buffer, sizeof(context->ts_buffer), ts_header, 0, stuffing_bytes);
        if(ts_header_len < 0){
            return -1;
        }
        
        memcpy(context->ts_buffer + ts_header_len + stuffing_bytes, ptr, ptr_len);
        memset(context->ts_buffer + ts_header_len, 0xff, stuffing_bytes);
        if(context->media_write_callback){
            context->media_write_callback(pmt->program_number, pmt_stream->elementary_PID, pmt_stream->stream_type, context->ts_buffer, sizeof(context->ts_buffer), context->arg);
        }
        if(mpeg2_increase_stream_continuity_counter(context, pmt_stream->elementary_PID, &pmt, &pmt_stream) < 0){
            return -1;
        }
    }
    else{ // Split into multiple packages
        int spilt_over = 0;
        for(int i = 0;;i++){
            int remaining_bytes = ptr_len - pos;
            if(i == 0){
                ts_header.payload_unit_start_indicator = 1;
                if(context->key_flag){ // random_access_indicator
                    ts_header.adaptation.random_access_indicator = 1;
                }
                if(pcr_flag){
                    // PCR
                    int64_t pcr;
                    pcr = context->pts;
                    if(context->dts > 0){
                        pcr = context->dts;
                    }
                    ts_header.adaptation.PCR_flag = 1;
                    ts_header.PCR = (pcr - PCR_DELAY) * 300; // timebase 90kHz -> 27MHz
                }

                ts_header.adaptation_field_control = 3; // daptation(not have stuff, video have PCR) + payload
            }
            else{
                ts_header.payload_unit_start_indicator = 0;
                ts_header.adaptation.random_access_indicator = 0;
                ts_header.adaptation.PCR_flag = 0;
                
                int available_bytes_no_adaptation = sizeof(context->ts_buffer) - 4/*ts fixed len*/;
                int available_bytes_have_adaptation_no_pcr = sizeof(context->ts_buffer) - 4/*ts fixed len*/ - 2/*adaptation fixed len*/;
                if(remaining_bytes <= available_bytes_have_adaptation_no_pcr){
                    ts_header.adaptation_field_control = 3; // daptation(may have stuff,not have PCR) + payload
                    stuffing_bytes = available_bytes_have_adaptation_no_pcr - remaining_bytes;
                    spilt_over = 1;
                }
                else{
                    ts_header.adaptation_field_control = 1; // only payload
                }
            }
            ts_header_len = mpeg2_ts_header_pack(context->ts_buffer, sizeof(context->ts_buffer), ts_header, 0, stuffing_bytes);
            if(ts_header_len < 0){
                return -1;
            }
            int memcpy_len = sizeof(context->ts_buffer) - ts_header_len - stuffing_bytes;
            memcpy(context->ts_buffer + ts_header_len + stuffing_bytes, ptr + pos, memcpy_len);
            pos += memcpy_len;
            memset(context->ts_buffer + ts_header_len, 0xff, stuffing_bytes);
            
            if(context->media_write_callback){
                context->media_write_callback(pmt->program_number, pmt_stream->elementary_PID, pmt_stream->stream_type, context->ts_buffer, sizeof(context->ts_buffer), context->arg);
            }
            if(mpeg2_increase_stream_continuity_counter(context, pmt_stream->elementary_PID, &pmt, &pmt_stream) < 0){
                return -1;
            }
            ts_header.continuity_counter = pmt_stream->stream_continuity_counter;
            if((spilt_over == 1) || (pos >= ptr_len)){
                break;
            }
        }
    }
    return 0;
}

int mpeg2_add_h264_aud(uint8_t *buffer, int len){
    if(len < sizeof(h264_nalu_aud)){
        return 0;
    }
    memcpy(buffer, h264_nalu_aud, sizeof(h264_nalu_aud));
    return sizeof(h264_nalu_aud);
}

int mpeg2_h264_aud_size(){
    return sizeof(h264_nalu_aud);
}

int mpeg2_add_h265_aud(uint8_t *buffer, int len){
    if(len < sizeof(h265_nalu_aud)){
        return 0;
    }
    memcpy(buffer, h265_nalu_aud, sizeof(h265_nalu_aud));
    return sizeof(h265_nalu_aud);
}

int mpeg2_h265_aud_size(){
    return sizeof(h265_nalu_aud);
}