#ifndef MPEG2CORE_PES
#define MPEG2CORE_PES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define PES_HEADER_MIN_SIZE     9
#define PES_VIDEO               0xe0
#define PES_VIDEO_PRIVATE       0xef
#define PES_AUDIO               0xc0
#define PES_AUDIO_PRIVATE       0xdf
#define PES_PACKET_LENGTH_MAX   65535

typedef struct mpeg2_pes_header_st{
    /* fix */
    uint32_t packet_start_code_prefix;
    uint8_t  stream_id;
    uint16_t PES_packet_length;

    /* optional pes header */
    uint8_t one_zero;
    uint8_t PES_scrambling_control;
    uint8_t PES_priority;
    uint8_t data_alignment_indicator;
    uint8_t copyright;
    uint8_t original_or_copy;
    // 7 flags
    uint8_t PTS_DTS_flags;
    uint8_t ESCR_flag;
    uint8_t S_rate_flag;
    uint8_t DSM_trick_mode_flag;
    uint8_t Additional_copy_info_flag;
    uint8_t PES_CRC_flag;
    uint8_t PES_extension_flag;

    uint8_t PES_header_data_length;

    /* optional fields 1 */
    int64_t pts;
    int64_t dts;

    uint64_t ESCR;
    uint32_t ES_rate;
    uint8_t DSM_trick_mode;
    uint8_t additional_copy_info;
    uint16_t previous_PES_CRC;

    /* PES extension */

    /* optional fields 2 */

    // only ps muxer
    int write_packet_length_flag; // 1:wirte(PS,if don't write PES_packet-length, vlc and libmpeg2core can be parsed, but ffmpeg cannot), 0:ignore(TS)
}mpeg2_pes_header;

/**
 * parse PES packet
 * @param[in] pes_header    mpeg2_pes_header
 * @param[in] buffer        a complete PES packet
 * @param[in] len           size of buffer
 * @param[out] media_pos    The starting position of the media in the buffer
 * @param return            0:ok <0:error
 */
int mpeg2_pes_packet_parse(mpeg2_pes_header *pes_header, uint8_t *buffer, int len, int *media_pos);

/**
 * parse PES packet
 * @param[in] pes_header    mpeg2_pes_header
 * @param[in] buffer        pes buffer
 * @param[in] buffer_len    size of buffer
 * @param[in] frame         audio frame or video frame
 * @param[in] frame_len     size of frame
 * @param return            bytes(size of pes):ok <0:error
 */
int mpeg2_pes_packet_pack(mpeg2_pes_header pes_header, uint8_t *buffer, int buffer_len, uint8_t *frame, int frame_len);

/**
 * PS only
 */
/**
 * parse PES packet
 * @param[in] pes_header    mpeg2_pes_header
 * @param[in] buffer        a complete PES packet
 * @param[in] len           size of buffer
 * @param[out] media_pos    The starting position of the media in the buffer
 * @param return            0:ok <0:error
 */
int mpeg2_pes_mpeg1_packet_parse(mpeg2_pes_header *pes_header, uint8_t *buffer, int len, int *media_pos);
#endif