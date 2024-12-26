#include "mpeg2core_pes.h"
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
    pes_header->stream_id = buffer[idx]; // PES_VIDEO or PES_AUDIO
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
int mpeg2_pes_packet_pack(mpeg2_pes_header pes_header, uint8_t *buffer, int buffer_len, uint8_t *frame, int frame_len){
    if((buffer == NULL) || (frame == NULL) || (buffer_len < PES_HEADER_MIN_SIZE)){
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

    // 10\PES_scrambling_control:0\PES_priority:0\data_alignment_indicator:1\copyright:0\original_or_copy:0
    buffer[idx] = 0x84;
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
    if((buffer_len - idx) < frame_len){
        return 0;
    }
    memcpy(buffer + idx, frame, frame_len);
    idx += frame_len;
    return idx;
}