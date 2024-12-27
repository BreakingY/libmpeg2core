#include "mpeg2core_common.h"
#include "mpeg2core_ts.h"
#include <time.h>

#define BUFFER 2 * 1024 * 1024
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
static void media_write_callback(int program_number, int stream_pid, int stream_type, uint8_t *data, int data_len, void *arg){
    switch (stream_type){
        case STREAM_TYPE_AUDIO_AAC:
            // printf("STREAM_TYPE_AUDIO_AAC\n");
            break;
        case STREAM_TYPE_AUDIO_MPEG1:
            // printf("STREAM_TYPE_AUDIO_MPEG1\n");
            break;
        case STREAM_TYPE_AUDIO_MPEG2:
            // printf("STREAM_TYPE_AUDIO_MPEG2\n");
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
            // printf("PSI SI\n");
            break;
    }
    FILE *ts_fd = (FILE *)arg;
    if(ts_fd){
        fwrite(data, 1, data_len, ts_fd);
    }
    return;
}
// STREAM_TYPE_VIDEO_H264 STREAM_TYPE_VIDEO_H265
int ts_muxer_h26x_test(const char *file, int stream_type_h26x){
    uint16_t program_number = 1;
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        printf("file not exist\n");
        exit(0);
    }
    FILE *ts_fd = fopen("test_out_h26x.ts", "wb");
    if(ts_fd == NULL){
        printf("open test_out_h26x.ts failed\n");
        exit(0);
    }
    mpeg2_ts_context *context = create_ts_context();
    if(!context){
        printf("create_ts_context error\n");
        exit(0);
    }
    mpeg2_set_write_callback(context, media_write_callback, ts_fd);
    if(mpeg2_ts_add_program(context, program_number, NULL, 0) < 0){
        printf("mpeg2_ts_add_program error\n");
        exit(0);
    }
    int stream_pid_h26x = mpeg2_ts_add_program_stream(context, program_number, stream_type_h26x, NULL, 0);
    if(stream_pid_h26x < 0){
        printf("mpeg2_ts_add_program_stream error\n");
        exit(0);
    }
    uint8_t frame[BUFFER];
    int frame_size = 0;
    int fps = 25;
    int start_code;
    int64_t frames = 0;
    int64_t pts = 0;
    int64_t dts = pts;
    int64_t last = 0;
    while (1) {
        start_code = 0;
        frame_size = getFrameFromH264File(fp, frame, BUFFER);
        if (frame_size < 0) {
            printf("read over\n");
            break;
        }
        if(mpeg2_ts_packet_muxer(context, stream_pid_h26x, frame, frame_size, stream_type_h26x, time_base_convert(pts, 90000), time_base_convert(dts, 90000)) < 0){
            printf("mpeg2_ts_packet_muxer error\n");
        }
        start_code = get_start_code(frame, frame_size);
        int type;
        // Only I/P/B frames and new access units add pts
        switch (stream_type_h26x){
            case STREAM_TYPE_VIDEO_H264:
                type = frame[start_code] & 0x1f;
                if((type == 5) || (type == 1)){
                    if(mpeg2_h264_new_access_unit(frame, frame_size)){
                        pts += 1000 / fps; // ms
                        dts = pts;
                        frames++;
                    }
                }
                break;
            case STREAM_TYPE_VIDEO_H265:
                type = (frame[start_code] >> 1) & 0x3f;
                if((type == 19) || (type == 1) || (type == 20) || (type == 21)){
                    if(mpeg2_h265_new_access_unit(frame, frame_size)){
                        pts += 1000 / fps; // ms
                        dts = pts;
                        frames++;
                    } 
                }
                break;
            default:
                break;
        }
        
        // printf("type:%d frame_size:%d pts:%d dts:%d\n", type, frame_size, time_base_convert(pts, 90000), time_base_convert(dts, 90000));
        last = time_base_convert(pts, 90000);
    }
    if(mpeg2_ts_remove_program(context, program_number) < 0){
        printf("mpeg2_ts_remove_program error\n");
        exit(0);
    }
    destroy_ts_context(context);
    if(ts_fd){
        fclose(ts_fd);
    }
    if(fp){
        fclose(fp);
    }
    return 0;
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
int ts_muxer_aac_test(const char *file){
    uint16_t program_number = 1;
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        printf("file not exist\n");
        exit(0);
    }
    FILE *ts_fd = fopen("test_out_aac.ts", "wb");
    if(ts_fd == NULL){
        printf("open test_out_aac.ts failed\n");
        exit(0);
    }
    // FILE *aac_fd = fopen("ts_muxer_aac_test.aac", "wb");
    mpeg2_ts_context *context = create_ts_context();
    if(!context){
        printf("create_ts_context error\n");
        exit(0);
    }
    mpeg2_set_write_callback(context, media_write_callback, ts_fd);
    if(mpeg2_ts_add_program(context, program_number, NULL, 0) < 0){
        printf("mpeg2_ts_add_program error\n");
        exit(0);
    }
    int stream_pid_aac = mpeg2_ts_add_program_stream(context, program_number, STREAM_TYPE_AUDIO_AAC, NULL, 0);
    if(stream_pid_aac < 0){
        printf("mpeg2_ts_add_program_stream error\n");
        exit(0);
    }
    uint8_t frame[4 * 1024];
    int64_t pts = 0;
    int64_t dts = pts;
    int sample = 44100;
    while (1) {
        int ret = fread(frame, 1, 7, fp);
        if (ret < 7){
            printf("read over\n");
            break;
        }
        adts_header header;
        if(ParseAdtsHeader(frame, ret, &header)){
            printf("ParseAdtsHeader error\n");
            break;
        }
        sample = sampling_frequencies[header.sampling_frequency_index];
        // aac_frame_length = (protection_absent == 1 ? 7 : 9) + size(AACFrame)
        // printf("sample:%d aac_frame_length:%d protection_absent:%d\n", sample, header.aac_frame_length, header.protection_absent);
        ret = fread(frame + 7, 1, header.aac_frame_length - 7, fp);
        if (ret < (header.aac_frame_length - 7)) {
            printf("read aac frame error\n");
            break;
        }
        
        if(mpeg2_ts_packet_muxer(context, stream_pid_aac, frame, header.aac_frame_length, STREAM_TYPE_AUDIO_AAC, pts, dts) < 0){
            printf("mpeg2_ts_packet_muxer error\n");
        }
        // fwrite(frame, 1, header.aac_frame_length, aac_fd);
        pts += 2048; // timebase-->sample
        dts = pts;
        // printf("pts:%lld time_base_convert(pts, sample):%lld\n",pts, time_base_convert(pts, sample));
    }
    if(mpeg2_ts_remove_program(context, program_number) < 0){
        printf("mpeg2_ts_remove_program error\n");
        exit(0);
    }
    destroy_ts_context(context);
    if(ts_fd){
        fclose(ts_fd);
    }
    if(fp){
        fclose(fp);
    }
    return 0;
}
int main(int argc, char **argv){
    if(argc < 3){
        printf("./bin input(*.h264/*.h265/*.aac) type(0-h264 1-h265 2-aac)\n");
        exit(0);
    }
    int type = atoi(argv[2]);
    switch (type){
        case 0:
            ts_muxer_h26x_test(argv[1], STREAM_TYPE_VIDEO_H264);
            break;
        case 1:
            ts_muxer_h26x_test(argv[1], STREAM_TYPE_VIDEO_H265);
            break;
        case 2:
            ts_muxer_aac_test(argv[1]);
            break;
        default:
            break;
    }
    return 0;
}

