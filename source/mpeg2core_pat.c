#include "mpeg2core_internal.h"
#include "mpeg2core_crc32.h"

int mpeg2_pat_parse(mpeg2_ts_context *context){
    if(context == NULL){
        return -1;
    }
    if((context->ts_header.payload == NULL) || (context->ts_header.payload_len <= 0)){
        return 0;
    }
    if(mpeg2_get_one_section(context) < 0){
        return -1;
    }
    if(mpeg2_get_one_complete_section(context) < 0){ // The complete section has not been received, continue parsing the TS packets.
        return 0;
    }
    if(context->section_header.table_id != TID_PAT){
        return -1;
    }
    if(context->pat.version_number == INITIAL_VERSION){
        context->pat.version_number = context->section_header.version_number;
    }
    if(context->section_header.version_number != context->pat.version_number){ // version has changed
        memset(&context->pat, 0, sizeof(mpeg2_pat));
        context->pat.version_number = context->section_header.version_number;
        // reset pmt
        context->pmt_array_num = 0;
        memset(&context->pmt, 0, sizeof(mpeg2_pmt));
        context->pmt.version_number = INITIAL_VERSION;
        for(int i = 0; i < PAT_PROGARM_MAX; i++){
            memset(&context->pmt_array[i], 0, sizeof(mpeg2_pmt));
            context->pmt_array[i].version_number = INITIAL_VERSION;
        }
    }
    // If the version has not changed, there is no need to re parse the pat
    if(context->pat.pat_ready == 1){
        return 0;
    }
    for(int i = 0; i < context->pat.section_number_array_num; i++){
        if(context->pat.section_number_array[i] == context->section_header.section_number){ // repeat
            return 0;
        }
    }
    context->pat.section_number_array[context->pat.section_number_array_num++] = context->section_header.section_number;
    // parse program
    int program_len = context->section_header.section_length - 5/*transport_stream_id-->last_section_number*/ - 4/*CRC_32*/;
    uint8_t *program_payload_start = context->section_buffer + SECTION_HEADER_SIZE;
    uint8_t *program_payload_end = context->section_buffer + SECTION_HEADER_SIZE + program_len;
    while((program_payload_end - program_payload_start) >= 4){
        mpeg2_program program;
        program.program_number = (program_payload_start[0] << 8) | program_payload_start[1];
        program.reserved = (program_payload_start[2] & 0xc0) >> 5;
        uint16_t pid = ((program_payload_start[2] & 0x1f) << 8) | program_payload_start[3];
        if(program.program_number == 0){
            context->pat.network_pid = pid;
        }
        else{
            program.program_map_pid = pid;
        }
        context->pat.program_array[context->pat.program_array_num++] = program;
        program_payload_start += 4;
    }
    if(program_payload_start != program_payload_end){
        return -1;
    }
    uint32_t CRC_32 = (program_payload_start[0] << 24) | (program_payload_start[1] << 16) | (program_payload_start[2] << 8) | program_payload_start[3];
    // All sections have been received
    if(context->pat.section_number_array_num == (context->section_header.last_section_number + 1)){
        context->pat.pat_ready = 1;
        // PMT
        for(int i = 0; i < context->pat.program_array_num; i++){
            mpeg2_program program = context->pat.program_array[i];
            uint16_t pmt_pid = program.program_map_pid;
            uint16_t program_number = program.program_number;
            context->pmt_array[context->pmt_array_num].pid = pmt_pid;
            context->pmt_array[context->pmt_array_num].program_number = program_number;
            context->pmt_array_num++;
        }
    }
    return 0;
}
int mpeg2_pat_pack(mpeg2_pat pat, uint8_t *buffer, int len){
    if(buffer == NULL){
        return -1;
    }
    // section
    mpeg2_section_header section_header;
    memset(&section_header, 0, sizeof(mpeg2_section_header));
    section_header.table_id = TID_PAT;
    section_header.section_length = 5/*section:transport_stream_id-->last_section_number*/ + 4 * pat.program_array_num/*program*/ + 4/*CRC_32*/;
    if((TS_PACKET_LENGTH_188 - TS_FIXED_HEADER_LEN - section_header.section_length - 3/*table_id-->section_length*/) < 0){ // too long, too many programs
        return -1;
    }
    section_header.transport_stream_id = TRANSPORT_STREAM_ID;
    section_header.section_number = 0;
    section_header.last_section_number = 0;
    int ret = mpeg2_section_header_pack(buffer, len, section_header);
    if(ret < 0){
        return -1;
    }
    for(int i = 0; i < pat.program_array_num; i++){
        int program_number = pat.program_array[i].program_number;
        int program_map_pid = pat.program_array[i].program_map_pid;
        // program_number
        buffer[ret] = (program_number & 0xff00) >> 8;
        buffer[ret + 1] = program_number & 0xff;
        ret += 2;
        // reserved\program_map_pid
        buffer[ret] = 0;
        buffer[ret] = (program_map_pid & 0x1f00) >> 8;
        buffer[ret + 1] = program_map_pid & 0xff;
        ret += 2;
    }
    //CRC_32
    uint32_t crc_32 = mpeg2core_crc32(0xffffffff, buffer, ret);
    buffer[ret] = crc_32 & 0xff;
    buffer[ret + 1] = (crc_32 >> 8) & 0xff;
    buffer[ret + 2] = (crc_32 >> 16) & 0xff;
    buffer[ret + 3] = (crc_32 >> 24) & 0xff;
    ret += 4;
    return ret;
}