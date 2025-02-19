#include "mpeg2core_internal.h"
#include "mpeg2core_crc32.h"

int mpeg2_pmt_parse(mpeg2_ts_context *context){
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
    if(context->section_header.table_id != TID_PMT){
        return -1;
    }
    if(context->pmt_array[context->current_pmt_idx].version_number == INITIAL_VERSION){
        context->pmt_array[context->current_pmt_idx].version_number = context->section_header.version_number;
    }
    if(context->section_header.version_number != context->pmt_array[context->current_pmt_idx].version_number){ // version has changed
        memset(&context->pmt_array[context->current_pmt_idx], 0, sizeof(mpeg2_sdt));
        context->pmt_array[context->current_pmt_idx].version_number = context->section_header.version_number;
    }
    // If the version has not changed, there is no need to re parse the sdt
    if(context->pmt_array[context->current_pmt_idx].pmt_ready == 1){
        return 0;
    }
    for(int i = 0; i < context->pmt_array[context->current_pmt_idx].section_number_array_num; i++){
        if(context->pmt_array[context->current_pmt_idx].section_number_array[i] == context->section_header.section_number){ // repeat
            return 0;
        }
    }
    context->pmt_array[context->current_pmt_idx].section_number_array[context->pmt_array[context->current_pmt_idx].section_number_array_num++] = context->section_header.section_number;
    // parse
    mpeg2_pmt pmt = context->pmt_array[context->current_pmt_idx];
    int pmt_len = context->section_header.section_length - 5/*transport_stream_id-->last_section_number*/ - 4/*CRC_32*/;
    uint8_t *pmt_payload_start = context->section_buffer + SECTION_HEADER_SIZE;
    uint8_t *pmt_payload_end = context->section_buffer + SECTION_HEADER_SIZE + pmt_len;
    pmt.reserved1 = (pmt_payload_start[0] & 0xe0) >> 5;
    pmt.PCR_PID = ((pmt_payload_start[0] & 0x1f) << 8) | pmt_payload_start[1];
    pmt.reserved2 = (pmt_payload_start[2] & 0xf0) >> 4;
    pmt.program_info_length = ((pmt_payload_start[2] & 0x0f) << 8) | pmt_payload_start[3];
    pmt_payload_start += 4;
    memcpy(pmt.descriptor, pmt_payload_start, pmt.program_info_length);
    pmt_payload_start += pmt.program_info_length;
    while((pmt_payload_end - pmt_payload_start) >= 5){
        mpeg2_pmt_stream pmt_stream;
        pmt_stream.stream_type = pmt_payload_start[0];
        pmt_stream.reserved1 = (pmt_payload_start[1] & 0xe0) >> 5;
        pmt_stream.elementary_PID = ((pmt_payload_start[1] & 0x1f) << 8) | pmt_payload_start[2];
        pmt_stream.reserved2 = (pmt_payload_start[3] & 0xf0) >> 4;
        pmt_stream.ES_info_length = ((pmt_payload_start[3] & 0x0f) << 8) | pmt_payload_start[4];
        memcpy(pmt_stream.descriptor, pmt_payload_start + 5, pmt_stream.ES_info_length);
        pmt_payload_start += 5 + pmt_stream.ES_info_length;
        pmt.pmt_stream_array[pmt.pmt_stream_array_num++] = pmt_stream;
        
    }
    if(pmt_payload_start != pmt_payload_end){
        return -1;
    }
    uint32_t CRC_32 = (pmt_payload_start[0] << 24) | (pmt_payload_start[1] << 16) | (pmt_payload_start[2] << 8) | pmt_payload_start[3];
    pmt.pid = context->pmt_array[context->current_pmt_idx].pid;
    context->pmt_array[context->current_pmt_idx] = pmt;
    // All sections have been received
    if(context->pmt_array[context->current_pmt_idx].section_number_array_num == (context->section_header.last_section_number + 1)){
        context->pmt_array[context->current_pmt_idx].pmt_ready = 1;
    }
    return 0;
}
int mpeg2_pmt_pack(mpeg2_pmt pmt, uint8_t *buffer, int len){
    if(buffer == NULL){
        return -1;
    }
    int media_cnt = pmt.pmt_stream_array_num;
    // section
    mpeg2_section_header section_header;
    memset(&section_header, 0, sizeof(mpeg2_section_header));
    section_header.table_id = TID_PMT;
    section_header.section_length = 5/*section:transport_stream_id-->last_section_number*/ + 4/*reserved1-->program_info_length*/ + (5 * media_cnt)/*stream_type-->ES_info_length*/ + 4/*CRC_32*/;
    if((TS_PACKET_LENGTH_188 - TS_FIXED_HEADER_LEN - section_header.section_length - 3/*table_id-->section_length*/) < 0){ // too long, too many streams
        return -1;
    }
    section_header.transport_stream_id = pmt.program_number;
    section_header.section_number = 0;
    section_header.last_section_number = 0;
    int ret = mpeg2_section_header_pack(buffer, len, section_header);
    if(ret < 0){
        return -1;
    }
    // reserved1
    buffer[ret] = 0;
    // PCR_PID
    buffer[ret] = (pmt.pcr_pid & 0x1f00) >> 8;
    buffer[ret + 1] = pmt.pcr_pid & 0xff;
    ret += 2;
    // reserved2
    buffer[ret] = 0;
    // program_info_length
    buffer[ret] = 0;
    buffer[ret + 1] = 0;
    ret += 2;
    for(int i = 0; i < pmt.pmt_stream_array_num; i++){
        mpeg2_pmt_stream pmt_stream =  pmt.pmt_stream_array[i];
        buffer[ret] = pmt_stream.stream_type;
        ret++;
        // reserved1
        buffer[ret] = 0;
        // elementary_PID
        buffer[ret] = (pmt_stream.elementary_PID & 0x1f00) >> 8;
        buffer[ret + 1] = pmt_stream.elementary_PID & 0xff;
        ret += 2;
        // reserved2
        buffer[ret] = 0;
        // ES_info_length
        buffer[ret] = 0;
        buffer[ret + 1] = 0;
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