# libmpeg2core
mpeg2 ts ps muxer/demuxer, H264/H265/MPEG1 AUDIO/MP3/AAC/AAC_LATM/G711.
# compile
* mkdir build
* cd build
* cmake ..
* make -j4
# TS
1. demuxer
* ./mpeg2core_ts_demuxer_test ../media/h264_aac.ts
* ./mpeg2core_ts_demuxer_test ../media/h265_aac.ts
2. muxer
* ./mpeg2core_ts_muxer_test ../media/test.h264 0
* ./mpeg2core_ts_muxer_test ../media/test.h265 1
* ./mpeg2core_ts_muxer_test ../media/test.aac 2
# PS
1. demuxer
* ./mpeg2core_ps_demuxer_test ../media/h264_mpeg2.ps
* ./mpeg2core_ps_demuxer_test ../media/h264_mpeg1.ps
* ./mpeg2core_ps_demuxer_test ../media/h265_mpeg1.ps
2. muxer
* ./mpeg2core_ps_muxer_test ../media/test.h264 0
* ./mpeg2core_ps_muxer_test ../media/test.h265 1
* ./mpeg2core_ps_muxer_test ../media/test.aac 2
* ./mpeg2core_ps_muxer_test_av(Audio and video, for testing purposes only, without audio and video synchronization)
