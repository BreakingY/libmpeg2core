#ifndef MPEG2CORE_PS
#define MPEG2CORE_PS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpeg2core_type.h"
#define PS_STREAM_MAX   16
// PS system header
typedef struct mpeg2_ps_stream_header_st{
    uint8_t stream_id;
    uint8_t buffer_bound_scale;
    uint16_t buffer_size_bound;
}mpeg2_ps_stream_header;
typedef struct mpeg2_ps_system_header_st{
    uint32_t system_header_start_code;
    uint16_t header_length;
    uint8_t marker_bit1;
    uint32_t rate_bound;
    uint8_t marker_bit12;
    uint8_t audio_bound;
    uint8_t fixed_flag;
    uint8_t CSPS_flag;
    uint8_t system_audio_lock_flag;
    uint8_t system_video_lock_flag;
    uint8_t marker_bit3;
    uint8_t video_bound;
    uint8_t packet_rate_restriction_flag;
    uint8_t reserved_bits;

    mpeg2_ps_stream_header ps_stream_header_array[PS_STREAM_MAX];
    int ps_stream_header_array_num;

    int ready_flag;
}mpeg2_ps_system_header;

// program_stream_map(PSM)
typedef struct mpeg2_psm_stream_st{
    uint8_t stream_type;
    uint8_t elementary_stream_id;
    uint16_t elementary_stream_info_length;
    uint8_t descriptor[DESC_BUFFER_MAX_BYTES];
}mpeg2_psm_stream;
typedef struct mpeg2_psm_st{
    uint32_t packet_start_code_prefix;
    uint8_t map_stream_id;
    uint16_t program_stream_map_length;
    uint8_t current_next_indicator;
    uint8_t reserved1;
    uint8_t program_stream_map_version;
    uint8_t reserved2;
    uint8_t marker_bit;
    uint16_t program_stream_info_length;
    uint8_t descriptor[DESC_BUFFER_MAX_BYTES];
    uint16_t elementary_stream_map_length;

    mpeg2_psm_stream psm_stream_array[PS_STREAM_MAX];
    int psm_stream_array_num;

    uint32_t crc_32;

    int ready_flag;
}mpeg2_psm;

// PS header
typedef struct mpeg2_ps_header_st{
    uint32_t pack_start_code;
    // '01'
    int64_t system_clock_reference_base;
    uint16_t system_clock_reference_extension;
    uint32_t program_mux_rate;
    // uint8_t marker_bit1;
    // uint8_t marker_bit2;
    // uint8_t reserved;
    uint8_t pack_stuffing_length;
    int is_mpeg2;
    int ready_flag;
}mpeg2_ps_header;
// pts dts video timebase: 90000, audio timebase: sampling rate
typedef void (*PSVideoReadCallback)(int/*STREAM_TYPE_VIDEO_XXX, mpeg2core_type.h, if 0 unknow type*/, int64_t/*pts*/, int64_t/*dts*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 
typedef void (*PSAudioReadCallback)(int/*STREAM_TYPE_AUDIO_XXX, mpeg2core_type.h, if 0 unknow type*/, int64_t/*pts*/, int64_t/*dts*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 
typedef void (*PSMediaWriteCallback)(int/*STREAM_TYPE_xxx_xxx, mpeg2core_type.h end of file == 0*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 
enum PS_STAT{
    PS_HEADER = 0,
    PS_STUFFING,
    PS_BODY,
};
typedef struct mpeg2_ps_context_st{
    mpeg2_ps_system_header ps_system_header;
    mpeg2_psm psm;
    mpeg2_psm_stream psm_stream_audio; // The currently audio stream, muxer
    mpeg2_psm_stream psm_stream_video; // The currently video stream, muxer
    mpeg2_ps_header ps_header;
    uint8_t *ps_buffer;
    int ps_buffer_pos;
    int ps_buffer_len;
    int demuxer_stat;

    PSVideoReadCallback video_read_callback;
    PSAudioReadCallback audio_read_callback;
    PSMediaWriteCallback media_write_callback;
    void *arg;

    // muxer only
    int file_flag;
    uint8_t frame_buffer[PES_MAX_BYTES]; // video frame cache
    int frame_buffer_len;
    // build PES
    uint8_t pes_buffer_v[PES_MAX_BYTES];
    int pes_buffer_pos_v;
  
    uint8_t pes_buffer_a[PES_MAX_BYTES];
    int pes_buffer_pos_a;
    int audio_frame_cnt;
    int video_frame_cnt;
    int psm_period;
    int psm_flag;
    int key_flag;
    int write_init;
    int64_t pts;
    int64_t dts;

}mpeg2_ps_context;


mpeg2_ps_context *create_ps_context();
void destroy_ps_context(mpeg2_ps_context *context);

void dump_ps_header(mpeg2_ps_header ps_header);
void dump_psm(mpeg2_psm psm);

/**
 * PS demuxer API
 * not thread safe
 */
/**
 * set read callback
 * @param[in] context               created by create_ps_context()
 * @param[in] video_read_callback   video callback
 * @param[in] audio_read_callback   audio callback
 * @param[in] arg                   user arg
 * @param return                    void
 */
void mpeg2_ps_set_read_callback(mpeg2_ps_context *context, PSVideoReadCallback video_read_callback, PSAudioReadCallback audio_read_callback, void *arg);

/**
 * parse PS packet
 * @param[in] context   created by create_ps_context()
 * @param[in] buffer    ps data
 * @param[in] len       size of buffer
 * @param return        0:ok <0:error
 */
int mpeg2_ps_packet_demuxer(mpeg2_ps_context *context, uint8_t *buffer, int len);

/**
 * PS muxer API, MPEG2 PS
 * not thread safe
 */
/**
 * set write callback
 * @param[in] context               created by create_ps_context()
 * @param[in] media_write_callback  write callback
 * @param[in] file_flag             1: it's a file, add 0x000001B9 at the end  0: it's a stream
 * @param[in] arg                   user arg
 * @param return                    void
 */
void mpeg2_ps_set_write_callback(mpeg2_ps_context *context, PSMediaWriteCallback media_write_callback, int file_flag, void *arg);

/**
 * add one stream
 * @param[in] context           created by create_ps_context()
 * @param[in] stream_type       STREAM_TYPE_VIDEO_XXX(mpeg2core_type.h), STREAM_TYPE_AUDIO_XXX(mpeg2core_type.h)
 * @param[in] stream_info       stream descriptor info // TODO
 * @param[in] stream_info_len   stream descriptor info len
 * @param return                0:ok <0:error
 */
int mpeg2_ps_add_stream(mpeg2_ps_context *context, int stream_type, uint8_t *stream_info, int stream_info_len);

/**
 * muxer PS packet
 * @param[in] context       created by create_ps_context()
 * @param[in] buffer        video(only one h264 h265 NALU) or audio data
 * @param[in] len           size of buffer
 * @param[in] type          stream type, STREAM_TYPE_xxx_xxx, mpeg2core_type.h
 * @param[in] pts           pts millisecond in video timebase: 90000 or millisecond in audio timebase: sampling rate
 * @param[in] dts           dts millisecond in video timebase: 90000 or millisecond in audio timebase: sampling rate
 * @param return            0:ok <0:error
 */
int mpeg2_ps_packet_muxer(mpeg2_ps_context *context, uint8_t *buffer, int len, int type, int64_t pts, int64_t dts);

#endif