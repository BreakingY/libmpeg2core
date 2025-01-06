cmake_minimum_required(VERSION 3.0)
project (MPEG2COREPROJ)

include_directories(./include ./source)
aux_source_directory(./source SRC)

set(EXECUTABLE_OUTPUT_PATH ./)
add_compile_options(-g) 
set(CMAKE_BUILD_TYPE Debug) # Release
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./)

add_library(mpeg2core SHARED ${SRC})
# target_link_libraries(mpeg2core pthread)

# test
add_executable(mpeg2core_ts_demuxer_test ./test/mpeg2core_ts_demuxer_test.c)
target_link_libraries(mpeg2core_ts_demuxer_test mpeg2core)

add_executable(mpeg2core_ts_muxer_test ./test/mpeg2core_ts_muxer_test.c)
target_link_libraries(mpeg2core_ts_muxer_test mpeg2core)

add_executable(mpeg2core_ps_demuxer_test ./test/mpeg2core_ps_demuxer_test.c)
target_link_libraries(mpeg2core_ps_demuxer_test mpeg2core)

add_executable(mpeg2core_ps_muxer_test ./test/mpeg2core_ps_muxer_test.c)
target_link_libraries(mpeg2core_ps_muxer_test mpeg2core)