#ifndef MPEG2CORE_COMMON
#define MPEG2CORE_COMMON
#include "mpeg2core_type.h"

/**
 * detecting TS packet length
 * @param[in] fp        TS file pointer
 * @param return        TS_PACKET_LENGTH_188 or TS_PACKET_LENGTH_204 <0:error
 */
int probe_ts_packet_length(FILE *fp);


/**
 * get NALU start code
 * @param[in] buffer    NALU
 * @param[in] len       len of buffer
 * @param return        start code bytes
 */
int get_start_code(uint8_t *buffer, int len);

/**
 * NALU start code is 4 bytes
 * @param[in] buffer    NALU
 * @param[in] len       len of buffer
 * @param return        0: not 1: is
 */
int start_code3(uint8_t *buffer, int len);

/**
 * NALU start code is 4 bytes
 * @param[in] buffer    NALU
 * @param[in] len       len of buffer
 * @param return        0: not 1: is
 */
int start_code4(uint8_t *buffer, int len);

/**
 * H264 NALU is new access unit
 * @param[in] buffer    NALU
 * @param[in] len       len of buffer
 * @param return        0:not 1:is new access unit 
 */
int mpeg2_h264_new_access_unit(uint8_t *buffer, int len);

/**
 * H265 NALU is new access unit
 * @param[in] buffer    NALU
 * @param[in] len       len of buffer
 * @param return        0:not 1:is new access unit 
 */
int mpeg2_h265_new_access_unit(uint8_t *buffer, int len);
#endif