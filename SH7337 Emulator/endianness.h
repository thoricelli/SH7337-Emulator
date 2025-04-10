#pragma once

#include <stdint.h>

//Macros
#define SWAP_UINT16(val) (((val) << 8) | ((val) >> 8))
#define SWAP_INT16(val)  (((val) << 8) | (((val) >> 8) & 0xFF))

#define SWAP_UINT32(val) ((((val) << 8) & 0xFF00FF00) | (((val) >> 8) & 0xFF00FF) | ((val) << 16) | ((val) >> 16))
#define SWAP_INT32(val)  ((((val) << 8) & 0xFF00FF00) | (((val) >> 8) & 0xFF00FF) | ((val) << 16) | (((val) >> 16) & 0xFFFF))

#define SWAP_UINT64(val) ((((val) << 8) & 0xFF00FF00FF00FF00ULL) | (((val) >> 8) & 0x00FF00FF00FF00FFULL) | \
                          (((val) << 16) & 0xFFFF0000FFFF0000ULL) | (((val) >> 16) & 0x0000FFFF0000FFFFULL) | \
                          ((val) << 32) | ((val) >> 32))

#define SWAP_INT64(val)  ((((val) << 8) & 0xFF00FF00FF00FF00ULL) | (((val) >> 8) & 0x00FF00FF00FF00FFULL) | \
                          (((val) << 16) & 0xFFFF0000FFFF0000ULL) | (((val) >> 16) & 0x0000FFFF0000FFFFULL) | \
                          ((val) << 32) | (((val) >> 32) & 0xFFFFFFFFULL))

//Functions
uint16_t swap_uint16(uint16_t val);
int16_t swap_int16(int16_t val);
uint32_t swap_uint32(uint32_t val);
int32_t swap_int32(int32_t val);
int64_t swap_int64(int64_t val);
uint64_t swap_uint64(uint64_t val);