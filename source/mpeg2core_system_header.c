#include "mpeg2core_ps.h"
#include "mpeg2core_internal.h"

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