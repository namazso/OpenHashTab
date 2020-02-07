// public domain
#pragma once

#ifndef EXTERN_C_START
#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif
#endif

EXTERN_C_START

#include <stddef.h>
#include <stdint.h>

extern uint32_t Crc32_ComputeBuf(uint32_t inCrc32, const void* buf, size_t bufLen);

EXTERN_C_END