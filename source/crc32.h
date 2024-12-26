#ifndef MPEG2CORE_CRC32
#define MPEG2CORE_CRC32
#include <stdio.h>
#include <stdint.h>
uint32_t mpeg2core_crc32(uint32_t crc, const uint8_t *buffer, uint32_t size);
#endif