#include "mpeg2core_ps.h"
#include "mpeg2core_internal.h"
#include "mpeg2core_pes.h"

int mpeg2_is_system_header(uint8_t *buffer, int len){
    if(!buffer || len < 4){
        return -1;
    }
    uint32_t system_header_start_code = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    if(system_header_start_code == PS_SYSTEM_HEADER_STARTCODE){
        return 1;
    }
    return 0;
}
int mpeg2_system_header_parse(uint8_t *buffer, int len, mpeg2_ps_system_header *ps_system_header){
    memset(ps_system_header, 0, sizeof(mpeg2_ps_system_header));
    if(!buffer || (len < PS_SYSTEM_HEADER_FIXED_LEN)){
        return 0;
    }
    int idx = 0;
    ps_system_header->system_header_start_code = (buffer[idx] << 24) | (buffer[idx + 1] << 16) | (buffer[idx + 2] << 8) | buffer[idx + 3];
    if(ps_system_header->system_header_start_code != PS_SYSTEM_HEADER_STARTCODE){
        return -1;
    }
    idx += 4;
    ps_system_header->header_length = (buffer[idx] << 8) | buffer[idx + 1];
    idx += 2;
    if(len < (ps_system_header->header_length + 6)){
        return 0;
    }
    // skip system header
    ps_system_header->ready_flag = 1;
    return idx + ps_system_header->header_length;
}

int mpeg2_system_header_pack(uint8_t *buffer, int len, mpeg2_ps_system_header ps_system_header){
    if(!buffer || len < PS_SYSTEM_HEADER_FIXED_LEN){
        return -1;
    }
    int sys_header_len = PS_SYSTEM_HEADER_FIXED_LEN - 6/*system_header_start_code and header_length*/ + 3/*stream_id-->STD_buffer_size_bound*/ * 2;
    int idx = 0;
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0x01;
    buffer[3] = 0xBB;
    idx += 4;
    // header_length
    buffer[idx] = sys_header_len >> 8;
    buffer[idx + 1] = sys_header_len & 0xff;
    idx += 2;
    // marker_bit = 1\rate_bound\marker_bit = 1
    int rate_bound = 50000;
    buffer[idx] = 0x80 | ((rate_bound >> 15) & 0x7f);
    buffer[idx + 1] = (rate_bound >> 7) & 0xff;
    buffer[idx + 2] = ((rate_bound & 0x7f) << 1) | 0x01;
    idx += 3;
    // audio_bound = 1\fixed_flag = 0\CSPS_flag = 1
    buffer[idx] = 0x05;
    idx++;
    // system_audio_lock_flag = 1\system_video_lock_flag = 1\marker_bit = 1\video_bound = 1
    buffer[idx] = 0xe1;
    idx++;
    // packet_rate_restriction_flag = 0\reserved_bits
    buffer[idx] = 0x7f;
    idx++;
    /*audio stream bound*/
    // stream_id
    buffer[idx] = PES_AUDIO;
    idx++;
    // 11\P-STD_buffer_bound_scale = 0\P-STD_buffer_size_bound = 512
    buffer[idx] = 0xc0 | (512 & 0x1f00 >> 8);
    buffer[idx + 1] = 512 & 0xff;
    idx += 2;
    /*video stream bound*/
    buffer[idx] = PES_VIDEO;
    idx++;
    // 11\P-STD_buffer_bound_scale = 1\P-STD_buffer_size_bound = 2048
    buffer[idx] = 0xe0 | (2048 & 0x1f00 >> 8);
    buffer[idx + 1] = 2048 & 0xff;
    idx += 2;
    return idx;
}