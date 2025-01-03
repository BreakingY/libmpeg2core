#ifndef MPEG2CORE_INTERNAL
#define MPEG2CORE_INTERNAL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpeg2core_ts.h"
#include "mpeg2core_ps.h"
/**
 * TS
 */
#define INITIAL_VERSION     0xff
#define SECTION_HEADER_SIZE 8
#define TS_FIXED_HEADER_LEN 4
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
int mpeg2_ts_video_parse(mpeg2_ts_context *context, int type, int stream_pid);
int mpeg2_ts_audio_parse(mpeg2_ts_context *context, int type, int stream_pid);

/**
 * muxer
 */
#define TRANSPORT_STREAM_ID 1
#define PID_PMT             0x1000
#define PID_MEDIA           0x0100
// #define PID_VIDEO           0x0100
// #define PID_AUDIO           0x0101
// #define PROGRAM             1
#define PCR_DELAY           0 //(700 * 90) // 700ms
#define SERVICE_NAME        "Breaking/libmpeg2core"

// 0:not 1:is type:STREAM_TYPE_XXX_XXX
int mpeg2_is_video(int stream_type);
int mpeg2_is_audio(int stream_type);

// 0:ok <0:error
int mpeg2_find_pmt(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream);

// 0:ok <0:error
int mpeg2_increase_stream_frame_cnt(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream);

// 0:ok <0:error
int mpeg2_increase_stream_continuity_counter(mpeg2_ts_context *context, int stream_pid, mpeg2_pmt **pmt, mpeg2_pmt_stream **pmt_stream);

// return bytes:ok <0:error
int mpeg2_ts_header_pack(uint8_t *buffer, int len, mpeg2_ts_header ts_header, int psi_si, int stuffing_bytes);

// return bytes:ok <0:error
int mpeg2_section_header_pack(uint8_t *buffer, int len, mpeg2_section_header section_header);

// return bytes:ok <0:error
int mpeg2_sdt_pack(uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_pat_pack(mpeg2_pat pat, uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_pmt_pack(mpeg2_pmt pmt, uint8_t *buffer, int len);

// 0:ok <0:error
int mpeg2_ts_media_pack(mpeg2_ts_context *context, mpeg2_pmt *pmt, mpeg2_pmt_stream *pmt_stream);

// return bytes
int mpeg2_add_h264_aud(uint8_t *buffer, int len);
int mpeg2_h264_aud_size();

// return bytes
int mpeg2_add_h265_aud(uint8_t *buffer, int len);
int mpeg2_h265_aud_size();

/**
 * PS
 */
#define PS_HEADER_FIXED_HEADER      14 // pack_start_code-->pack_stuffing_length
#define PS_SYSTEM_HEADER_FIXED_LEN  12 // system_header_start_code-->reserved_bits
#define PS_PSM_FIXED_LEN            16 // include crc_32
#define PS_HEADER_STARTCODE         0x000001BA
#define PS_HEADER_ENDCODE           0x000001B9
#define PS_SYSTEM_HEADER_STARTCODE  0x000001BB
// PES level
#define PES_STARTCODE               0x000001 // include psm
#define PSM_MAP_STREAM_ID           0xBC
/**
 * demuxer
 */
// return bytes:ok <0:error
int mpeg2_ps_header_parse(uint8_t *buffer, int len, mpeg2_ps_header *ps_header);

// 1: is 0: not
int mpeg2_is_system_header(uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_system_header_parse(uint8_t *buffer, int len, mpeg2_ps_system_header *ps_system_header);

// 1: is psm 0: not is psm
int mpeg2_is_psm(uint8_t *buffer, int len);

// 1: is pes(psm or media) 0: not is pes
int mpeg2_is_pes_or_psm(uint8_t *buffer, int len);

// 1: is mpeg1 pes 0: not is mpeg1 pes
int mpeg2_is_pes_mpeg1(uint8_t *buffer, int len);

// return bytes:ok <0:error
int mpeg2_psm_parse(uint8_t *buffer, int len, mpeg2_psm *psm);

// 0:ok <0:error
int mpeg2_ps_media_parse(mpeg2_ps_context *context, uint8_t *pes_buffer, int pes_buffer_len);

// 0:ok <0:error
int mpeg2_ps_mpeg1_media_parse(mpeg2_ps_context *context, uint8_t *pes_buffer, int pes_buffer_len);

/**
 * muxer
 */
// return bytes:ok <0:error
int mpeg2_system_header_pack(uint8_t *buffer, int len, mpeg2_ps_system_header ps_system_header);

// return bytes:ok <0:error
int mpeg2_psm_pack(uint8_t *buffer, int len, mpeg2_psm psm);

// return bytes:ok <0:error
int mpeg2_ps_header_pack(uint8_t *buffer, int len, mpeg2_ps_header ps_header, int over_flag);
#endif