#include "mpeg2core_internal.h"
#include "crc32.h"
#define SERVICE_ENCODER "encoder"

int mpeg2_sdt_parse(mpeg2_ts_context *context){
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
    if(context->section_header.table_id != TID_SDT){ // ignore, may be BAT or ST(pid equal to sdt)
        return 0;
    }
    if(context->sdt.version_number == INITIAL_VERSION){
        context->sdt.version_number = context->section_header.version_number;
    }
    if(context->section_header.version_number != context->sdt.version_number){ // version has changed
        memset(&context->sdt, 0, sizeof(mpeg2_sdt));
        context->sdt.version_number = context->section_header.version_number;
    }
    // If the version has not changed, there is no need to re parse the sdt
    if(context->sdt.sdt_ready == 1){
        return 0;
    }
    for(int i = 0; i < context->sdt.section_number_array_num; i++){
        if(context->sdt.section_number_array[i] == context->section_header.section_number){ // repeat
            return 0;
        }
    }
    context->sdt.section_number_array[context->sdt.section_number_array_num++] = context->section_header.section_number;
    // parse
    int server_len = context->section_header.section_length - 5/*transport_stream_id-->last_section_number*/ - 4/*CRC_32*/;
    uint8_t *server_payload_start = context->section_buffer + SECTION_HEADER_SIZE;
    uint8_t *server_payload_end = context->section_buffer + SECTION_HEADER_SIZE + server_len;
    context->sdt.original_network_id = (server_payload_start[0] << 8) | server_payload_start[1];
    context->sdt.reserved_future_use = server_payload_start[2];
    server_payload_start += 3;
    while((server_payload_end - server_payload_start) >= 5){
        mpeg2_sdt_info sdt_info;
        sdt_info.service_id = (server_payload_start[0] << 8) | server_payload_start[1];
        sdt_info.reserved_future_use = (server_payload_start[2] & 0xfc) >> 2;
        sdt_info.EIT_schedule_flag = (server_payload_start[2] & 0x02) >> 1;
        sdt_info.EIT_present_following_flag = server_payload_start[2] & 0x01;
        sdt_info.running_status = (server_payload_start[3] & 0xe0) >> 5;
        sdt_info.free_CA_mode = (server_payload_start[3] & 0x10) >> 4;
        sdt_info.descriptor_loop_length = ((server_payload_start[3] & 0x0f) << 8) | server_payload_start[4];
        memcpy(sdt_info.descriptor, server_payload_start + 4, sdt_info.descriptor_loop_length); // TODO:parse descriptor
        context->sdt.sdt_info_array[context->sdt.sdt_info_array_num++] = sdt_info;
        server_payload_start += 5 + sdt_info.descriptor_loop_length;
    }
    if(server_payload_start != server_payload_end){
        return -1;
    }
    uint32_t CRC_32 = (server_payload_start[0] << 24) | (server_payload_start[1] << 16) | (server_payload_start[2] << 8) | server_payload_start[3];
    // All sections have been received
    if(context->sdt.section_number_array_num == (context->section_header.last_section_number + 1)){
        context->sdt.sdt_ready = 1;
    }
    return 0;
}
int mpeg2_sdt_pack(uint8_t *buffer, int len){
    if(buffer == NULL){
        return -1;
    }
    int s1, n1, v1;
    n1 = strlen(SERVICE_ENCODER);
    v1 = strlen(SERVICE_NAME);
    s1 = 3 /*tag*/ + 1 + n1 + 1 + v1;
    int body_len = 5/*service_id-->descriptor_loop_length*/ + s1/*descriptor*/;
    // section
    mpeg2_section_header section_header;
    memset(&section_header, 0, sizeof(mpeg2_section_header));
    section_header.table_id = TID_SDT;
    section_header.section_length = 5/*section:transport_stream_id-->last_section_number*/ + 3/*original_network_id and reserved_future_use*/  + body_len + 4/*CRC_32*/;
    section_header.transport_stream_id = TRANSPORT_STREAM_ID;
    section_header.section_number = 0;
    section_header.last_section_number = 0;
    int ret = mpeg2_section_header_pack(buffer, len, section_header);
    if(ret < 0){
        return -1;
    }
    // original_network_id
    buffer[ret] = (TRANSPORT_STREAM_ID & 0xff00) >> 8;
    buffer[ret + 1] = TRANSPORT_STREAM_ID & 0xff;
    ret += 2;
    // reserved_future_use
    buffer[ret] = 0xff;
    ret++;
    // service_id
    buffer[ret] = (TRANSPORT_STREAM_ID & 0xff00) >> 8;
    buffer[ret + 1] = TRANSPORT_STREAM_ID & 0xff;
    ret += 2;
    // reserved_future_use\EIT_schedule_flag\EIT_present_following_flag
    buffer[ret] = 0xfc | 0x00; // no EIT
    ret++;
    // running_status\free_CA_mode\descriptor_loop_length
    buffer[ret] = 0x80 | ((s1 & 0x0f00) >> 8); 
    buffer[ret + 1] = s1 & 0xff;
    ret += 2;
    // descriptor
    buffer[ret] = 0x48;
    ret++;
    buffer[ret] = (uint8_t)(3 + n1 + v1); // tag len
    ret++;
    buffer[ret] = 1; // service type
    ret++;
    buffer[ret] = (uint8_t)v1; // service_provider_name_length
    ret++;
    memcpy(buffer + ret, SERVICE_NAME, v1);
    ret += v1;
    buffer[ret] = (uint8_t)n1; // service_name_length
    ret++;
    memcpy(buffer + ret, SERVICE_ENCODER, n1);
    ret += n1;
    //CRC_32
    uint32_t crc_32 = mpeg2core_crc32(0xffffffff, buffer, ret);
    buffer[ret] = crc_32 & 0xff;
    buffer[ret + 1] = (crc_32 >> 8) & 0xff;
    buffer[ret + 2] = (crc_32 >> 16) & 0xff;
    buffer[ret + 3] = (crc_32 >> 24) & 0xff;
    ret += 4;
    return ret;
}