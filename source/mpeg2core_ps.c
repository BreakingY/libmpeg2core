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
void destroy_ps_context(mpeg2_ps_context *context){
    if(context){
        if(context->ps_buffer){
            free(context->ps_buffer);
        }
        free(context);
    }
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

