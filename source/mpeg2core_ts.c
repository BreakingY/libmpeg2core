#include "mpeg2core_ts.h"
#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
#include "mpeg2core_common.h"
mpeg2_ts_context *create_ts_context(){
    mpeg2_ts_context *context = (mpeg2_ts_context *)malloc(sizeof(mpeg2_ts_context));
    if(!context){
        return NULL;
    }
    memset(context, 0, sizeof(mpeg2_ts_context));
    context->pat.version_number = INITIAL_VERSION;
    context->sdt.version_number = INITIAL_VERSION;
    context->pmt.version_number = INITIAL_VERSION;
    for(int i = 0; i < PAT_PROGARM_MAX; i++){
        context->pmt_array[i].version_number = INITIAL_VERSION;
    }
    context->pcr_period = 1; // frames
    context->pat_period = 25; // frames
    return context;
}
void mpeg2_set_read_callback(mpeg2_ts_context *context, VideoReadCallback video_read_callback, AudioReadCallback audio_read_callback, void *arg){
    if(!context){
        return;
    }
    context->video_read_callback = video_read_callback;
    context->audio_read_callback = audio_read_callback;
    context->arg = arg;
    return;
}
void mpeg2_set_write_callback(mpeg2_ts_context *context, MediaWriteCallback media_write_callback, void *arg){
    if(!context){
        return;
    }
    context->media_write_callback = media_write_callback;
    context->arg = arg;
    return;
}
void destroy_ts_context(mpeg2_ts_context *context){
    if(context){
        free(context);
    }
}
void dump_section_header(mpeg2_section_header section_header){
    printf("===================section header==================\n");
    printf("table_id:%0x\n", section_header.table_id);
    printf("section_syntax_indicator:%d\n", section_header.section_syntax_indicator);
    printf("zero:%d\n", section_header.zero);
    printf("reserved1:%d\n", section_header.reserved1);
    printf("section_length:%d\n", section_header.section_length);
    printf("transport_stream_id:%d\n", section_header.transport_stream_id);
    printf("reserved2:%d\n", section_header.reserved2);
    printf("version_number:%d\n", section_header.version_number);
    printf("current_next_indicator:%d\n", section_header.current_next_indicator);
    printf("section_number:%d\n", section_header.section_number);
    printf("last_section_number:%d\n", section_header.last_section_number);
    printf("===================================================\n");
    return;
}
void dump_ts_header(mpeg2_ts_header ts_header){
    printf("=====================ts header=====================\n");
    printf("--------fixed--------\n");
    printf("sync_byte:%0x\n", ts_header.sync_byte);
    printf("transport_error_indicator:%d\n", ts_header.transport_error_indicator);
    printf("payload_unit_start_indicator:%d\n", ts_header.payload_unit_start_indicator);
    printf("transport_priority:%d\n", ts_header.transport_priority);
    printf("pid:%d\n", ts_header.pid);
    printf("transport_scrambling_control:%d\n", ts_header.transport_scrambling_control);
    printf("adaptation_field_control:%0x\n", ts_header.adaptation_field_control);
    printf("continuity_counter:%d\n", ts_header.continuity_counter);
    if(ts_header.adaptation_field_control >= 2){
        printf("--------adaptation--------\n");
        mpeg2_ts_adaptation_header adaptation = ts_header.adaptation;
        printf("adaptation_field_length:%d\n", adaptation.adaptation_field_length);
        printf("discontinuity_indicator:%d\n", adaptation.discontinuity_indicator);
        printf("random_access_indicator:%d\n", adaptation.random_access_indicator);
        printf("elementary_stream_priority_indicator:%d\n", adaptation.elementary_stream_priority_indicator);
        printf("PCR_flag:%d\n", adaptation.PCR_flag);
        printf("OPCR_flag:%d\n", adaptation.OPCR_flag);
        printf("splicing_point_flag:%d\n", adaptation.splicing_point_flag);
        printf("transport_private_data_flag:%d\n", adaptation.transport_private_data_flag);
        printf("adaptation_field_extension_flag:%d\n", adaptation.adaptation_field_extension_flag);
    }
    
    printf("PCR: %" PRIu64 "\n", ts_header.PCR);
    printf("OPCR: %" PRIu64 "\n", ts_header.OPCR);

    printf("payload_len:%d\n", ts_header.payload_len);
    printf("===================================================\n");
    return;
}
void dump_program(mpeg2_pat pat){
    printf("=====================program=======================\n");
    for(int i = 0; i < pat.program_array_num; i++){
        printf("program_number:%d\n", pat.program_array[i].program_number);
        printf("program_map_pid(PMT PID):%d\n", pat.program_array[i].program_map_pid);
    }
    printf("===================================================\n");
    return;
}
void dump_pmt_array(mpeg2_pmt *pmt_array, int pmt_array_num){
    if(!pmt_array){
        return;
    }
    printf("====================pmt_array======================\n");
    for(int i = 0; i < pmt_array_num; i++){
        mpeg2_pmt pmt = pmt_array[i];
        printf("pid:%d\n", pmt.pid);
        printf("program_number:%d\n", pmt.program_number);
        printf("reserved1:%d\n", pmt.reserved1);
        printf("PCR_PID:%d\n", pmt.PCR_PID);
        printf("reserved2:%d\n", pmt.reserved2);
        printf("program_info_length:%d\n", pmt.program_info_length);
        // printf("descriptor:%s\n", pmt.descriptor);
        for(int j = 0; j < pmt.pmt_stream_array_num; j++){
            mpeg2_pmt_stream pmt_stream = pmt.pmt_stream_array[j];
            printf("    stream_type:%d\n", pmt_stream.stream_type);
            printf("    reserved1:%d\n", pmt_stream.reserved1);
            printf("    elementary_PID:%d\n", pmt_stream.elementary_PID);
            printf("    reserved2:%d\n", pmt_stream.reserved2);
            printf("    ES_info_length:%d\n", pmt_stream.ES_info_length);
            // printf("    descriptor:%s\n", pmt_stream.descriptor);
        }
    }
    printf("===================================================\n");
    return;
}
int mpeg2_ts_header_parse(uint8_t *buffer, int len, mpeg2_ts_header *ts_header){
    if((buffer == NULL) || ((len != TS_PACKET_LENGTH_188) && (len != TS_PACKET_LENGTH_204))){
        return -1;
    }
    int idx = 0;
    mpeg2_ts_adaptation_header adaptation;
    memset(&adaptation, 0, sizeof(adaptation));
    ts_header->payload = NULL;
    ts_header->payload_len = 0;
    // fixed
    ts_header->sync_byte = buffer[idx];
    idx++;
    ts_header->transport_error_indicator = (buffer[idx] & 0x80) >> 7;
    ts_header->payload_unit_start_indicator = (buffer[idx] & 0x40) >> 6;
    ts_header->transport_priority = (buffer[idx] & 0x20) >> 5;
    ts_header->pid = ((buffer[idx] & 0x1f) << 8) | buffer[idx + 1];
    idx += 2;
    ts_header->transport_scrambling_control = (buffer[idx] & 0xc0) >> 6;
    ts_header->adaptation_field_control = (buffer[idx] & 0x30) >> 4;
    ts_header->continuity_counter = buffer[idx] & 0x0f;
    idx++;
    
    switch (ts_header->adaptation_field_control){
        case 0:
            return -1;
            break;
        case 1: // only payload 
            ts_header->payload = buffer + 4/*fixed header*/;
            ts_header->payload_len = len - 4;
            break;
        case 2: // only adaptation
        case 3: // adaptation + payload
            // adaptation
            adaptation.adaptation_field_length = buffer[idx];
            idx++;
            adaptation.discontinuity_indicator = (buffer[idx] & 0x80) >> 7;
            adaptation.random_access_indicator = (buffer[idx] & 0x40) >> 6;
            adaptation.elementary_stream_priority_indicator = (buffer[idx] & 0x20) >> 5;
            adaptation.PCR_flag = (buffer[idx] & 0x10) >> 4;
            adaptation.OPCR_flag = (buffer[idx] & 0x08) >> 3;
            adaptation.splicing_point_flag = (buffer[idx] & 0x04) >> 2;
            adaptation.transport_private_data_flag = (buffer[idx] & 0x02) >> 1;
            adaptation.adaptation_field_extension_flag = buffer[idx] & 0x01;
            idx++;
            ts_header->adaptation = adaptation;
            if(ts_header->adaptation_field_control == 3){
                ts_header->payload = buffer + 4/*fixed header*/ + 1/*adaptation_field_length*/ + adaptation.adaptation_field_length;
                ts_header->payload_len = len - (4/*fixed header*/ + 1/*adaptation_field_length*/ + adaptation.adaptation_field_length);
            }
            break;
        default:
            break;
    }
    // optional field
    /**
     * Specifically:
     * PCR_ base(i) = ((system_clock_frequency * t(i)) DIV 300) % 2^33     (2-1)
     * PCR_ ext(i) = ((system_clock_frequency * t(i)) DIV 1) % 300         (2-2)
     * PCR(i) = PCR_ base(i) * 300 + PCR_ ext(i)                           (2-3)
     * double temp = ((_hour * 60) + _minute) * 60 + _second;
     * _PCR_base = (unsigned long long)((27000000 * temp) / 300) & 0x1ffffffff;//2^33 //1<<33
     * _PCR_ext = ((unsigned long long)(27000000 * temp) / 1) % 300l;
     * _PCR = _PCR_base * 300 + _PCR_ext;
     */
    if(adaptation.PCR_flag){
        struct PCR *pcr = (struct PCR *)&buffer[idx];
        idx += 6;
        ts_header->PCR = ((pcr->pcr_base32_25 << 25) | (pcr->pcr_base24_17 << 17) | (pcr->pcr_base16_9 << 9) | (pcr->pcr_base8_1 << 1) | (pcr->pcr_base0)) * 300 + 
                        ((pcr->pcr_extension8 << 8) | pcr->pcr_extension7_0);
    }
    if(adaptation.OPCR_flag){
        struct OPCR *opcr = (struct OPCR *)&buffer[idx];
        idx += 6;
        ts_header->OPCR = (opcr->opcr_base32_25 << 25) | (opcr->opcr_base24_17 << 17) | (opcr->opcr_base16_9 << 9) | (opcr->opcr_base8_1 << 1) | (opcr->opcr_base0) * 300 +
                            ((opcr->opcr_extension8 << 8) | opcr->opcr_extension7_0);
    }
    return 0;
}
int mpeg2_ts_packet_demuxer(mpeg2_ts_context *context, uint8_t *buffer, int len){
    if(!context || (buffer == NULL) || ((len != TS_PACKET_LENGTH_188) && (len != TS_PACKET_LENGTH_204))){
        return -1;
    }
    memset(&context->ts_header, 0, sizeof(mpeg2_ts_header));
    if(mpeg2_ts_header_parse(buffer, len, &context->ts_header) < 0){
        return -1;
    }
    mpeg2_ts_header ts_header = context->ts_header;
    if(ts_header.transport_error_indicator == 1){ // incorrect, skip
        return 0;
    }
    // PSI/SI
    switch (ts_header.pid){
        case PID_PAT:
            if(mpeg2_pat_parse(context) < 0){
                return -1;
            }
            break;
        case PID_SDT:
            if(mpeg2_sdt_parse(context) < 0){
                return -1;
            }
            break;
        default:
            break;
    }
    // PMT
    for(int i = 0; i < context->pmt_array_num; i++){
        uint16_t pid = context->pmt_array[i].pid;
        if(pid == ts_header.pid){
            context->current_pmt_idx = i;
            if(mpeg2_pmt_parse(context) < 0){
                return -1;
            }
        }
    }
    // media
    for(int pmt_idx = 0; pmt_idx < context->pmt_array_num; pmt_idx++){
        context->current_pmt_idx = pmt_idx;
        context->pmt = context->pmt_array[pmt_idx];
        for(int i = 0; i < context->pmt.pmt_stream_array_num; i++){
            mpeg2_pmt_stream pmt_stream = context->pmt.pmt_stream_array[i];
            if(ts_header.pid != pmt_stream.elementary_PID){
                continue;
            }
            switch (pmt_stream.stream_type){
                case STREAM_TYPE_AUDIO_AAC:
                case STREAM_TYPE_AUDIO_MPEG1:
                case STREAM_TYPE_AUDIO_MPEG2:
                case STREAM_TYPE_AUDIO_AAC_LATM:
                case STREAM_TYPE_AUDIO_G711A:
                case STREAM_TYPE_AUDIO_G711U:
                    if(mpeg2_ts_audio_parse(context, pmt_stream.stream_type, pmt_stream.elementary_PID) < 0){
                        return -1;
                    }
                    break;
                case STREAM_TYPE_VIDEO_H264:
                case STREAM_TYPE_VIDEO_HEVC:
                    if(mpeg2_ts_video_parse(context, pmt_stream.stream_type, pmt_stream.elementary_PID) < 0){
                        return -1;
                    }
                    break;
                default:
                    break;
            }
        }
        
    }
    return 0;
}

int mpeg2_ts_add_program(mpeg2_ts_context *context, uint16_t program_number, uint8_t* info, int len){
    if(!context){
        return -1;
    }
    if(context->pat.program_array_num >= PAT_PROGARM_MAX){
        return -1;
    }
    int pmt_pid = PID_PMT + context->pat.program_array_num;
    context->pat.program_array[context->pat.program_array_num].program_number = program_number;
    context->pat.program_array[context->pat.program_array_num].program_map_pid = pmt_pid;
    context->pat.program_array_num++;
    context->pmt_array[context->pmt_array_num].program_number = program_number;
    context->pmt_array[context->pmt_array_num].pid = pmt_pid;
    if(info && (len <= sizeof(context->pmt_array[context->pmt_array_num].descriptor))){
        memcpy(context->pmt_array[context->pmt_array_num].descriptor, info, len);
        context->pmt_array[context->pmt_array_num].program_info_length = len;
    }
    context->pmt_array_num++;

    return 0;
}

int mpeg2_ts_remove_program(mpeg2_ts_context *context, uint16_t program_number){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->pat.program_array_num; i++){
        if(context->pat.program_array[i].program_number == program_number){
            if (i + 1 < context->pat.program_array_num){
			    memmove(&context->pat.program_array[i], &context->pat.program_array[i + 1], (context->pat.program_array_num - i - 1) * sizeof(context->pat.program_array[0]));
                memmove(&context->pmt_array[i], &context->pmt_array[i + 1], (context->pmt_array_num - i - 1) * sizeof(context->pmt_array[0]));
            }
            context->pat.program_array_num--;
            context->pmt_array_num--;
            return 0;
        }
    }
    return -1;
}

int mpeg2_ts_add_program_stream(mpeg2_ts_context *context, uint16_t program_number, int stream_type, uint8_t* stream_info, int stream_info_len){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->pmt_array_num; i++){
        if(context->pmt_array[i].program_number == program_number){
            if(context->pmt_array[i].pmt_stream_array_num >= PMT_STREAM_MAX){
                return -1;
            }
            int stream_pid = PID_MEDIA + context->total_streams;
            context->pmt_array[i].pmt_stream_array[context->pmt_array[i].pmt_stream_array_num].stream_type = stream_type;
            context->pmt_array[i].pmt_stream_array[context->pmt_array[i].pmt_stream_array_num].elementary_PID = stream_pid;
            if(stream_info && (stream_info_len <= sizeof(context->pmt_array[i].pmt_stream_array[context->pmt_array[i].pmt_stream_array_num].descriptor))){
                memcpy(context->pmt_array[i].pmt_stream_array[context->pmt_array[i].pmt_stream_array_num].descriptor, stream_info, stream_info_len);
                context->pmt_array[i].pmt_stream_array[context->pmt_array[i].pmt_stream_array_num].ES_info_length = stream_info_len;
            }
            if(mpeg2_is_video(stream_type) || (context->pmt_array[i].pcr_pid == 0)){
                context->pmt_array[i].pcr_pid = stream_pid;
            }
            context->pmt_array[i].pmt_stream_array_num++;
            context->total_streams++;
            return stream_pid;
        }
    }
    return -1;
}

int mpeg2_find_pmt(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->pmt_array_num; i++){
        for(int j = 0; j < context->pmt_array[i].pmt_stream_array_num; j++){
            mpeg2_pmt_stream pmt_stream_tmp = context->pmt_array[i].pmt_stream_array[j];
            if(pmt_stream_tmp.elementary_PID == stream_pid){
                *pmt = &context->pmt_array[i];
                *pmt_stream = &context->pmt_array[i].pmt_stream_array[j];
                return 0;
            }
        }
    }
    return -1;
}
int mpeg2_increase_stream_frame_cnt(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->pmt_array_num; i++){
        for(int j = 0; j < context->pmt_array[i].pmt_stream_array_num; j++){
            if(context->pmt_array[i].pmt_stream_array[j].elementary_PID == stream_pid){
                context->pmt_array[i].pmt_stream_array[j].frame_cnt++;
                *pmt = &context->pmt_array[i];
                *pmt_stream = &context->pmt_array[i].pmt_stream_array[j];
                return 0;
            }
        }
    }
    return -1;
}
int mpeg2_increase_stream_continuity_counter(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream){
    if(!context){
        return -1;
    }
    for(int i = 0; i < context->pmt_array_num; i++){
        for(int j = 0; j < context->pmt_array[i].pmt_stream_array_num; j++){
            if(context->pmt_array[i].pmt_stream_array[j].elementary_PID == stream_pid){
                context->pmt_array[i].pmt_stream_array[j].stream_continuity_counter++;
                if(context->pmt_array[i].pmt_stream_array[j].stream_continuity_counter > 0x0f){
                    context->pmt_array[i].pmt_stream_array[j].stream_continuity_counter = 0;
                }
                *pmt = &context->pmt_array[i];
                *pmt_stream = &context->pmt_array[i].pmt_stream_array[j];
                return 0;
            }
        }
    }
    return -1;
}
int mpeg2_ts_header_pack(uint8_t *buffer, int len, mpeg2_ts_header ts_header, int psi_si, int stuffing_bytes){
    if((buffer == NULL) || ((len != TS_PACKET_LENGTH_188) && (len != TS_PACKET_LENGTH_204))){
        return -1;
    }
    memset(buffer, 0, len);
    int idx = 0;
    // sync_byte
    buffer[idx] = ts_header.sync_byte;
    idx++;
    // transport_error_indicator
    buffer[idx] |= ts_header.transport_error_indicator << 7;
    // payload_unit_start_indicator
    buffer[idx] |= ts_header.payload_unit_start_indicator << 6;
    // transport_priority
    buffer[idx] |= ts_header.transport_priority << 5;
    // PID
    buffer[idx] |= (ts_header.pid & 0x1f00) >> 8;
    buffer[idx + 1] = ts_header.pid & 0xff;
    idx += 2;
    // transport_scrambling_control
    buffer[idx] |= ts_header.transport_scrambling_control << 6;
    // adaptation_field_control
    buffer[idx] |= ts_header.adaptation_field_control << 4;
    // continuity_counter
    buffer[idx] |= ts_header.continuity_counter & 0x0f;
    idx++;
    uint64_t pcr_base;
	uint64_t pcr_ext;
    uint64_t opcr_base;
	uint64_t opcr_ext;
    switch (ts_header.adaptation_field_control){ // video audio
        case 0:
            return -1;
            break;
        case 1: // only payload 
            break;
        case 2: // only adaptation
            return -1; // not support
            break;
        case 3: // adaptation + payload
            // adaptation_field_length
            buffer[idx] = 1/*discontinuity_indicator-->5 flags*/ + stuffing_bytes; // video audio stuff
            if(ts_header.adaptation.PCR_flag){
                buffer[idx] += 6;
            }
            if(ts_header.adaptation.OPCR_flag){
                buffer[idx] += 6;
            }
            idx++;
            // discontinuity_indicator\random_access_indicator\elementary_stream_priority_indicator\5 flags
            buffer[idx] = 0x00;
            if(ts_header.adaptation.random_access_indicator){ // video IDR
                buffer[idx] |= 0x40;
            }
            if(ts_header.adaptation.PCR_flag){
                buffer[idx] |= 0x10;
            }
            if(ts_header.adaptation.OPCR_flag){
                buffer[idx] |= 0x08;
            }
            idx++;
            if(ts_header.adaptation.PCR_flag){
                pcr_base = ts_header.PCR / 300;
	            pcr_ext = ts_header.PCR % 300;
                buffer[idx] = (pcr_base >> 25) & 0xFF;
                buffer[idx + 1] = (pcr_base >> 17) & 0xFF;
                buffer[idx + 2] = (pcr_base >> 9) & 0xFF;
                buffer[idx + 3] = (pcr_base >> 1) & 0xFF;
                buffer[idx + 4] = ((pcr_base & 0x01) << 7) | 0x7E | ((pcr_ext >> 8) & 0x01);
                buffer[idx + 5] = pcr_ext & 0xFF;
                idx += 6;
            }
            if(ts_header.adaptation.OPCR_flag){
                opcr_base = ts_header.OPCR / 300;
	            opcr_ext = ts_header.OPCR % 300;
                buffer[idx] = (opcr_base >> 25) & 0xFF;
                buffer[idx + 1] = (opcr_base >> 17) & 0xFF;
                buffer[idx + 2] = (opcr_base >> 9) & 0xFF;
                buffer[idx + 3] = (opcr_base >> 1) & 0xFF;
                buffer[idx + 4] = ((opcr_base & 0x01) << 7) | 0x7E | ((opcr_ext >> 8) & 0x01);
                buffer[idx + 5] = opcr_ext & 0xFF;
                idx += 6;
            }
            break;
        default:
            break;
    }
    // point_field
    if(psi_si){
        buffer[idx] = 0;
        idx++;
    }
    return idx;
}
static int mpeg2_ts_pack_psi_si(mpeg2_ts_context *context, mpeg2_ts_header ts_header){
    int ts_header_len = mpeg2_ts_header_pack(context->ts_buffer, sizeof(context->ts_buffer), ts_header, 1, 0);
    if(ts_header_len < 0){
        return -1;
    }
    memcpy(context->ts_buffer + ts_header_len, context->section_buffer, context->section_buffer_pos);
    memset(context->ts_buffer + ts_header_len + context->section_buffer_pos, 0xff , sizeof(context->ts_buffer) - (ts_header_len + context->section_buffer_pos));
    if(context->media_write_callback){
        context->media_write_callback(0, 0, 0, context->ts_buffer, sizeof(context->ts_buffer), context->arg);
    }
    return 0;
}
static int mpeg2_ts_pack(mpeg2_ts_context *context, mpeg2_pmt *pmt, mpeg2_pmt_stream *pmt_stream, int stream_type, int psi_flag){
    if(!context){
        return -1;
    }
    mpeg2_ts_header ts_header;
    memset(&ts_header, 0, sizeof(mpeg2_ts_header));
    ts_header.sync_byte = 0x47;
    ts_header.transport_error_indicator = 0;
    ts_header.payload_unit_start_indicator = 1;
    ts_header.transport_priority = 0;
    ts_header.transport_scrambling_control = 0;
    ts_header.continuity_counter = 0;
    int ts_header_len = 0;
    int stuffing_bytes = 0;
    if((context->write_init == 0) || (psi_flag == 1)){
        // if(context->write_init == 0){
        // SDT
        memset(context->section_buffer, 0, sizeof(context->section_buffer));
        context->section_buffer_pos = mpeg2_sdt_pack(context->section_buffer, sizeof(context->section_buffer));
        if(context->section_buffer_pos < 0){
            return -1;
        }
        ts_header.pid = PID_SDT;
        ts_header.continuity_counter = context->sdt_continuity_counter;
        context->sdt_continuity_counter++;
        if(context->sdt_continuity_counter > 0x0f){
            context->sdt_continuity_counter = 0;
        }
        ts_header.adaptation_field_control = 1; // only paylod
        if(mpeg2_ts_pack_psi_si(context, ts_header) < 0){
            return -1;
        }
        context->write_init = 1;
        // }
        // PAT
        memset(context->section_buffer, 0, sizeof(context->section_buffer));
        context->section_buffer_pos = mpeg2_pat_pack(context->pat, context->section_buffer, sizeof(context->section_buffer));
        if(context->section_buffer_pos < 0){
            return -1;
        }
        ts_header.pid = PID_PAT;
        ts_header.continuity_counter = context->pat_continuity_counter;
        context->pat_continuity_counter++;
        if(context->pat_continuity_counter > 0x0f){
            context->pat_continuity_counter = 0;
        }
        ts_header.adaptation_field_control = 1; // only paylod
        if(mpeg2_ts_pack_psi_si(context, ts_header) < 0){
            return -1;
        }
        // PMT
        for(int i = 0; i < context->pmt_array_num; i++){
            mpeg2_pmt pmt = context->pmt_array[i];
            memset(context->section_buffer, 0, sizeof(context->section_buffer));
            context->section_buffer_pos = mpeg2_pmt_pack(pmt, context->section_buffer, sizeof(context->section_buffer));
            if(context->section_buffer_pos < 0){
                return -1;
            }
            ts_header.pid = PID_PMT;
            ts_header.continuity_counter = context->pmt_array[i].pmt_continuity_counter;
            context->pmt_array[i].pmt_continuity_counter++;
            if(context->pmt_array[i].pmt_continuity_counter > 0x0f){
                context->pmt_array[i].pmt_continuity_counter = 0;
            }
            ts_header.adaptation_field_control = 1; // only paylod
            if(mpeg2_ts_pack_psi_si(context, ts_header) < 0){
                return -1;
            }
        }
    }
    // PES to TS
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MPEG2:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
            if(mpeg2_ts_media_pack(context, pmt, pmt_stream) < 0){
                return -1;
            }
            break;
        case STREAM_TYPE_VIDEO_H264:
        case STREAM_TYPE_VIDEO_HEVC:
            if(mpeg2_ts_media_pack(context, pmt, pmt_stream) < 0){
                return -1;
            }
            break;
        default:
            break;
    }
    return 0;
}
static int mpeg2_audio_pes_pack(mpeg2_ts_context *context, mpeg2_pes_header pes_header, uint8_t *buffer, int len){
    if(context == NULL){
        return -1;
    }
    pes_header.stream_id = PES_AUDIO;
    context->pes_buffer_pos_a = mpeg2_pes_packet_pack(pes_header, context->pes_buffer_a, sizeof(context->pes_buffer_a), buffer, len);
    if(context->pes_buffer_pos_a < 0){
        return -1;
    }
    context->audio_frame_cnt++;
    if(((context->audio_frame_cnt % context->pat_period) == 0) && (context->video_frame_cnt == 0)){
        context->psi_flag = 1;
    }
    return 0;
}
static int mpeg2_video_pes_pack(mpeg2_ts_context *context, mpeg2_pes_header pes_header){
    if(context == NULL){
        return -1;
    }
    pes_header.stream_id = PES_VIDEO;
    context->pes_buffer_pos_v = mpeg2_pes_packet_pack(pes_header, context->pes_buffer_v, sizeof(context->pes_buffer_v), context->frame_buffer, context->frame_buffer_len);
    if(context->pes_buffer_pos_v < 0){
        return -1;
    }
    context->video_frame_cnt++;
    if(context->video_frame_cnt % context->pat_period == 0){
        context->psi_flag = 1;
    }
    // reset frame_buffer
    memset(context->frame_buffer, 0 ,sizeof(context->frame_buffer));
    context->frame_buffer_len = 0;
    return 0;
}
static int mpeg2core_h264_check_cache(mpeg2_ts_context *context){
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
static int mpeg2core_h265_check_cache(mpeg2_ts_context *context){
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
int mpeg2_ts_packet_muxer(mpeg2_ts_context *context, int stream_pid, uint8_t *buffer, int len, int type, int64_t pts, int64_t dts){
    if(!context || (buffer == NULL) || (len <= 0)){
        return -1;
    }
    mpeg2_pmt *pmt = NULL;
    mpeg2_pmt_stream *pmt_stream = NULL;
    if(mpeg2_find_pmt(context, stream_pid, &pmt, &pmt_stream) < 0){
        return -1;
    }
    if(pmt_stream->stream_type != type || pmt_stream->elementary_PID != stream_pid){
        return -1;
    }
    context->pts = pts;
    context->dts = dts;
    mpeg2_pes_header pes_header;
    memset(&pes_header, 0, sizeof(mpeg2_pes_header));
    pes_header.PTS_DTS_flags = 3; // 3:pts + dts 2:pts
    pes_header.pts = pts;
    pes_header.dts = dts;
    int video_nalu_type;
    int start_code = 0;
    int write_flag = 0;
    switch (type){
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MPEG2:
        case STREAM_TYPE_AUDIO_AAC_LATM:
        case STREAM_TYPE_AUDIO_G711A:
        case STREAM_TYPE_AUDIO_G711U:
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
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len; 
                context->psi_flag = 1;
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
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len;
            }
            break;
        case STREAM_TYPE_VIDEO_HEVC:
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
                memcpy(context->frame_buffer + context->frame_buffer_len, buffer, len);
                context->frame_buffer_len += len;
                context->psi_flag = 1;
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
    if(mpeg2_ts_pack(context, pmt, pmt_stream, type, context->psi_flag) < 0){
        return -1;
    }
    context->psi_flag = 0;
    context->key_flag = 0;
    context->last_pts = context->pts;
    context->last_dts = context->dts;
    return 0;
}
