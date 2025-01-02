#include "mpeg2core_ps.h"
static char *h26x_filename = "test_out.h26x"; 
static FILE *h26x_fd = NULL;
static void video_read_callback(int type, int64_t pts, int64_t dts, uint8_t *data, int data_len, void *arg){
    switch (type){
        case STREAM_TYPE_VIDEO_H264:
            // printf("STREAM_TYPE_VIDEO_H264\n");
            break;
        case STREAM_TYPE_VIDEO_HEVC:
            // printf("STREAM_TYPE_VIDEO_HEVC\n");
            break;
        case PES_H264_ID: // MPEG1
            // printf("PES_H264_ID\n");
            break;
        default: // unknow type
            break;
    }
    if(h26x_fd == NULL){
        h26x_fd = fopen(h26x_filename, "wb");
    }
    fwrite(data, 1, data_len, h26x_fd);
    // printf("pts: %" PRIu64 " dts: %" PRIu64 "\n", pts, dts);
    // printf("data_len: %d\n", data_len);
    return;
}
static char *aac_filename = "test_out.aac"; 
static FILE *aac_fd = NULL;
static void audio_read_callback(int type, int64_t pts, int64_t dts, uint8_t *data, int data_len, void *arg){
    switch (type){
        case STREAM_TYPE_AUDIO_AAC:
            // printf("STREAM_TYPE_AUDIO_AAC\n");
            break;
        case STREAM_TYPE_AUDIO_MPEG1:
            // printf("STREAM_TYPE_AUDIO_MPEG1\n");
            break;
        case STREAM_TYPE_AUDIO_MP3:
            // printf("STREAM_TYPE_AUDIO_MP3\n");
        case STREAM_TYPE_AUDIO_AAC_LATM:
            // printf("STREAM_TYPE_AUDIO_AAC_LATM\n");
            break;
        case STREAM_TYPE_AUDIO_G711A:
            // printf("STREAM_TYPE_AUDIO_G711A\n");
            break;
        case STREAM_TYPE_AUDIO_G711U:
            // printf("STREAM_TYPE_AUDIO_G711U\n");
            break;
        default: // unknow type
            break;
    }
    if(aac_fd == NULL){
        aac_fd = fopen(aac_filename, "wb");
    }
    fwrite(data, 1, data_len, aac_fd);
    // printf("pts: %" PRIu64 " dts: %" PRIu64 "\n", pts, dts);
    // printf("data_len: %d\n", data_len);
    return;
}
int ps_demuxer_test(int argc, char **argv){
    if(argc < 2){
        printf("./bin input(ps)\n");
        exit(0);
    }
    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL){
        printf("file not exist\n");
        exit(0);
    }
    mpeg2_ps_context *context = create_ps_context();
    if(!context){
        printf("create_ts_context error\n");
        exit(0);
    }
    mpeg2_ps_set_read_callback(context, video_read_callback, audio_read_callback, NULL);
    unsigned char buffer[1 * 1024 *1024];
    memset(buffer, 0, sizeof(buffer));
    int ret, bytes;
    while(1){
        bytes = fread(buffer, 1, sizeof(buffer), fp);
        if(bytes <= 0){
            break;
        }
        ret = mpeg2_ps_packet_demuxer(context, buffer, bytes);
        if(ret < 0){
            printf("mpeg2_ps_packet_demuxer error\n");
            continue;
        }

    }
    destroy_ps_context(context);
    if(h26x_fd){
        fclose(h26x_fd);
    }
    if(aac_fd){
        fclose(aac_fd);
    }
    return 0;
}
int main(int argc, char **argv){
    ps_demuxer_test(argc, argv);
    return 0;
}

