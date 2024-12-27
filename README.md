# libmpeg2core
mpeg2 ts ps muxer/demuxer, H264/H265/MPEG1 audio/MP3/AAC/AAC_LATM/G711.
# TS
* mkdir build
* cd build
* cmake ..
* make -j4
1. demuxer
* ./mpeg2core_ts_demuxer_test ../test/h264_aac.ts
* ./mpeg2core_ts_demuxer_test ../test/h265_aac.ts
2. muxer
* ./mpeg2core_ts_muxer_test ../test/test.h264 0
* ./mpeg2core_ts_muxer_test ../test/test.h265 1
* ./mpeg2core_ts_muxer_test ../test/test.aac 2
# PS
