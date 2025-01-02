#include "mpeg2core_ps.h"
#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
int mpeg2_psm_parse(uint8_t *buffer, int len, mpeg2_psm *psm){
    memset(psm, 0, sizeof(mpeg2_psm));
    if(!buffer || (len < PS_PSM_FIXED_LEN)){
        return 0;
    }
    int idx = 0;
    psm->packet_start_code_prefix = (buffer[idx] << 16) | (buffer[idx + 1] << 8) | buffer[idx + 2];
    if(psm->packet_start_code_prefix != PES_STARTCODE){
        return -1;
    }
    idx += 3;
    psm->map_stream_id = buffer[idx];
    idx++;
    psm->program_stream_map_length = (buffer[idx] << 8) | buffer[idx + 1];
    idx += 2;
    psm->current_next_indicator = buffer[idx] & 0x10;
    psm->reserved1 = buffer[idx] & 0x60;
    psm->program_stream_map_version = buffer[idx] & 0x1f;
    idx++;
    psm->reserved2 = buffer[idx] & 0xfe;
    psm->marker_bit = buffer[idx] & 0x01;
    idx++;
    psm->program_stream_info_length = (buffer[idx] << 8) | buffer[idx + 1];
    idx += 2;
    if((len - idx) < psm->program_stream_info_length){
        return 0;
    }
    memcpy(psm->descriptor, buffer + idx, psm->program_stream_info_length);
    idx += psm->program_stream_info_length;
    if((len - idx) < 2){
        return 0;
    }
    psm->elementary_stream_map_length = (buffer[idx] << 8) | buffer[idx + 1];
    idx += 2;
    if((len - idx) < psm->elementary_stream_map_length){
        return 0;
    }
    int stream_bytes = 0;
    while(1){
        mpeg2_psm_stream psm_stream;
        psm_stream.stream_type = buffer[idx];
        idx++;
        psm_stream.elementary_stream_id = buffer[idx];
        idx++;
        psm_stream.elementary_stream_info_length = (buffer[idx] << 8) | buffer[idx + 1];
        idx += 2;
        if((len - idx) < psm_stream.elementary_stream_info_length){
            return 0;
        }
        memcpy(psm_stream.descriptor, buffer + idx, psm_stream.elementary_stream_info_length);
        idx += psm_stream.elementary_stream_info_length;
        if(psm->psm_stream_array_num < sizeof(psm->psm_stream_array)){
            psm->psm_stream_array[psm->psm_stream_array_num++] = psm_stream;
        }
        if(idx >= (psm->elementary_stream_map_length + 12/*packet_start_code_prefix-->elementary_stream_map_length*/ + psm->program_stream_info_length)){
            break;
        }
    }
    if((len - idx) < 4){
        return 0;
    }
    psm->crc_32 = (buffer[idx] << 24) | (buffer[idx + 1] << 16) | (buffer[idx + 2] << 8) || buffer[idx + 3];
    idx += 4;
    psm->ready_flag = 1;
    return idx;
}