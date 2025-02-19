#include "mpeg2core_common.h"
#include "mpeg2core_ps.h"
#include <time.h>

#define BUFFER 2 * 1024 * 1024
const char *video_path = "../media/test.h264";
const char *audio_path = "../media/test.aac";
static uint8_t *findNextStartCode(uint8_t *buf, int len)
{
    int i;

    if (len < 3)
        return NULL;

    for (i = 0; i < len - 3; ++i) {
        if (get_start_code(buf, len))
            return buf;

        ++buf;
    }

    if (start_code3(buf, len))
        return buf;

    return NULL;
}
static int getFrameFromH264File(FILE *fp, uint8_t *frame, int size)
{
    int r_size, frame_size;
    uint8_t *next_start_code;

    if (fp == NULL)
        return -1;
    r_size = fread(frame, 1, size, fp);
    if(!get_start_code(frame, r_size)){
        return -1;
    }
    next_start_code = findNextStartCode(frame + 3, r_size - 3);
    if (!next_start_code) {
        // fseek(fp, 0, SEEK_SET);
        // frame_size = r_size;
        return -1;
    } else {
        frame_size = (next_start_code - frame);
        fseek(fp, frame_size - r_size, SEEK_CUR);
    }
    return frame_size;
}
// timestamp_ms = millisecond * (sampling_rate/1000)
// timestamp_s = second * sampling_rate
static int time_base_convert(int64_t timestamp_ms, int sampling_rate){
    return timestamp_ms * (sampling_rate / 1000);
} 
static void media_write_callback(int stream_type, uint8_t *data, int data_len, void *arg){
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
            // printf("STREAM_TYPE_AUDIO_AAC\n");
            break;
        case STREAM_TYPE_AUDIO_MPEG1:
            // printf("STREAM_TYPE_AUDIO_MPEG1\n");
            break;
        case STREAM_TYPE_AUDIO_MP3:
            // printf("STREAM_TYPE_AUDIO_MP3\n");
            break;
        case STREAM_TYPE_AUDIO_AAC_LATM:
            // printf("STREAM_TYPE_AUDIO_AAC_LATM\n");
            break;
        case STREAM_TYPE_AUDIO_G711A:
            // printf("STREAM_TYPE_AUDIO_G711A\n");
            break;
        case STREAM_TYPE_AUDIO_G711U:
            // printf("STREAM_TYPE_AUDIO_G711U\n");
            break;
        case STREAM_TYPE_VIDEO_H264:
            // printf("STREAM_TYPE_VIDEO_H264\n");
            break;
        case STREAM_TYPE_VIDEO_HEVC:
            // printf("STREAM_TYPE_VIDEO_HEVC\n");
            break;
        default:
            // printf("end of file 0x000001B9\n");
            break;
    }
    FILE *ps_fd = (FILE *)arg;
    if(ps_fd){
        fwrite(data, 1, data_len, ps_fd);
    }
    return;
}
typedef struct adts_header_st {
    // fixed
    unsigned int syncword;
    unsigned int id;
    unsigned int layer;
    unsigned int protection_absent;
    unsigned int profile;
    unsigned int sampling_frequency_index;
    unsigned int private_bit;
    unsigned int channel_configuration;
    unsigned int original_copy;
    unsigned int home;
    // variable
    unsigned int copyright_identification_bit;
    unsigned int copyright_identification_start;
    unsigned int aac_frame_length;
    unsigned int adts_buffer_fullness;
    unsigned int number_of_raw_data_block_in_frame;
}adts_header;
static const int sampling_frequencies[] = {
    96000, // 0x0
    88200, // 0x1
    64000, // 0x2
    48000, // 0x3
    44100, // 0x4
    32000, // 0x5
    24000, // 0x6
    22050, // 0x7
    16000, // 0x8
    12000, // 0x9
    11025, // 0xa
    8000   // 0xb
           // 0xc d e f
};
static int ParseAdtsHeader(uint8_t *in, int len, adts_header *res){
    memset(res, 0, sizeof(*res));
    if(len < 7){
        return -1;
    }
    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)){ // syncword
        res->id = (in[1] & 0x08) >> 3;
        res->layer = (in[1] & 0x06) >> 1;
        res->protection_absent = in[1] & 0x01;
        res->profile = (in[2] & 0xc0) >> 6;
        res->sampling_frequency_index = (in[2] & 0x3c) >> 2;
        res->private_bit = (in[2] & 0x02) >> 1;
        res->channel_configuration = ((in[2] & 0x01) << 2) | ((in[3] & 0xc0) >> 6);
        res->original_copy = (in[3] & 0x20) >> 5;
        res->home = (in[3] & 0x10) >> 4;

        res->copyright_identification_bit = (in[3] & 0x08) >> 3;
        res->copyright_identification_start = (in[3] & 0x04) >> 2;
        res->aac_frame_length = ((((in[3]) & 0x03) << 11) |
                               ((in[4] & 0xFF) << 3) |
                               (in[5] & 0xE0) >> 5);
        res->adts_buffer_fullness = ((in[5] & 0x1f) << 6 |
                                   (in[6] & 0xfc) >> 2);
        res->number_of_raw_data_block_in_frame = (in[6] & 0x03);
        return 0;
    } 
    else{
        return -1;
    }
    return 0;
}
int ps_muxer_h264_aac_test(){
    FILE *v_fp = fopen(video_path, "r");
    if(v_fp == NULL){
        printf("video_path not exist\n");
        exit(0);
    }
    FILE *a_fp = fopen(audio_path, "r");
    if(a_fp == NULL){
        printf("audio_path not exist\n");
        exit(0);
    }
    FILE *ps_fd = fopen("test_out_h264_aac.ps", "wb");
    if(ps_fd == NULL){
        printf("open test_out_h264_aac.ps failed\n");
        exit(0);
    }
    mpeg2_ps_context *context = create_ps_context();
    if(!context){
        printf("create_ps_context error\n");
        exit(0);
    }
    mpeg2_ps_set_write_callback(context, media_write_callback, 1, ps_fd);
    int ret = mpeg2_ps_add_stream(context, STREAM_TYPE_VIDEO_H264, NULL, 0);
    if(ret < 0){
        printf("mpeg2_ps_add_stream error\n");
        exit(0);
    }
    ret = mpeg2_ps_add_stream(context, STREAM_TYPE_AUDIO_AAC, NULL, 0);
    if(ret < 0){
        printf("mpeg2_ps_add_stream error\n");
        exit(0);
    }
    // video
    uint8_t frame_v[BUFFER];
    int frame_size_v = 0;
    int fps = 25;
    int start_code;
    int64_t frames_v = 0;
    int64_t pts_v = 0;
    int64_t dts_v = pts_v;
    int type;
    // audio
    uint8_t frame_a[4 * 1024];
    int64_t pts_a = 0;
    int64_t dts_a = pts_a;
    int sample_a = 44100;
    int64_t frames_a = 0;
    while (1) {
        // video
        start_code = 0;
        frame_size_v = getFrameFromH264File(v_fp, frame_v, BUFFER);
        if (frame_size_v < 0) {
            printf("read video over\n");
            break;
        }
        if(mpeg2_ps_packet_muxer(context, frame_v, frame_size_v, STREAM_TYPE_VIDEO_H264, time_base_convert(pts_v, 90000), time_base_convert(dts_v, 90000)) < 0){
            printf("mpeg2_ps_packet_muxer error\n");
        }
        start_code = get_start_code(frame_v, frame_size_v);
        // Only I/P/B frames and new access units add pts
        type = frame_v[start_code] & 0x1f;
        if((type == 5) || (type == 1)){
            if(mpeg2_h264_new_access_unit(frame_v, frame_size_v)){
                pts_v += 1000 / fps; // ms
                dts_v = pts_v;
                frames_v++;
            }
        }
        // audio
        int ret = fread(frame_a, 1, 7, a_fp);
        if (ret < 7){
            printf("read audio over\n");
            break;
        }
        adts_header header;
        if(ParseAdtsHeader(frame_a, ret, &header)){
            printf("ParseAdtsHeader error\n");
            break;
        }
        sample_a = sampling_frequencies[header.sampling_frequency_index];
        ret = fread(frame_a + 7, 1, header.aac_frame_length - 7, a_fp);
        if (ret < (header.aac_frame_length - 7)) {
            printf("read aac frame error\n");
            break;
        }
        
        if(mpeg2_ps_packet_muxer(context, frame_a, header.aac_frame_length, STREAM_TYPE_AUDIO_AAC, pts_a, dts_a) < 0){
            printf("mpeg2_ps_packet_muxer error\n");
        }
        // fwrite(frame, 1, header.aac_frame_length, aac_fd);
        pts_a += 2048; // timebase-->sample
        dts_a = pts_a;
        frames_a++;
    }
    destroy_ps_context(context);
    if(v_fp){
        fclose(v_fp);
    }
    if(a_fp){
        fclose(a_fp);
    }
    if(ps_fd){
        fclose(ps_fd);
    }
    return 0;
}
int main(int argc, char **argv){
    ps_muxer_h264_aac_test();
    return 0;
}

