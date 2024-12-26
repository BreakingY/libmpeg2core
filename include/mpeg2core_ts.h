#ifndef MPEG2CORE_TS
#define MPEG2CORE_TS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpeg2core_type.h"

#define SECTION_MAX_BYTES   4096
#define BUFFER_MAX_BYTES    4096
#define PES_MAX_BYTES       2 * 1024 * 1024
#define SECTION_COUNT_MAX   255 
#define PAT_PROGARM_MAX     512
#define PMT_STREAM_MAX      256
// section header
typedef struct mpeg2_section_header_st{
    uint8_t table_id;
    uint8_t section_syntax_indicator;
    uint8_t zero;
    uint8_t reserved1;
    uint16_t section_length;
    uint16_t transport_stream_id; // PAT-->transport_stream_id; PMT-->program_number
    uint8_t reserved2;
    uint8_t version_number;
    uint8_t current_next_indicator;
    uint8_t section_number;
    uint8_t last_section_number;
}mpeg2_section_header;

// SDT
typedef struct mpeg2_sdt_info_st{
    uint16_t service_id;
    uint8_t reserved_future_use;
    uint8_t EIT_schedule_flag;
    uint8_t EIT_present_following_flag;
    uint8_t running_status;
    uint8_t free_CA_mode;
    uint16_t descriptor_loop_length;
    uint8_t descriptor[BUFFER_MAX_BYTES];
}mpeg2_sdt_info;
typedef struct mpeg2_sdt_st{
    uint8_t section_number_array[SECTION_COUNT_MAX];
    int section_number_array_num;
    uint8_t version_number; // version(section), if change, the pat needs to be rereceived

    uint16_t original_network_id;
    uint8_t reserved_future_use;

    mpeg2_sdt_info sdt_info_array[PAT_PROGARM_MAX];
    int sdt_info_array_num;
    int sdt_ready;
}mpeg2_sdt;

// PAT
typedef struct mpeg2_program_st{
    uint16_t program_number;
    uint8_t reserved;
    uint16_t program_map_pid;
}mpeg2_program;
typedef struct mpeg2_pat_st{
    uint8_t section_number_array[SECTION_COUNT_MAX];
    int section_number_array_num;
    uint8_t version_number; // version(section), if change, the pat needs to be rereceived

    mpeg2_program program_array[PAT_PROGARM_MAX];
    int program_array_num;
    uint16_t network_pid;
    int pat_ready;
}mpeg2_pat;

// PMT
typedef struct mpeg2_pmt_stream_st{
    uint8_t stream_type;
    uint8_t reserved1;
    uint16_t elementary_PID;
    uint8_t reserved2;
    uint16_t ES_info_length;
    uint8_t descriptor[BUFFER_MAX_BYTES];
}mpeg2_pmt_stream;
typedef struct mpeg2_pmt_st{
    uint8_t section_number_array[SECTION_COUNT_MAX];
    int section_number_array_num;
    uint8_t version_number; // version(section), if change, the pat needs to be rereceived

    uint8_t reserved1;
    uint16_t PCR_PID;
    uint8_t reserved2;
    uint16_t program_info_length;
    uint8_t descriptor[BUFFER_MAX_BYTES];
    mpeg2_pmt_stream pmt_stream_array[PMT_STREAM_MAX];
    int pmt_stream_array_num;
    int pmt_ready;
    uint16_t pid;
}mpeg2_pmt;

// TS adaptation header
typedef struct mpeg2_ts_adaptation_header_st{
    uint8_t adaptation_field_length;
    uint8_t discontinuity_indicator;
    uint8_t random_access_indicator;
    uint8_t elementary_stream_priority_indicator;
    // 5 flags
    uint8_t PCR_flag;
    uint8_t OPCR_flag;
    uint8_t splicing_point_flag;
    uint8_t transport_private_data_flag;
    uint8_t adaptation_field_extension_flag;
}mpeg2_ts_adaptation_header;

// TS header
typedef struct mpeg2_ts_header_st{
    // fixed
    uint8_t sync_byte;
    uint8_t transport_error_indicator;
    uint8_t payload_unit_start_indicator;
    uint8_t transport_priority;
    uint16_t pid;
    uint8_t transport_scrambling_control;
    uint8_t adaptation_field_control;
    uint8_t continuity_counter;

    // adaptation field
    mpeg2_ts_adaptation_header adaptation;

    // optional field
    uint64_t PCR;
    uint64_t OPCR;

    /**
     * PSI/SI with pointer_field 
     * PES
     */
    uint8_t *payload;
    int payload_len;
}mpeg2_ts_header;
// pts dts video timebase: 90000, audio timebase: sampling rate
typedef void (*VideoReadCallback)(int/*STREAM_TYPE_VIDEO_xxx, mpeg2core_type.h*/, int64_t/*pts*/, int64_t/*dts*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 
typedef void (*AudioReadCallback)(int/*STREAM_TYPE_AUDIO_xxx, mpeg2core_type.h*/, int64_t/*pts*/, int64_t/*dts*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 
typedef void (*MediaWriteCallback)(int/*STREAM_TYPE_xxx_xxx, mpeg2core_type.h*/, uint8_t*/*data*/, int/*data_len*/, void*/*user arg*/); 

typedef struct mpeg2_ts_context_st{
    mpeg2_ts_header ts_header;
    mpeg2_pat pat;
    mpeg2_sdt sdt;
    mpeg2_pmt pmt_array[PAT_PROGARM_MAX];
    int pmt_array_num;
    int current_pmt_idx;
    mpeg2_pmt pmt; // pmt_array[0] libmpeg2core only receive one stream, default pmt_array[0]

    // rebuild section
    mpeg2_section_header section_header; // the section header of the current TS packet
    uint8_t section_buffer[SECTION_MAX_BYTES];
    int section_buffer_pos;

    // build PES
    uint8_t pes_buffer_v[PES_MAX_BYTES];
    int pes_buffer_pos_v;
  
    uint8_t pes_buffer_a[PES_MAX_BYTES];
    int pes_buffer_pos_a;

    VideoReadCallback video_read_callback;
    AudioReadCallback audio_read_callback;
    MediaWriteCallback media_write_callback;
    void *arg;
    int write_init;

    // only TS muxer
    uint8_t ts_buffer[TS_PACKET_LENGTH_188];
    int video_type;
    int audio_type;
    uint8_t frame_buffer[PES_MAX_BYTES]; // video frame cache
    int frame_buffer_len;
    int audio_frame_cnt;
    int video_frame_cnt;
    int psi_flag; // generate PAT PMT
    int pat_continuity_counter;
    int pmt_continuity_counter;
    int sdt_continuity_counter;
    int video_continuity_counter;
    int audio_continuity_counter;
    int key_flag; // generate random_access_indicator
    int64_t pts;
    int64_t dts;
    int64_t last_pts;
    int64_t last_dts;
    int pcr_period;
    int pat_period;
    int pcr_start_flag;
    int pcr_pid;

}mpeg2_ts_context;

mpeg2_ts_context *create_ts_context();
void destroy_ts_context(mpeg2_ts_context *context);

void dump_section_header(mpeg2_section_header section_header);
void dump_ts_header(mpeg2_ts_header ts_header);
void dump_program(mpeg2_pat pat);
void dump_pmt_array(mpeg2_pmt *pmt_array, int pmt_array_num);
/**
 * TS demuxer API
 */
/**
 * set read callback
 * @param[in] context               created by create_ts_context()
 * @param[in] video_read_callback   video callback
 * @param[in] audio_read_callback   audio callback
 * @param[in] arg                   user arg
 * @param return                    void
 */
void mpeg2_set_read_callback(mpeg2_ts_context *context, VideoReadCallback video_read_callback, AudioReadCallback audio_read_callback, void *arg);

/**
 * parse TS packet
 * @param[in] context   created by create_ts_context()
 * @param[in] buffer    a complete TS packet
 * @param[in] len       size of buffer(108 or 204)
 * @param return        0:ok <0:error
 */
int mpeg2_ts_packet_demuxer(mpeg2_ts_context *context, uint8_t *buffer, int len);

/**
 * TS muxer API
 */
/**
 * set write callback
 * @param[in] context               created by create_ts_context()
 * @param[in] media_write_callback  write callback
 * @param[in] arg                   user arg
 * @param return                    void
 */
void mpeg2_set_write_callback(mpeg2_ts_context *context, MediaWriteCallback media_write_callback, void *arg);

/**
 * set media type
 * @param[in] context       created by create_ts_context()
 * @param[in] video_type    STREAM_TYPE_VIDEO_XXX(mpeg2core_type.h), If have not, pass in STREAM_TYPE_VIDEO_NONE
 * @param[in] audio_type    STREAM_TYPE_AUDIO_XXX(mpeg2core_type.h), If have not, pass in STREAM_TYPE_AUDIO_NONE
 * @param return            void
 */
void mpeg2_set_media_type(mpeg2_ts_context *context, int video_type, int audio_type);

/**
 * muxer TS packet
 * @param[in] context   created by create_ts_context()
 * @param[in] buffer    video(only one h264 h265 NALU) or audio data
 * @param[in] len       size of buffer
 * @param[in] type      media type, STREAM_TYPE_xxx_xxx, mpeg2core_type.h
 * @param[in] pts       pts millisecond in video timebase: 90000 or millisecond in audio timebase: sampling rate
 * @param[in] dts       dts millisecond in video timebase: 90000 or millisecond in audio timebase: sampling rate
 * @param return        0:ok <0:error
 */
int mpeg2_ts_packet_muxer(mpeg2_ts_context *context, uint8_t *buffer, int len, int type, int64_t pts, int64_t dts);
#endif