#include "mpeg2core_common.h"
static int judgment_package(FILE *fp, int ts_length){
    int byte = 0;
    int loop_times = 5;
    for (int i = 0; i < loop_times; i++){
        byte = fgetc(fp);
        if (byte == EOF || byte != 0x47) {
            return -1;
        }
        if (fseek(fp, ts_length - 1, SEEK_CUR) != 0){
            return -1;
        }
    }
    return 0;
}

int probe_ts_packet_length(FILE *fp){
    if (fp == NULL) {
        return -1;
    }

    int64_t file_pos = ftell(fp);
    int byte;
    int length = -1;

    while ((byte = fgetc(fp)) != EOF) {
        if (byte == 0x47) {
            if (fseek(fp, -1, SEEK_CUR) != 0) {
                goto end;
            }
            if (judgment_package(fp, TS_PACKET_LENGTH_188) == 0) {
                length = TS_PACKET_LENGTH_188;
                goto end;
            }
            if (judgment_package(fp, TS_PACKET_LENGTH_204) == 0) {
                length = TS_PACKET_LENGTH_204;
                goto end;
            }
        }
    }

    if (length == -1) {
        if (fseek(fp, 0, SEEK_END) != 0) {
            goto end;
        }
        int64_t file_size = ftell(fp);
        if (file_size % TS_PACKET_LENGTH_188 == 0) {
            length = TS_PACKET_LENGTH_188;
            goto end;
        } else if (file_size % TS_PACKET_LENGTH_204 == 0) {
            length = TS_PACKET_LENGTH_204;
            goto end;
        }
    }
end:
    if (fseek(fp, file_pos, SEEK_SET) != 0) {
        return -1;
    }
    return length;
}
int start_code3(uint8_t *buffer, int len){
    if(len < 3){
        return 0;
    }
    if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1)
        return 1;
    else
        return 0;
}
int start_code4(uint8_t *buffer, int len){
    if(len < 4){
        return 0;
    }
    if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1)
        return 1;
    else
        return 0;
}
int get_start_code(uint8_t *buffer, int len){
    if(start_code3(buffer, len)){
        return 3;
    }
    else if(start_code4(buffer, len)){
        return 4;
    }
    return 0;
}
int mpeg2_h264_new_access_unit(uint8_t *buffer, int len){
    int start_code = get_start_code(buffer, len);
    if(len < (start_code + 2)){
        return 0;
    }
    int nal_type = buffer[start_code] & 0x1f;
    if(H264_NAL_AUD == nal_type || H264_NAL_SPS == nal_type || H264_NAL_PPS == nal_type || H264_NAL_SEI == nal_type || (14 <= nal_type && nal_type <= 18))
        return 1;
    
    if(H264_NAL_NIDR == nal_type || H264_NAL_PARTITION_A == nal_type || H264_NAL_IDR == nal_type){
        return (buffer[start_code + 1] & 0x80) ? 1 : 0; // first_mb_in_slice
    }
    
    return 0;
}
int mpeg2_h265_new_access_unit(uint8_t *buffer, int len){
    int start_code = get_start_code(buffer, len);
    int nal_type = (buffer[start_code] >> 1) & 0x3f;
    if(len < (start_code + 3)){
        return 0;
    }
    int nuh_layer_id = ((buffer[start_code] & 0x01) << 5) | ((buffer[start_code + 1] >> 3) &0x1F);
    
    if(H265_NAL_VPS == nal_type || H265_NAL_SPS == nal_type || H265_NAL_PPS == nal_type ||
       (nuh_layer_id == 0 && (H265_NAL_AUD == nal_type || H265_NAL_SEI_PREFIX == nal_type || (41 <= nal_type && nal_type <= 44) || (48 <= nal_type && nal_type <= 55))))
        return 1;
        
    
    if (nal_type < H265_NAL_VPS){
        return (buffer[start_code + 2] & 0x80) ? 1 : 0;
    }
    
    return 0;
}

static uint8_t h264_nalu_aud[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0xF0};
static uint8_t h265_nalu_aud[7] = {0x00, 0x00, 0x00, 0x01, 0x46, 0x01, 0x50};
int mpeg2_add_h264_aud(uint8_t *buffer, int len){
    if(len < sizeof(h264_nalu_aud)){
        return 0;
    }
    memcpy(buffer, h264_nalu_aud, sizeof(h264_nalu_aud));
    return sizeof(h264_nalu_aud);
}

int mpeg2_h264_aud_size(){
    return sizeof(h264_nalu_aud);
}

int mpeg2_add_h265_aud(uint8_t *buffer, int len){
    if(len < sizeof(h265_nalu_aud)){
        return 0;
    }
    memcpy(buffer, h265_nalu_aud, sizeof(h265_nalu_aud));
    return sizeof(h265_nalu_aud);
}

int mpeg2_h265_aud_size(){
    return sizeof(h265_nalu_aud);
}