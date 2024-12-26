#ifndef MPEG2CORE_INTERNAL
#define MPEG2CORE_INTERNAL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpeg2core_ts.h"
/**
 * TS
 */
#define INITIAL_VERSION     0xff
#define SECTION_HEADER_SIZE 8
struct PCR
{
    uint8_t pcr_base32_25:8;
    uint8_t pcr_base24_17:8;
    uint8_t pcr_base16_9:8;
    uint8_t pcr_base8_1:8;

    uint8_t pcr_extension8:1;
    uint8_t reserved:6;
    uint8_t pcr_base0:1;

    uint8_t pcr_extension7_0:8;
};
struct OPCR
{
    uint8_t opcr_base32_25:8;
    uint8_t opcr_base24_17:8;
    uint8_t opcr_base16_9:8;
    uint8_t opcr_base8_1:8;

    uint8_t opcr_extension8:1;
    uint8_t oreserved:6;
    uint8_t opcr_base0:1;

    uint8_t opcr_extension7_0:8;
};
/**
 * demuxer
 */
// 0:ok <0:error
int mpeg2_ts_header_parse(uint8_t *buffer, int len, mpeg2_ts_header *ts_header);

// 0:ok <0:error
int mpeg2_section_header_parse(uint8_t *buffer, int len, mpeg2_section_header *section_header);

// 0:ok <0:error
int mpeg2_get_one_section(mpeg2_ts_context *context);

// 0:ok <0:error
int mpeg2_get_one_complete_section(mpeg2_ts_context *context);

// 0:ok <0:error
int mpeg2_sdt_parse(mpeg2_ts_context *context);

// 0:ok <0:error
int mpeg2_pat_parse(mpeg2_ts_context *context);

// 0:ok <0:error
int mpeg2_pmt_parse(mpeg2_ts_context *context);

// media 0:ok <0:error
int mpeg2_ts_video_parse(mpeg2_ts_context *context, int type);
int mpeg2_ts_audio_parse(mpeg2_ts_context *context, int type);

/**
 * muxer
 */
#define TRANSPORT_STREAM_ID 1
#define PID_PMT             0x1000
#define PID_VIDEO           0x0100
#define PID_AUDIO           0x0101
#define PROGRAM             1
#define PCR_DELAY           0 //(700 * 90) // 700ms
#define SERVICE_NAME        "Breaking/libmpeg2core"

// return bytes:ok <0:error
int mpeg2_ts_header_pack(uint8_t *buffer, int len, mpeg2_ts_header ts_header, int psi_si, int stuffing_bytes);

// return bytes:ok <0:error
int mpeg2_section_header_pack(uint8_t *buffer, int len, mpeg2_section_header section_header);

// return bytes:ok <0:error
int mpeg2_sdt_pack(uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_pat_pack(uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_pmt_pack(mpeg2_ts_context *context, uint8_t *buffer, int len);

// 0:ok <0:error
int mpeg2_ts_media_pack(mpeg2_ts_context *context, int type, int media_pid);

// return bytes
int mpeg2_add_h264_aud(uint8_t *buffer, int len);
int mpeg2_h264_aud_size();

// return bytes
int mpeg2_add_h265_aud(uint8_t *buffer, int len);
int mpeg2_h265_aud_size();

/**
 * PS
 */
#endif