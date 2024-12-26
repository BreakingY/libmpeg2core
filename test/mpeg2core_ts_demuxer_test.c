#include "mpeg2core_common.h"
#include "mpeg2core_ts.h"
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
        default:
            return;
            break;
    }
    if(h26x_fd == NULL){
        h26x_fd = fopen(h26x_filename, "wb");
    }
    fwrite(data, 1, data_len, h26x_fd);
    int start_code = get_start_code(data, data_len);
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
        case STREAM_TYPE_AUDIO_MPEG2:
            // printf("STREAM_TYPE_AUDIO_MPEG2\n");
        case STREAM_TYPE_AUDIO_AAC_LATM:
            // printf("STREAM_TYPE_AUDIO_AAC_LATM\n");
            break;
        case STREAM_TYPE_AUDIO_G711A:
            // printf("STREAM_TYPE_AUDIO_G711A\n");
            break;
        case STREAM_TYPE_AUDIO_G711U:
            // printf("STREAM_TYPE_AUDIO_G711U\n");
            break;
        default:
            return;
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
int ts_demuxer_test(int argc, char **argv){
    if(argc < 2){
        printf("./bin input(ts)\n");
        exit(0);
    }
    FILE *fp = fopen(argv[1], "r");
    int ts_packet_length = probe_ts_packet_length(fp);
    printf("ts_packet_length:%d\n", ts_packet_length);
    if(ts_packet_length < 0){
        exit(0);
    }
    mpeg2_ts_context *context = create_ts_context();
    if(!context){
        printf("create_ts_context error\n");
        exit(0);
    }
    mpeg2_set_read_callback(context, video_read_callback, audio_read_callback, NULL);
    unsigned char *buffer = (unsigned char *)malloc(ts_packet_length + 1);
    memset(buffer, 0, ts_packet_length + 1);
    int ret;
    while(fread(buffer, 1, ts_packet_length, fp) == ts_packet_length){
        ret = mpeg2_ts_packet_demuxer(context, buffer, ts_packet_length);
        if(ret < 0){
            printf("mpeg2_ts_packet_demuxer error\n");
            continue;
        }
        switch (context->ts_header.pid){
            case PID_PAT:
                // printf("PID_PAT\n");
                // dump_section_header(context->section_header);
                // dump_program(context->pat);
                break;
            case PID_CAT:
                // printf("PID_CAT\n");
                break;
            case PID_SDT:
                // printf("SDT、BAT、ST\n");
                // dump_section_header(context->section_header);
                // for(int i = 0; i < context->sdt.sdt_info_array_num; i++){
                //     printf("%s\n", context->sdt.sdt_info_array[i].descriptor);
                // }
                break;
            default:
                break;
        }
        // if(context->ts_header.PCR){
        //     printf("PCR:%" PRIu64 "\n", context->ts_header.PCR);
        // }
        // dump_ts_header(context->ts_header);
        // dump_pmt_array(context->pmt_array, context->pmt_array_num);
        memset(buffer, 0, ts_packet_length + 1);
    }
    free(buffer);
    destroy_ts_context(context);
    if(h26x_fd){
        fclose(h26x_fd);
    }
    if(aac_fd){
        fclose(aac_fd);
    }
    return 0;
}
int main(int argc, char **argv){
    ts_demuxer_test(argc, argv);
    return 0;
}

