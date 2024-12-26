#ifndef MPEG2CORE_TYPE
#define MPEG2CORE_TYPE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define H265_NAL_BLA_W_LP	16
#define H265_NAL_IDR_W_RADL 19
#define H265_NAL_IDR_N_LP   20
#define H265_NAL_CRA        21 // may have leading pictures
#define H265_NAL_RSV_IRAP	23
#define H265_NAL_VPS		32
#define H265_NAL_SPS		33
#define H265_NAL_PPS		34
#define H265_NAL_AUD		35
#define H265_NAL_SEI_PREFIX	39
#define H265_NAL_SEI_SUFFIX	40

#define H264_NAL_NIDR           1
#define H264_NAL_PARTITION_A    2
#define H264_NAL_IDR            5
#define H264_NAL_SEI            6
#define H264_NAL_SPS            7
#define H264_NAL_PPS            8
#define H264_NAL_AUD            9

/**
 * TS related definitions
 */
#define TS_PACKET_LENGTH_188 188
#define TS_PACKET_LENGTH_204 204

// PID
/**
 * PSI:PAT,PMT,CAT,NIT，SI: SDT/BAT,EIT,TDT/TOT,ST,SIT,DIT,RST
 * SDT/BAT,pid:0x11(table id is different);TDT/TOT,pid:0x14(table id is different)；ST(table-id=0x72),may exist in packets 0x10~0x14
 * PSI/SI                   | PID
 * ------------------------ | ----------------
 * PAT                      | 0x0000
 * CAT                      | 0x0001
 * TSDT                     | 0x0002
 * NIT、ST                  | 0x0010
 * SDT、BAT、ST             | 0x0011
 * EIT、ST                  | 0x0012
 * RST、ST                  | 0x0013
 * TDT、TOT、ST             | 0x0014
 * DIT                      | 0x001E
 * SIT                      | 0x001F
 */
#define PID_PAT                     0x0000
#define PID_CAT                     0x0001
#define PID_TSDT                    0x0002
#define PID_NIT                     0x0010
#define PID_SDT                     0x0011
#define PID_EIT                     0x0012
#define PID_RST                     0x0013
#define PID_TDT                     0x0014

// #define PID_SYNC                    0x0015
// #define PID_INBAND                  0x001c
// #define PID_MEASUREMENT             0x001d

#define PID_DIT                     0x001e
#define PID_SIT                     0x001f

// #define PID_NULL                    0x1fff
// #define PID_UNSPEC                  0xffff

// table id
#define TID_PAT                     0x00
#define TID_CAT                     0x01
#define TID_PMT                     0x02
#define TID_NIT                     0x40
#define TID_SDT                     0x42
#define TID_BAT                     0x4A
/**
 * current TS：
 * PF 0x4E
 * Schedule: 0x50-0x5F
 * other TS：
 * PF 0x4F
 * Schedule: 0x60-0x6F
 */
// #define TID_EIT_PF                  0x4e
// #define TID_EIT_SCHEDULE50          0x50
// #define TID_EIT_SCHEDULE51          0x51

// #define TID_TDT                     0x70
// #define TID_RST                     0x71
// #define TID_ST                      0x72
// #define TID_TOT                     0x73
// #define TID_SIT                     0x7E
// #define TID_DIT                     0x7E

// stream_type
#define STREAM_TYPE_AUDIO_NONE      0x00
// #define STREAM_TYPE_VIDEO_MPEG1     0x01
// #define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04 // MP3
// #define STREAM_TYPE_PRIVATE_SECTION 0x05
// #define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_AUDIO_AAC_LATM  0x11
// #define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_NONE      STREAM_TYPE_AUDIO_NONE
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24
#define STREAM_TYPE_VIDEO_H265      STREAM_TYPE_VIDEO_HEVC
// #define STREAM_TYPE_VIDEO_CAVS      0x42
// #define STREAM_TYPE_VIDEO_VC1       0xea
// #define STREAM_TYPE_VIDEO_DIRAC     0xd1

// #define STREAM_TYPE_AUDIO_AC3       0x81
// #define STREAM_TYPE_AUDIO_DTS       0x82
// #define STREAM_TYPE_AUDIO_TRUEHD    0x83
#define	STREAM_TYPE_AUDIO_G711A     0x90
#define STREAM_TYPE_AUDIO_G711U     0x91
// 

/**
 * PS related definitions
 */


#endif