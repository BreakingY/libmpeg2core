#include "mpeg2core_ps.h"
#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
#include "mpeg2core_common.h"

mpeg2_ps_context *create_ps_context(){
    mpeg2_ps_context *context = (mpeg2_ps_context *)malloc(sizeof(mpeg2_ps_context));
    if(!context){
        return NULL;
    }
    memset(context, 0, sizeof(mpeg2_ps_context));
    context->demuxer_stat = PS_HEADER;
    context->ps_buffer = (uint8_t *)malloc(PS_MAX_BYTES);
    context->ps_buffer_len = PS_MAX_BYTES;
    context->psm_period = 10; // 10 frames
    return context;
}
void mpeg2_ps_set_read_callback(mpeg2_ps_context *context, PSVideoReadCallback video_read_callback, PSAudioReadCallback audio_read_callback, void *arg){
    if(!context){
        return;
    }
    context->video_read_callback = video_read_callback;
    context->audio_read_callback = audio_read_callback;
    context->arg = arg;
    return;
}
void mpeg2_ps_set_write_callback(mpeg2_ps_context *context, PSMediaWriteCallback media_write_callback, int file_flag, void *arg){
    if(!context){
        return;
    }
    context->media_write_callback = media_write_callback;
    context->arg = arg;
    context->file_flag = file_flag;
    return;
}

void dump_ps_header(mpeg2_ps_header ps_header){
    printf("================== ===ps header====================\n");
    printf("pack_start_code:%0x\n", ps_header.pack_start_code);
    printf("system_clock_reference_base: %" PRIu64 "\n", ps_header.system_clock_reference_base);
    printf("system_clock_reference_extension:%d\n", ps_header.system_clock_reference_extension);
    printf("program_mux_rate:%d\n", ps_header.program_mux_rate);
    printf("pack_stuffing_length:%d\n", ps_header.pack_stuffing_length);
    printf("===================================================\n");
}
void dump_psm(mpeg2_psm psm){
    printf("=========================psm=======================\n");
    printf("packet_start_code_prefix:%0x\n", psm.packet_start_code_prefix);
    printf("map_stream_id:%d\n", psm.map_stream_id);
    printf("program_stream_map_length:%d\n", psm.program_stream_map_length);
    printf("current_next_indicator:%d\n", psm.current_next_indicator);
    printf("program_stream_map_version:%d\n", psm.program_stream_map_version);
    printf("program_stream_info_length:%d\n", psm.program_stream_info_length);
    printf("elementary_stream_map_length:%d\n", psm.elementary_stream_map_length);
    for(int i = 0; i < psm.psm_stream_array_num; i++){
        mpeg2_psm_stream psm_stream = psm.psm_stream_array[i];
        printf("stream_type:%0x\n", psm_stream.stream_type);
        printf("elementary_stream_id:%0x\n", psm_stream.elementary_stream_id);
        printf("elementary_stream_info_length:%d\n", psm_stream.elementary_stream_info_length);
    }
    printf("crc_32:%d\n", psm.crc_32);
    printf("===================================================\n");
    return;
}
int mpeg2_is_psm(uint8_t *buffer, int len){
    if(!buffer || len < 4/*packet_start_code_prefix and map_stream_id*/){
        return -1;
    }
    uint32_t packet_start_code_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    if(packet_start_code_prefix != PES_STARTCODE){
        return 0;
    }
    uint8_t map_stream_id = buffer[3];
    if(map_stream_id == PSM_MAP_STREAM_ID){
        return 1;
    }
    return 0;
}
int mpeg2_is_pes_or_psm(uint8_t *buffer, int len){
    if(!buffer || len < 4/*packet_start_code_prefix and map_stream_id*/){
        return -1;
    }
    uint32_t packet_start_code_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    if(packet_start_code_prefix != PES_STARTCODE){
        return 0;
    }
    uint8_t map_stream_id = buffer[3];
    if(map_stream_id == PSM_MAP_STREAM_ID || map_stream_id == PES_VIDEO || map_stream_id == PES_AUDIO || map_stream_id == PES_VIDEO_PRIVATE || map_stream_id == PES_AUDIO_PRIVATE){
        return 1;
    }
    return 0;
}
int mpeg2_is_pes_mpeg1(uint8_t *buffer, int len){
    if(!buffer || len < 4/*packet_start_code_prefix and map_stream_id*/){
        return -1;
    }
    uint32_t packet_start_code_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    if(packet_start_code_prefix != PES_STARTCODE){
        return 0;
    }
    uint8_t map_stream_id = buffer[3];
    if(map_stream_id == PRIVATE_STREAM_1 || map_stream_id == PADDING_STREAM || map_stream_id == PRIVATE_STREAM_2 || map_stream_id == PES_H264_ID || map_stream_id == PES_AC3_ID 
        || map_stream_id == PES_DTS_ID|| map_stream_id == PES_LPCM_ID || map_stream_id == PES_SUB_ID){
        return 1;
    }
    return 0;
}
int mpeg2_ps_header_parse(uint8_t *buffer, int len, mpeg2_ps_header *ps_header){
    memset(ps_header, 0, sizeof(mpeg2_ps_header));
    if(!buffer || (len < 0)){
        return -1;
    }
    int idx = 0;
    ps_header->pack_start_code = (buffer[idx] << 24) | (buffer[idx + 1] << 16) | (buffer[idx + 2] << 8) | buffer[idx + 3];
    if(ps_header->pack_start_code != PS_HEADER_STARTCODE){
        return -1;
    }
    idx += 4;
    uint8_t v8 = buffer[idx];
    idx++;
    if(0 == (0xC0 & v8)){
        // MPEG-1
		// ISO/IEC 11172-1
		/*
		pack() {
			pack_start_code						32 bslbf
			'0010'								4 bslbf
			system_clock_reference [32..30]		3 bslbf
			marker_bit							1 bslbf
			system_clock_reference [29..15]		15 bslbf
			marker_bit							1 bslbf
			system_clock_reference [14..0]		15 bslbf
			marker_bit							1 bslbf
			marker_bit							1 bslbf
			mux_rate							22 uimsbf
			marker_bit							1 bslbf
			if (nextbits() == system_header_start_code)
				system_header ()
			while (nextbits() == packet_start_code)
				packet()
		}
		*/
        if((len - idx) < 7){
            return 0;
        }
        uint16_t system_clock_reference29_15 = (buffer[idx] << 8) | buffer[idx + 1];
        idx += 2;
        uint16_t system_clock_reference14_0 = (buffer[idx] << 8) | buffer[idx + 1];
        idx += 2;
        ps_header->system_clock_reference_base = (((uint64_t)(v8 >> 1) & 0x07) << 30) | (((uint64_t)(system_clock_reference29_15 >> 1) & 0xefff) << 15) | ((uint64_t)(system_clock_reference14_0 >> 1) & 0xefff);
        ps_header->system_clock_reference_extension = 1;
        ps_header->program_mux_rate = ((buffer[idx] & 0x7f) << 15) | (buffer[idx + 1] << 7) | ((buffer[idx + 2] & 0xfe) >> 1);
        idx += 3;
        ps_header->ready_flag = 1;
        ps_header->is_mpeg2 = 0;
    }
    else{
        if((len - idx) < 9){
            return 0;
        }
        uint64_t v64 = 0;
        uint8_t *ptr = &buffer[idx];
        for(int i = 0; i < 8; i++){
            v64 |= ptr[i] << 8;
        }
        ps_header->system_clock_reference_base = (((v8 >> 3) & 0x07) << 30) | ((v8 & 0x03) << 28) | (((v64 >> 51) & 0x1FFF) << 15) | ((v64 >> 35) & 0x7FFF);
        ps_header->system_clock_reference_extension = (uint16_t)((v64 >> 25) & 0x1F);
        ps_header->program_mux_rate = (uint32_t)((v64 >> 2) & 0x3FFFFF);
        idx += 8;
        ps_header->pack_stuffing_length = buffer[idx] & 0x07;
        idx++;
        ps_header->ready_flag = 1;
        ps_header->is_mpeg2 = 1;
    }
    return idx;
}
static int mpeg2_buffer_used_move(mpeg2_ps_context *context, int used_bytes){
    if(!context || (context->ps_buffer_pos < used_bytes)){
        return -1;
    }
    memmove(context->ps_buffer, context->ps_buffer + used_bytes, context->ps_buffer_pos - used_bytes);
    context->ps_buffer_pos -= used_bytes;
    return 0;
}
static int mpeg2_is_ps_header(uint8_t *buffer, int len){
    if(!buffer || len < 4){
        return -1;
    }
    uint32_t pack_start_code = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    if(pack_start_code == PS_HEADER_STARTCODE){
        return 1;
    }
    return 0;
}
static int mpeg2_is_ps_header_end(uint8_t *buffer, int len){
    if(!buffer || len < 4){
        return -1;
    }
    uint32_t pack_start_code = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    if(pack_start_code == PS_HEADER_ENDCODE){
        return 1;
    }
    return 0;
}
static int mpeg2_psm_pes_parse(mpeg2_ps_context *context, uint8_t *pes_ptr, int pes_ptr_len){
    if(!context || !pes_ptr){
        return -1;
    }
    int used_bytes = 0;
    /**
     * case 1.mpeg1: no have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE. mpeg2core_pes.h
     *               file: ../media/h265_mpeg1.ps
     * case 2.mpeg1: no have psm, pes type-->PRIVATE_STREAM_1\PADDING_STREAM\PRIVATE_STREAM_2\PES_H264_ID\PES_AC3_ID\PES_DTS_ID\PES_LPCM_ID\PES_SUB_ID. mpeg2core_type.h
     *               file: ../media/h264_mpeg1.ps
     * case 3.mpeg2: have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE . mpeg2core_pes.h
     *               file: ../media/h264_mpeg2.ps
     */
    int mpeg1_pes_flag = mpeg2_is_pes_mpeg1(pes_ptr, pes_ptr_len);
    if(mpeg1_pes_flag < 0){
        return -1;
    }
    if(mpeg1_pes_flag == 1){
        used_bytes = mpeg2_ps_mpeg1_media_parse(context, pes_ptr, pes_ptr_len); // case 2
        if(used_bytes < 0){
            return -1;
        }
        return 0;
    }
    int psm_flag = mpeg2_is_psm(pes_ptr, pes_ptr_len);
    if(psm_flag < 0){
        return -1;
    }
    if(psm_flag == 1){ // psm
        used_bytes = mpeg2_psm_parse(pes_ptr, pes_ptr_len, &context->psm); // case 3
        if(used_bytes <= 0){
            return -1;
        }
    }
    else{ // media
        used_bytes = mpeg2_ps_media_parse(context, pes_ptr, pes_ptr_len); // case 1 or case 3
        if(used_bytes < 0){
            return -1;
        }
    }
    return 0;
}
static int mpeg2_ps_packer_parse(mpeg2_ps_context *context){
    if(!context){
        return -1;
    }
    uint32_t start_code = 0;
    int used_bytes = 0;
    while(1){
        if(context->demuxer_stat == PS_HEADER){
            used_bytes = mpeg2_ps_header_parse(context->ps_buffer, context->ps_buffer_pos, &context->ps_header);
            if(used_bytes < 0){
                return -1;
            }
            if(context->ps_header.ready_flag){
                if(mpeg2_buffer_used_move(context, used_bytes) < 0){
                    return -1;
                }
                if(context->ps_header.is_mpeg2){
                    context->demuxer_stat = PS_STUFFING;
                }
                else{
                    context->demuxer_stat = PS_BODY;
                }
            }
            else{
                goto need_more_data;
            }
            // dump_ps_header(context->ps_header);
        }
        if(context->demuxer_stat == PS_STUFFING){
            if(context->ps_buffer_pos >= context->ps_header.pack_stuffing_length){
                if(mpeg2_buffer_used_move(context, context->ps_header.pack_stuffing_length) < 0){
                    return -1;
                }
                context->demuxer_stat = PS_BODY;
            }
            else{
                goto need_more_data;
            }
        }
        if(context->demuxer_stat == PS_BODY){
            // system header check start
            if(context->ps_buffer_pos < 4){
                goto need_more_data;
            }
            int system_flag = mpeg2_is_system_header(context->ps_buffer, context->ps_buffer_pos);
            if(system_flag < 0){
                return -1;
            }
            if(system_flag == 1){
                used_bytes = mpeg2_system_header_parse(context->ps_buffer, context->ps_buffer_pos, &context->ps_system_header);
                if(used_bytes < 0){
                    return -1;
                }
                if(context->ps_system_header.ready_flag){
                    if(mpeg2_buffer_used_move(context, used_bytes) < 0){
                        return -1;
                    }
                }
                else{
                    goto need_more_data;
                }
            }
            // system header check over

            // pes(psm) handle
            if(context->ps_buffer_pos < 4){
                goto need_more_data;
            }
            /**
             * case 1.mpeg1: no have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE. mpeg2core_pes.h
             *               file: ../media/h265_mpeg1.ps
             * case 2.mpeg1: no have psm, pes type-->PRIVATE_STREAM_1\PADDING_STREAM\PRIVATE_STREAM_2\PES_H264_ID\PES_AC3_ID\PES_DTS_ID\PES_LPCM_ID\PES_SUB_ID. mpeg2core_type.h
             *               file: ../media/h264_mpeg1.ps
             * case 3.mpeg2: have psm, pes type-->PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE . mpeg2core_pes.h
             *               file: ../media/h264_mpeg2.ps
             */
            int pes_flag = mpeg2_is_pes_or_psm(context->ps_buffer, context->ps_buffer_pos); // case 1 or case 3
            int mpeg1_pes_flag = mpeg2_is_pes_mpeg1(context->ps_buffer, context->ps_buffer_pos); // case 2
            int mpeg_flag = pes_flag;
            if(pes_flag < 0 || mpeg1_pes_flag < 0){
                return -1;
            }
            if(pes_flag == 0 && mpeg1_pes_flag == 0){ // The current format is not supported
                return -1;
            }
            int find_flag = 0;
            uint8_t *ptr = context->ps_buffer + 4;
            int ptr_len = context->ps_buffer_pos - 4;
            uint8_t *pes_start_ptr = context->ps_buffer;
            int ps_header_flag = 0;
            int ps_end_flag = 0;
            pes_flag = 0;
            mpeg1_pes_flag = 0;
            // search for the next PES packet
            while(ptr_len >= 4){
                if(mpeg_flag == 1){
                    pes_flag = mpeg2_is_pes_or_psm(ptr, ptr_len);
                }
                else{
                    mpeg1_pes_flag = mpeg2_is_pes_mpeg1(ptr, ptr_len);
                }
                ps_header_flag = mpeg2_is_ps_header(ptr, ptr_len);
                ps_end_flag = mpeg2_is_ps_header_end(ptr, ptr_len);
                if(pes_flag < 0 || mpeg1_pes_flag < 0 || ps_header_flag < 0 || ps_end_flag < 0){
                    return -1;
                }
                if(pes_flag == 1 || mpeg1_pes_flag == 1 || ps_header_flag == 1 || ps_end_flag == 1){
                    find_flag = 1;
                    uint8_t *pes_ptr = pes_start_ptr;
                    int pes_ptr_len = ptr - pes_start_ptr;
                    // handle case 1, case 2, case 3
                    if(mpeg2_psm_pes_parse(context, pes_ptr, pes_ptr_len) < 0){
                        return -1;
                    }
                    if(mpeg2_buffer_used_move(context, pes_ptr_len) < 0){
                        return -1;
                    }
                    if(pes_flag == 1 || mpeg1_pes_flag == 1){
                        ptr = context->ps_buffer + 4;
                        ptr_len = context->ps_buffer_pos - 4;
                        pes_start_ptr = context->ps_buffer;
                    }
                    if(ps_header_flag == 1){
                        context->demuxer_stat = PS_HEADER;
                        break;
                    }
                    if(ps_end_flag == 1){
                        if(mpeg2_buffer_used_move(context, context->ps_buffer_pos) < 0){
                            return -1;
                        }
                        return 0;
                    }
                }
                ptr++;
                ptr_len--;
            }
            if(find_flag == 0){
                goto need_more_data;
            }
            
        }
    }
need_more_data:
    return 0;
}
int mpeg2_ps_packet_demuxer(mpeg2_ps_context *context, uint8_t *buffer, int len){
    if(!context || (buffer == NULL) || (len < 0)){
        return -1;
    }
    if(context->ps_buffer_len < context->ps_buffer_pos + len){
        context->ps_buffer = (uint8_t *)realloc(context->ps_buffer, context->ps_buffer_pos + len + 1024);
        context->ps_buffer_len = context->ps_buffer_pos + len + 1024;
    }
    memcpy(context->ps_buffer + context->ps_buffer_pos, buffer, len);
    context->ps_buffer_pos += len;
    if(mpeg2_ps_packer_parse(context) < 0){
        return -1;
    }
    return 0;
}

int mpeg2_ps_add_stream(mpeg2_ps_context *context, int stream_type, uint8_t *stream_info, int stream_info_len){
    if(!context || context->psm.psm_stream_array_num >= PS_STREAM_MAX){
        return -1;
    }
    context->psm.psm_stream_array[context->psm.psm_stream_array_num].stream_type = stream_type;
    if(mpeg2_is_video(stream_type) == 1){
        context->psm.psm_stream_array[context->psm.psm_stream_array_num].elementary_stream_id = PES_VIDEO;
    }
    else if(mpeg2_is_audio(stream_type) == 1){
        context->psm.psm_stream_array[context->psm.psm_stream_array_num].elementary_stream_id = PES_AUDIO;
    }
    else{
        return -1;
    }
    if(stream_info != NULL){
        memcpy(context->psm.psm_stream_array[context->psm.psm_stream_array_num].descriptor, stream_info, stream_info_len);
        context->psm.psm_stream_array[context->psm.psm_stream_array_num].elementary_stream_info_length = stream_info_len;
    }
    context->psm.psm_stream_array_num++;
    return 0;
}
static int mpeg2_ps_find_stream(mpeg2_ps_context *context, int stream_type, mpeg2_psm_stream **psm_stream){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->psm.psm_stream_array_num; i++){
        if(stream_type == context->psm.psm_stream_array[i].stream_type){
            *psm_stream = &context->psm.psm_stream_array[i];
            return 0;
        }
    }
    return -1;
}
int mpeg2_ps_header_pack(uint8_t *buffer, int len, mpeg2_ps_header ps_header, int over_flag){
    if(!buffer || (len < (PS_HEADER_FIXED_HEADER + ps_header.pack_stuffing_length))){
        return -1;
    }
    int idx = 0;
    buffer[idx] = 0;
    buffer[idx + 1] = 0;
    buffer[idx + 2] = 0x01;
    buffer[idx + 3] = (over_flag == 1) ? 0xB9 : 0xBA;
    idx += 4;
    // system_clock_reference_base-->system_clock_reference_extension 6 bytes
    int system_clock_reference_extension = 0;
    buffer[idx] = 0x44 | (((ps_header.system_clock_reference_base >> 30) & 0x07) << 3) | ((ps_header.system_clock_reference_base >> 28) & 0x03);
    buffer[idx + 1] = ((ps_header.system_clock_reference_base >> 20) & 0xFF);
    buffer[idx + 2] = 0x04 | (((ps_header.system_clock_reference_base >> 15) & 0x1F) << 3) | ((ps_header.system_clock_reference_base >> 13) & 0x03);
    buffer[idx + 3] = ((ps_header.system_clock_reference_base >> 5) & 0xFF);
    buffer[idx + 4] = 0x04 | ((ps_header.system_clock_reference_base & 0x1F) << 3) | ((system_clock_reference_extension >> 7) & 0x03);
    buffer[idx + 5] = 0x01 | ((system_clock_reference_extension & 0x7F) << 1);
    idx += 6;
    // program_mux_rate = 255\marker_bit = 1\marker_bit = 1
    int program_mux_rate = 6106;
    buffer[idx] = (uint8_t)(program_mux_rate >> 14);
    buffer[idx + 1] = (uint8_t)(program_mux_rate >> 6);
    buffer[idx + 2] = (uint8_t)((program_mux_rate << 2) | 0x03);
    idx += 3;
    // reserved\pack_stuffing_length
    buffer[idx] = ps_header.pack_stuffing_length & 0x07;
    idx++;
    memset(&buffer[idx], 0xff, ps_header.pack_stuffing_length);
    idx += ps_header.pack_stuffing_length;
    return idx;
}
static int mpeg2_audio_pes_pack(mpeg2_ps_context *context, mpeg2_pes_header pes_header, uint8_t *buffer, int len){
    if(context == NULL){
        return -1;
    }
    pes_header.stream_id = PES_AUDIO;
    context->pes_buffer_pos_a = mpeg2_pes_packet_pack(pes_header, context->pes_buffer_a, sizeof(context->pes_buffer_a), buffer, len);
    if(context->pes_buffer_pos_a < 0){
        return -1;
    }
    context->audio_frame_cnt++;
    if(((context->audio_frame_cnt % context->psm_period) == 0) && (context->video_frame_cnt == 0)){
        context->psm_flag = 1;
    }
    return 0;
}
static int mpeg2_video_pes_pack(mpeg2_ps_context *context, mpeg2_pes_header pes_header){
    if(context == NULL){
        return -1;
    }
    pes_header.stream_id = PES_VIDEO;
    context->pes_buffer_pos_v = mpeg2_pes_packet_pack(pes_header, context->pes_buffer_v, sizeof(context->pes_buffer_v), context->frame_buffer, context->frame_buffer_len);
    if(context->pes_buffer_pos_v < 0){
        return -1;
    }
    context->video_frame_cnt++;
    if(context->video_frame_cnt % context->psm_period == 0){
        context->psm_flag = 1;
    }
    // reset frame_buffer
    memset(context->frame_buffer, 0 ,sizeof(context->frame_buffer));
    context->frame_buffer_len = 0;
    return 0;
}
static int mpeg2core_h264_check_cache(mpeg2_ps_context *context){
    if(!context){
        return 0;
    }
    if(context->frame_buffer_len > 0){
        uint8_t *ptr = context->frame_buffer;
        int ptr_len = context->frame_buffer_len;
        int start_code = get_start_code(ptr, ptr_len);
        int cache_nalu_type = ptr[start_code] & 0x1f;
        if(cache_nalu_type == H264_NAL_AUD){
            ptr = context->frame_buffer + mpeg2_h264_aud_size();
            ptr_len = context->frame_buffer_len - mpeg2_h264_aud_size();
            start_code = get_start_code(ptr, ptr_len);
            cache_nalu_type = ptr[start_code] & 0x1f;
        }
        if(!(cache_nalu_type == H264_NAL_SEI || cache_nalu_type == H264_NAL_SPS || cache_nalu_type == H264_NAL_PPS)){
            return 1;
        }
    }
    return 0;
}
static int mpeg2core_h265_check_cache(mpeg2_ps_context *context){
    if(!context){
        return 0;
    }
    if(context->frame_buffer_len > 0){
        uint8_t *ptr = context->frame_buffer;
        int ptr_len = context->frame_buffer_len;
        int start_code = get_start_code(ptr, ptr_len);
        int cache_nalu_type = (ptr[start_code] >> 1) & 0x3f;
        if(cache_nalu_type == H265_NAL_AUD){
            ptr = context->frame_buffer + mpeg2_h265_aud_size();
            ptr_len = context->frame_buffer_len - mpeg2_h265_aud_size();
            start_code = get_start_code(ptr, ptr_len);
            cache_nalu_type = (ptr[start_code] >> 1) & 0x3f;
        }
        if(!(cache_nalu_type == H265_NAL_VPS || cache_nalu_type == H265_NAL_SPS || cache_nalu_type == H265_NAL_PPS 
            || cache_nalu_type == H265_NAL_SEI_PREFIX || cache_nalu_type == H265_NAL_SEI_SUFFIX)){
            return 1;
        }
    }
    return 0;
}
static int mpeg2_ps_pack(mpeg2_ps_context *context, int stream_type, int psm_flag){
    if(!context){
        return -1;
    }
    mpeg2_ps_header ps_header;
    memset(&ps_header, 0, sizeof(mpeg2_ps_header));
    memset(context->ps_buffer, 0, context->ps_buffer_len);
    // ps header
    ps_header.system_clock_reference_base = context->pts;
    ps_header.pack_stuffing_length = 0;
    int used_bytes = mpeg2_ps_header_pack(context->ps_buffer, context->ps_buffer_len, ps_header, 0);
    if(used_bytes < 0){
        return -1;
    }
    int pack_ret = 0;
    if((context->write_init == 0) || (psm_flag == 1)){
        // system header
        mpeg2_ps_system_header ps_system_header;
        memset(&ps_system_header, 0, sizeof(mpeg2_ps_system_header));
        pack_ret = mpeg2_system_header_pack(context->ps_buffer + used_bytes, context->ps_buffer_len - used_bytes, ps_system_header);
        if(pack_ret < 0){
            return -1;
        }
        used_bytes += pack_ret;
        // psm
        pack_ret = mpeg2_psm_pack(context->ps_buffer + used_bytes, context->ps_buffer_len - used_bytes, context->psm);
        if(pack_ret < 0){
            return -1;
        }
        used_bytes += pack_ret;
        context->write_init == 1;
    }
    int left_bytes = context->ps_buffer_len - used_bytes;
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MP3:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
            if(left_bytes < context->pes_buffer_pos_a){
                return -1;
            }
            memcpy(context->ps_buffer + used_bytes, context->pes_buffer_a, context->pes_buffer_pos_a);
            used_bytes += context->pes_buffer_pos_a;
            if(context->media_write_callback){
                context->media_write_callback(stream_type, context->ps_buffer, used_bytes, context->arg);
            }
            break;
        case STREAM_TYPE_VIDEO_H264:
        case STREAM_TYPE_VIDEO_HEVC:
            if(left_bytes < context->pes_buffer_pos_v){
                return -1;
            }
            memcpy(context->ps_buffer + used_bytes, context->pes_buffer_v, context->pes_buffer_pos_v);
            used_bytes += context->pes_buffer_pos_v;
            if(context->media_write_callback){
                context->media_write_callback(stream_type, context->ps_buffer, used_bytes, context->arg);
            }
            break;
        default:
            break;
    }
    return 0;
}
int mpeg2_ps_packet_muxer(mpeg2_ps_context *context, uint8_t *buffer, int len, int type, int64_t pts, int64_t dts){
    if(!context || (buffer == NULL) || (len <= 0)){
        return -1;
    }
    mpeg2_psm_stream *psm_stream = NULL;
    if(mpeg2_ps_find_stream(context, type, &psm_stream) < 0){
        return -1;
    }
    context->pts = pts;
    context->dts = dts;
    mpeg2_pes_header pes_header;
    memset(&pes_header, 0, sizeof(mpeg2_pes_header));
    pes_header.PTS_DTS_flags = 3; // 3:pts + dts 2:pts
    pes_header.pts = pts;
    pes_header.dts = dts;
    pes_header.write_packet_length_flag = 1; // must set 1
    int video_nalu_type;
    int start_code = 0;
    int write_flag = 0;
    switch (type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MP3:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
            context->psm_stream_audio = *psm_stream;
            pes_header.PTS_DTS_flags = 2; // only pts
            pes_header.dts = 0;
            context->dts = 0;
            if(mpeg2_audio_pes_pack(context, pes_header, buffer, len) < 0){
                return -1;
            }
            write_flag = 1;
            context->key_flag = 1;
            break;
        case STREAM_TYPE_VIDEO_H264:
            context->psm_stream_video = *psm_stream;
            start_code = get_start_code(buffer, len);
            video_nalu_type = buffer[start_code] & 0x1f;
            if(video_nalu_type == H264_NAL_AUD){ // skip AUD
                return 0;
            }
            if(video_nalu_type == H264_NAL_SEI || video_nalu_type == H264_NAL_SPS || video_nalu_type == H264_NAL_PPS){ // SEI/SPS/PPS
                // chech cache
                if(mpeg2core_h264_check_cache(context)){
                    write_flag = 1;
                    if(mpeg2_video_pes_pack(context, pes_header) < 0){
                        return -1;
                    }
                }
                if(mpeg2_h264_new_access_unit(buffer, len)){
                    context->frame_buffer_len += mpeg2_add_h264_aud(context->frame_buffer + context->frame_buffer_len, sizeof(context->frame_buffer) - context->frame_buffer_len);
                }
                if(len > (sizeof(context->frame_buffer) - context->frame_buffer_len)){
                    return -1;
                }
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len; 
                context->psm_flag = 1;
                return 0;
            }
            else{ // I/P/B
                if(video_nalu_type == H264_NAL_IDR){
                    context->key_flag = 1;
                }
                if(mpeg2_h264_new_access_unit(buffer, len)){
                    context->frame_buffer_len += mpeg2_add_h264_aud(context->frame_buffer + context->frame_buffer_len, sizeof(context->frame_buffer) - context->frame_buffer_len);
                    write_flag = 1;
                    // New image, write the history cache and then cache again
                    if(mpeg2_video_pes_pack(context, pes_header) < 0){
                        return -1;
                    }
                }
                if(len > (sizeof(context->frame_buffer) - context->frame_buffer_len)){
                    return -1;
                }
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len;
            }
            break;
        case STREAM_TYPE_VIDEO_HEVC:
            context->psm_stream_video = *psm_stream;
            start_code = get_start_code(buffer, len);
            video_nalu_type = (buffer[start_code] >> 1) & 0x3f;
            if(video_nalu_type == H265_NAL_AUD){ // skip AUD
                return 0;
            }
            if(video_nalu_type == H265_NAL_VPS || video_nalu_type == H265_NAL_SPS || video_nalu_type == H265_NAL_PPS 
                || video_nalu_type == H265_NAL_SEI_PREFIX || video_nalu_type == H265_NAL_SEI_SUFFIX){ // VPS/SPS/PPS/SEI
                // chech cache
                if(mpeg2core_h265_check_cache(context)){
                    write_flag = 1;
                    if(mpeg2_video_pes_pack(context, pes_header) < 0){
                        return -1;
                    }
                }
                if(mpeg2_h265_new_access_unit(buffer, len)){
                    context->frame_buffer_len += mpeg2_add_h265_aud(context->frame_buffer + context->frame_buffer_len, sizeof(context->frame_buffer) - context->frame_buffer_len);
                }
                if(len > (sizeof(context->frame_buffer) - context->frame_buffer_len)){
                    return -1;
                }
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len;
                context->psm_flag = 1;
                return 0;
            }
            else{
                if(video_nalu_type == H265_NAL_IDR_W_RADL || video_nalu_type == H265_NAL_IDR_N_LP || video_nalu_type == H265_NAL_CRA){
                    context->key_flag = 1;
                }
                if(mpeg2_h265_new_access_unit(buffer, len)){
                    context->frame_buffer_len += mpeg2_add_h265_aud(context->frame_buffer + context->frame_buffer_len, sizeof(context->frame_buffer) - context->frame_buffer_len);
                    write_flag = 1;
                    // New image, write the history cache and then cache again
                    if(mpeg2_video_pes_pack(context, pes_header) < 0){
                        return -1;
                    }
                }
                if(len > (sizeof(context->frame_buffer) - context->frame_buffer_len)){
                    return -1;
                }
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len;
            }
            break;
        default:
            break;
    }
    if(write_flag == 0){
        return 0;
    }
    if(mpeg2_ps_pack(context, type, context->psm_flag) < 0){
        return -1;
    }
    context->psm_flag = 0;
    context->key_flag = 0;
    return 0;
}
static int mpeg2_ps_muxer_cache_clean(mpeg2_ps_context *context){
    if(!context){
        return -1;
    }
    if(context->frame_buffer_len <= 0){
        return 0;
    }
    mpeg2_pes_header pes_header;
    memset(&pes_header, 0, sizeof(mpeg2_pes_header));
    pes_header.PTS_DTS_flags = 3; // 3:pts + dts 2:pts
    pes_header.pts = context->pts;
    pes_header.dts = context->dts;
    pes_header.stream_id = PES_VIDEO;
    context->pes_buffer_pos_v = mpeg2_pes_packet_pack(pes_header, context->pes_buffer_v, sizeof(context->pes_buffer_v), context->frame_buffer, context->frame_buffer_len);
    if(context->pes_buffer_pos_v < 0){
        return -1;
    }
    context->video_frame_cnt++;
    context->frame_buffer_len = 0;
    if(mpeg2_ps_pack(context, context->psm_stream_video.stream_type, context->psm_flag) < 0){;
        return -1;
    }
    return 0;
}
void destroy_ps_context(mpeg2_ps_context *context){
    if(!context){
        return;
    }
    // muxer cache cleaning
    mpeg2_ps_muxer_cache_clean(context);
    // muxer, end of file
    mpeg2_ps_header ps_header;
    memset(&ps_header, 0, sizeof(mpeg2_ps_header));
    memset(context->ps_buffer, 0, context->ps_buffer_len);
    ps_header.system_clock_reference_base = context->pts;
    ps_header.pack_stuffing_length = 0;
    int used_bytes = mpeg2_ps_header_pack(context->ps_buffer, context->ps_buffer_len, ps_header, 1);
    if(context->media_write_callback && used_bytes > 0){
        context->media_write_callback(0, context->ps_buffer, used_bytes, context->arg);
    }
    if(context->ps_buffer){
        free(context->ps_buffer);
    }
    free(context);
    
}

