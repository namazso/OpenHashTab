// Public domain
// Based on public domain 7zip implementation by Igor Pavlov and Samuel Neves
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Set this if your processor is unaligned little endian
#define LITTLE_ENDIAN_UNALIGNED

#define BLAKE2S_BLOCK_SIZE 64
#define BLAKE2S_DIGEST_SIZE 32
#define BLAKE2SP_PARALLEL_DEGREE 8

typedef struct
{
  uint32_t h[8];
  uint32_t t[2];
  uint32_t f[2];
  uint8_t buf[BLAKE2S_BLOCK_SIZE];
  uint32_t bufPos;
  uint32_t lastNode_f1;
  uint32_t dummy[2]; /* for sizeof(CBlake2s) alignment */
} CBlake2s;

typedef struct
{
  CBlake2s S[BLAKE2SP_PARALLEL_DEGREE];
  unsigned bufPos;
} CBlake2sp;

void Blake2sp_Init(CBlake2sp *p);
void Blake2sp_Update(CBlake2sp *p, const uint8_t *data, size_t size);
void Blake2sp_Final(CBlake2sp *p, uint8_t *digest);

EXTERN_C_END
