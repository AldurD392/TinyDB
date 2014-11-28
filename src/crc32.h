#ifndef __crc32_h
#define __crc32_h

#include <sys/param.h>
#include <stdint.h>

#define CRC_HASHING_CONSTANT 2

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
#endif
