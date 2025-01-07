#include "mpeg2core_ps.h"
#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"
#include "mpeg2core_crc32.h"
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
    psm->current_next_indicator = (buffer[idx] & 0x80) >> 7;
    psm->reserved1 = (buffer[idx] & 0x60) >> 5;
    psm->program_stream_map_version = buffer[idx] & 0x1f;
    idx++;
    psm->reserved2 = (buffer[idx] & 0xfe) >> 1;
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
    psm->crc_32 = (buffer[idx] << 24) | (buffer[idx + 1] << 16) | (buffer[idx + 2] << 8) | buffer[idx + 3];
    idx += 4;
    psm->ready_flag = 1;
    return idx;
}

int mpeg2_psm_pack(uint8_t *buffer, int len, mpeg2_psm psm){
    if(!buffer || len < PS_PSM_FIXED_LEN){
        return -1;
    }
    int idx = 0;
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0x01;
    idx += 3;
    // map_stream_id
    buffer[idx] = PSM_MAP_STREAM_ID;
    idx++;
    // program_stream_map_length
    int program_stream_map_length = PS_PSM_FIXED_LEN - 6/*packet_start_code_prefix-->program_stream_map_length*/ + 4/*stream_map*/ * psm.psm_stream_array_num;
    buffer[idx] = (program_stream_map_length >> 8) & 0xff;
    buffer[idx + 1] = program_stream_map_length & 0xff;
    idx += 2;
    // current_next_indicator = 1\reserved\program_stream_map_version = 1
    buffer[idx] = 0x81;
    idx++;
    // reserved\marker_bit
    buffer[idx] = 0xff;
    idx++;
    // program_stream_info_length
    buffer[idx] = buffer[idx + 1] = 0;
    idx += 2;
    // elementary_stream_map_length
    int elementary_stream_map_length = 4/*stream_map*/ * psm.psm_stream_array_num;
    buffer[idx] = (elementary_stream_map_length >> 8) & 0xff;
    buffer[idx + 1] = elementary_stream_map_length & 0xff;
    idx += 2;
    // stream map
    for(int i = 0; i < psm.psm_stream_array_num; i++){
        mpeg2_psm_stream psm_stream = psm.psm_stream_array[i];
        // stream_type
        buffer[idx] = psm_stream.stream_type;
        idx++;
        // elementary_stream_id
        buffer[idx] = psm_stream.elementary_stream_id;
        idx++;
        // elementary_stream_info_length
        buffer[idx] = buffer[idx + 1] = 0;
        idx += 2;
    }
    //CRC_32
    uint32_t crc_32 = mpeg2core_crc32(0xffffffff, buffer, idx);
    buffer[idx] = crc_32 & 0xff;
    buffer[idx + 1] = (crc_32 >> 8) & 0xff;
    buffer[idx + 2] = (crc_32 >> 16) & 0xff;
    buffer[idx + 3] = (crc_32 >> 24) & 0xff;
    idx += 4;
    return idx;
}