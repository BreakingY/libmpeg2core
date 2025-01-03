#include "mpeg2core_pes.h"
#include "mpeg2core_type.h"
int mpeg2_pes_packet_parse(mpeg2_pes_header *pes_header, uint8_t *buffer, int len, int *media_pos){
    if(pes_header == NULL ){
        return -1;
    }
    if((buffer == NULL) || (len < PES_HEADER_MIN_SIZE)){
        return 0;
    }
    int idx = 0;
    pes_header->packet_start_code_prefix = (buffer[idx] << 16) | (buffer[idx + 1] << 8) | buffer[idx + 2];
    if(pes_header->packet_start_code_prefix != 0x000001){
        return -1;
    }
    idx += 3;
    pes_header->stream_id = buffer[idx]; // PES_VIDEO\PES_VIDEO_PRIVATE or PES_AUDIO\PES_AUDIO_PRIVATE
    idx++;
    pes_header->PES_packet_length = (buffer[idx] << 8) |  buffer[idx + 1];
    idx += 2;
    pes_header->one_zero = (buffer[idx] & 0xc0) >> 6;
    pes_header->PES_scrambling_control = (buffer[idx] & 0x30) >> 4;
    pes_header->PES_priority = (buffer[idx] & 0x08) >> 3;
    pes_header->data_alignment_indicator = (buffer[idx] & 0x04) >> 2;
    pes_header->copyright = (buffer[idx] & 0x02) >> 1;
    pes_header->original_or_copy = buffer[idx] & 0x01;
    idx++;
    // 7 flags
    pes_header->PTS_DTS_flags = (buffer[idx] & 0xc0) >> 6;
    pes_header->ESCR_flag = (buffer[idx] & 0x20) >> 5;
    pes_header->S_rate_flag = (buffer[idx] & 0x10) >> 4;
    pes_header->DSM_trick_mode_flag = (buffer[idx] & 0x08) >> 3;
    pes_header->Additional_copy_info_flag = (buffer[idx] & 0x04) >> 2;
    pes_header->PES_CRC_flag = (buffer[idx] & 0x02) >> 1;
    pes_header->PES_extension_flag = buffer[idx] & 0x01;
    idx++;
    pes_header->PES_header_data_length = buffer[idx];
    idx++;
    *media_pos = idx + pes_header->PES_header_data_length;
    uint8_t pts1, dts1;
    uint16_t pts2, dts2;
    uint16_t pts3, dts3;
    switch (pes_header->PTS_DTS_flags){
        case 0:
            break;
        case 1:
            return -1;
            break;
        case 2: // pts
            pts1 = buffer[idx];
            pts2 = (buffer[idx + 1] << 8) | buffer[idx + 2];
            pts3 = (buffer[idx + 3] << 8) | buffer[idx + 4];
            pes_header->pts = ((pts1 & 0x0e) << 29) | ((pts2 & 0xfffe) << 14) | ((pts3 & 0xfffe) >> 1);
            idx += 5;
            break;
        case 3: // pts + dts
            pts1 = buffer[idx];
            pts2 = (buffer[idx + 1] << 8) | buffer[idx + 2];
            pts3 = (buffer[idx + 3] << 8) | buffer[idx + 4];
            pes_header->pts = ((pts1 & 0x0e) << 29) | ((pts2 & 0xfffe) << 14) | ((pts3 & 0xfffe) >> 1);
            idx += 5;

            dts1 = buffer[idx];
            dts2 = (buffer[idx + 1] << 8) | buffer[idx + 2];
            dts3 = (buffer[idx + 3] << 8) | buffer[idx + 4];
            pes_header->dts = ((dts1 & 0x0e) << 29) | ((dts2 & 0xfffe) << 14) | ((dts3 & 0xfffe) >> 1);
            idx += 5;
            break;
        default:
            break;
    }
    
    return 0;
}
static int mpeg2_pes_header_pack(mpeg2_pes_header pes_header, uint8_t *buffer, int buffer_len, int first_flag){
    if((buffer == NULL) || (buffer_len < PES_HEADER_MIN_SIZE)){
        return -1;
    }
    uint8_t flags = 0x00;
    memset(buffer, 0, buffer_len);
    pes_header.PES_header_data_length = 0;
    int idx = 0;
    // packet_start_code_prefix
    buffer[idx] = 0;
    buffer[idx + 1] = 0;
    buffer[idx + 2] = 1;
    idx += 3;
    // stream_id
    buffer[idx] = pes_header.stream_id;
    idx++;
    // PES_packet_length
    buffer[idx] = 0;
    buffer[idx + 1] = 0;
    idx += 2;

    // 10\PES_scrambling_control:0\PES_priority:0\data_alignment_indicator:first_flag\copyright:0\original_or_copy:0
    if(first_flag == 1){
        buffer[idx] = 0x84;
    }
    else{
        buffer[idx] = 0x80;
    }
    
    idx++;
    // 7 flags
    buffer[idx] = 0;
    switch (pes_header.PTS_DTS_flags){
        case 0:
            break;
        case 1:
            return -1;
            break;
        case 2: // pts
            flags = buffer[idx] = 0x80;
            break;
        case 3: // pts + dts
            flags = buffer[idx] = 0xc0;
            break;
        default:
            break;
    }
    idx++;
    // PES_header_data_length
    int PES_header_data_length_idx = idx;
    buffer[idx] = 0;
    idx++;
    // pts dts 
    uint8_t *p = &buffer[idx];
    if(flags & 0x80){
		*p++ = ((flags >> 2) & 0x30)/* 0011/0010 */ | (((pes_header.pts >> 30) & 0x07) << 1) /* PTS 30-32 */ | 0x01 /* marker_bit */;
		*p++ = (pes_header.pts >> 22) & 0xFF; /* PTS 22-29 */
		*p++ = ((pes_header.pts >> 14) & 0xFE) /* PTS 15-21 */ | 0x01 /* marker_bit */;
		*p++ = (pes_header.pts >> 7) & 0xFF; /* PTS 7-14 */
		*p++ = ((pes_header.pts << 1) & 0xFE) /* PTS 0-6 */ | 0x01 /* marker_bit */;
        pes_header.PES_header_data_length += 5;
        idx += 5;
	}
	if(flags & 0x40){
		*p++ = 0x10 /* 0001 */ | (((pes_header.dts >> 30) & 0x07) << 1) /* DTS 30-32 */ | 0x01 /* marker_bit */;
		*p++ = (pes_header.dts >> 22) & 0xFF; /* DTS 22-29 */
		*p++ = ((pes_header.dts >> 14) & 0xFE) /* DTS 15-21 */ | 0x01 /* marker_bit */;
		*p++ = (pes_header.dts >> 7) & 0xFF; /* DTS 7-14 */
		*p++ = ((pes_header.dts << 1) & 0xFE) /* DTS 0-6 */ | 0x01 /* marker_bit */;
        pes_header.PES_header_data_length += 5;
        idx += 5;
	}
    buffer[PES_header_data_length_idx] = pes_header.PES_header_data_length;
    return idx;
}
static int mpeg2_set_pes_packet_len(uint8_t *buffer, int buffer_len, int packet_length){
    if((buffer == NULL) || (buffer_len < 6)){
        return -1;
    }
    buffer[4] = (packet_length >> 8) & 0xff;
    buffer[5] = packet_length & 0xff;
    return 0;
}
int mpeg2_pes_packet_pack(mpeg2_pes_header pes_header, uint8_t *buffer, int buffer_len, uint8_t *frame, int frame_len){
    if((buffer == NULL) || (frame == NULL)){
        return -1;
    }
    int ret = 0;
    int left_bytes = 0;
    int payload_bytes = 0;
    uint8_t *ptr = frame;
    int ptr_len = frame_len;
    int pes_used_bytes = 0;
    int first_flag = 1;
    uint8_t *pes_buffer;
    int pes_buffer_len;
    int packet_len = 0;
    if(pes_header.write_packet_length_flag == 0){
        ret = mpeg2_pes_header_pack(pes_header, buffer, buffer_len, first_flag);
        if(ret < 0){
            return -1;
        }
        if((ret + frame_len) > buffer_len){
            return 0;
        }
        memcpy(buffer + ret, frame, frame_len);
        return ret + frame_len;
    }
    int i = 0;
    while(ptr_len > 0){
        if(ret != 0){
            first_flag = 0;
            pes_header.PTS_DTS_flags = 0; // not pts dta
        }
        pes_buffer = buffer + pes_used_bytes;
        pes_buffer_len = buffer_len - pes_used_bytes;
        ret = mpeg2_pes_header_pack(pes_header, pes_buffer, pes_buffer_len, first_flag);
        if(ret < 0){
            return -1;
        }
        pes_used_bytes += ret;
        payload_bytes = PES_PACKET_LENGTH_MAX - ret + 6/*packet_start_code_prefix-->PES_packet_length*/;
        if(payload_bytes >= ptr_len){
            packet_len = ret - 6 + ptr_len;
            if(mpeg2_set_pes_packet_len(pes_buffer, pes_buffer_len, packet_len) < 0){
                return -1;
            }
            memcpy(pes_buffer + ret, ptr, ptr_len);
            pes_used_bytes += ptr_len;
            ptr += ptr_len;
            ptr_len = 0;
        }
        else{
            packet_len = ret - 6 + payload_bytes;
            if(mpeg2_set_pes_packet_len(pes_buffer, pes_buffer_len, payload_bytes) < 0){
                return -1;
            }
            memcpy(pes_buffer + ret, ptr, payload_bytes);
            pes_used_bytes += payload_bytes;
            ptr += payload_bytes;
            ptr_len -= payload_bytes;
        }
    }
    return pes_used_bytes;
}
/**
 * PS only
 */
// ISO/IEC 11172-1 
// 2.4.3.3 Packet Layer (p20)
/*
packet() {
	packet_start_code_prefix							24 bslbf
	stream_id											8 uimsbf
	packet_length										16 uimsbf
	if (packet_start_code != private_stream_2) { // private_stream_1	= 0xBD,  private_stream_2	= 0xBF
		while (nextbits() == '1')
			stuffing_byte								8 bslbf

		if (nextbits () == '01') {
			'01'										2 bslbf
			STD_buffer_scale							1 bslbf
			STD_buffer_size								13 uimsbf
		}
		if (nextbits() == '0010') {
			'0010'										4 bslbf
			presentation_time_stamp[32..30]				3 bslbf
			marker_bit									1 bslbf
			presentation_time_stamp[29..15]				15 bslbf
			marker_bit									1 bslbf
			presentation_time_stamp[14..0]				15 bslbf
			marker_bit									1 bslbf
		}
		else if (nextbits() == '0011') {
			'0011'										4 bslbf
			presentation_time_stamp[32..30]				3 bslbf
			marker_bit									1 bslbf
			presentation_time_stamp[29..15]				15 bslbf
			marker_bit									1 bslbf
			presentation_time_stamp[14..0]				15 bslbf
			marker_bit									1 bslbf
			'0001'										4 bslbf
			decoding_time_stamp[32..30]					3 bslbf
			marker_bit									1 bslbf
			decoding_time_stamp[29..15]					15 bslbf
			marker_bit									1 bslbf
			decoding_time_stamp[14..0]					15 bslbf
			marker_bit									1 bslbf
		}
		else
			'0000 1111'									8 bslbf
	}

	for (i = 0; i < N; i++) {
		packet_data_byte								8 bslbf
	}
}
*/
int mpeg2_pes_mpeg1_packet_parse(mpeg2_pes_header *pes_header, uint8_t *buffer, int len, int *media_pos){
    if(pes_header == NULL){
        return -1;
    }
    if(buffer == NULL){
        return 0;
    }
    int idx = 0;
    pes_header->packet_start_code_prefix = (buffer[idx] << 16) | (buffer[idx + 1] << 8) | buffer[idx + 2];
    if(pes_header->packet_start_code_prefix != 0x000001){
        return -1;
    }
    idx += 3;
    pes_header->stream_id = buffer[idx];
    if(pes_header->stream_id == PRIVATE_STREAM_2){ // skip
        *media_pos = 0;
        return 0;
    }
    idx++;
    pes_header->PES_packet_length = (buffer[idx] << 8) |  buffer[idx + 1];
    idx += 2;
    // skip stuffing_byte
    uint8_t v8;
    while(1){
        v8 = buffer[idx];
        if(v8 != 0xff){
            break;
        }
        idx++;
    }
    if(0x40 == (0xC0 & v8)){ // nextbits () == '01'
        //skip STD_buffer_scale STD_buffer_size
        idx += 2;
    }
    v8 = buffer[idx];
    if(0x20 == (0xF0 & v8)){ // nextbits() == '0010'
        /*
        '0010'										4 bslbf
        presentation_time_stamp[32..30]				3 bslbf
        marker_bit									1 bslbf
        presentation_time_stamp[29..15]				15 bslbf
        marker_bit									1 bslbf
        presentation_time_stamp[14..0]				15 bslbf
        marker_bit									1 bslbf
        */
        uint8_t pts1 = buffer[idx];
        uint16_t pts2 = (buffer[idx + 1] << 8) | buffer[idx + 2];
        uint16_t pts3 = (buffer[idx + 3] << 8) | buffer[idx + 4];
        pes_header->dts = pes_header->pts = ((pts1 & 0x0e) << 29) | ((pts2 & 0xfffe) << 14) | ((pts3 & 0xfffe) >> 1);

        idx += 5;
    }
    else if(0x30 == (0xF0 & v8)){ // nextbits() == '0011'
        /*
        '0011'										4 bslbf
        presentation_time_stamp[32..30]				3 bslbf
        marker_bit									1 bslbf
        presentation_time_stamp[29..15]				15 bslbf
        marker_bit									1 bslbf
        presentation_time_stamp[14..0]				15 bslbf
        marker_bit									1 bslbf
        '0001'										4 bslbf
        decoding_time_stamp[32..30]					3 bslbf
        marker_bit									1 bslbf
        decoding_time_stamp[29..15]					15 bslbf
        marker_bit									1 bslbf
        decoding_time_stamp[14..0]					15 bslbf
        marker_bit									1 bslbf
        */
        uint8_t pts1 = buffer[idx];
        uint16_t pts2 = (buffer[idx + 1] << 8) | buffer[idx + 2];
        uint16_t pts3 = (buffer[idx + 3] << 8) | buffer[idx + 4];
        pes_header->pts = ((pts1 & 0x0e) << 29) | ((pts2 & 0xfffe) << 14) | ((pts3 & 0xfffe) >> 1);
        int idx_tmp = idx + 5;
        uint8_t dts1 = buffer[idx_tmp];
        uint16_t dts2 = (buffer[idx_tmp + 1] << 8) | buffer[idx_tmp + 2];
        uint16_t dts3 = (buffer[idx_tmp + 3] << 8) | buffer[idx_tmp + 4];
        pes_header->dts = ((dts1 & 0x0e) << 29) | ((dts2 & 0xfffe) << 14) | ((dts3 & 0xfffe) >> 1);
        idx += 10;
    }
    else{
        // v8 == '0000 1111'
        idx++;
    }
    *media_pos = idx;
    return 0;
}