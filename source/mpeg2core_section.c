#include "mpeg2core_internal.h"

int mpeg2_section_header_parse(uint8_t *buffer, int len, mpeg2_section_header *section_header){
    if(len < SECTION_HEADER_SIZE){
        return -1;
    }
    int idx = 0;
    section_header->table_id = buffer[idx];
    idx++;
    section_header->section_syntax_indicator = (buffer[idx] & 0x80) >> 7;
    section_header->zero = 0;
    section_header->reserved1 = (buffer[idx] & 0x30) >> 4;
    section_header->section_length = ((buffer[idx] & 0x0f) << 8) | buffer[idx + 1];
    idx += 2;
    section_header->transport_stream_id = (buffer[idx] << 8) | buffer[idx + 1];
    idx += 2;
    section_header->reserved2 = (buffer[idx] & 0xc0) >> 6;
    section_header->version_number = (buffer[idx] & 0x3e) >> 1;
    section_header->current_next_indicator = buffer[idx] & 0x01;
    idx++;
    section_header->section_number = buffer[idx];
    idx++;
    section_header->last_section_number = buffer[idx];
    idx++;
    return 0;
}
int mpeg2_get_one_section(mpeg2_ts_context *context){
    if((context == NULL) || (context->ts_header.payload == NULL) || (context->ts_header.payload_len <= 0)){
        return -1;
    }
    uint8_t *section_payload = context->ts_header.payload;
    int section_payload_len = context->ts_header.payload_len;
    if(context->ts_header.payload_unit_start_indicator == 1){ // pointer_field only in the first package
        section_payload += context->ts_header.payload[0]/*pointer_field len*/ + 1/*pointer_field*/;
        section_payload_len -= context->ts_header.payload[0]/*pointer_field len*/ + 1/*pointer_field*/;
        if(mpeg2_section_header_parse(section_payload, section_payload_len, &context->section_header) < 0){
            return -1;
        }
        context->section_buffer_pos = 0;
    }
    else{ // A section is divided into multiple TS packets，there is no section header(similar to TS pack PES)
        // do nothing
    }
    int remaining_bytes = context->section_header.section_length + 3/*table_id-->section_length*/ - context->section_buffer_pos;
    if(remaining_bytes <= section_payload_len){ // complete
        memcpy(context->section_buffer + context->section_buffer_pos, section_payload, remaining_bytes);
        context->section_buffer_pos += remaining_bytes;
    }
    else{
        memcpy(context->section_buffer + context->section_buffer_pos, section_payload, section_payload_len);
        context->section_buffer_pos += section_payload_len;
    }
    return 0;
}
int mpeg2_get_one_complete_section(mpeg2_ts_context *context){
    if(context == NULL){
        return -1;
    }
    if((context->section_header.section_length != 0) && ((context->section_header.section_length + 3/*table_id-->section_length*/) <= context->section_buffer_pos)){
        return 0;
    }
    return -1;
}
int mpeg2_section_header_pack(uint8_t *buffer, int len, mpeg2_section_header section_header){
    if(len < SECTION_HEADER_SIZE){
        return -1;
    }
    int idx = 0;
    // table_id
    buffer[idx] = section_header.table_id;
    idx++;
    // section_syntax_indicator\zero\reserved1\section_length11_8
    buffer[idx] = 0x80 | ((section_header.section_length & 0x0f00) >> 8);
    idx++;
    // section_length7_0
    buffer[idx] = section_header.section_length & 0xff;
    idx++;
    // transport_stream_id
    buffer[idx] = section_header.transport_stream_id >> 8;
    buffer[idx + 1] = section_header.transport_stream_id & 0xff;
    idx += 2;
    // reserved2\version_number\current_next_indicator
    buffer[idx] = 0x01;
    idx++;
    // section_number
    buffer[idx] = section_header.section_number;
    idx++;
    // last_section_number
    buffer[idx] = section_header.last_section_number;
    idx++;
    return idx;
}