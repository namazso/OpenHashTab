//    Copyright 2019 namazso <admin@namazso.eu>
//    mbedtls wrapper for sha3 implementation from https://github.com/HarryR/SHA3IUF
//
//               DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                       Version 2, December 2004
//    
//    Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
//    
//    Everyone is permitted to copy and distribute verbatim or modified
//    copies of this license document, and changing it is allowed as long
//    as the name is changed.
//    
//               DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//      TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//    
//     0. You just DO WHAT THE FUCK YOU WANT TO.
#include "sha3.h"
#include <mbedtls/md_internal.h>
#include <mbedtls/platform.h>
#include <assert.h>
#include <string.h>

static int sha3_256_starts_wrap(void* ctx)
{
  sha3_Init256(ctx);
  return(0);
}

static int sha3_384_starts_wrap(void* ctx)
{
  sha3_Init384(ctx);
  return(0);
}

static int sha3_512_starts_wrap(void* ctx)
{
  sha3_Init512(ctx);
  return(0);
}

static int sha3_update_wrap(void* ctx, const unsigned char* input, size_t ilen)
{
  sha3_Update(ctx, input, ilen);
  return(0);
}

static int sha3_256_finish_wrap(void* ctx, unsigned char* output)
{
  memcpy(output, sha3_Finalize(ctx), 256 / 8);
  return(0);
}

static int sha3_384_finish_wrap(void* ctx, unsigned char* output)
{
  memcpy(output, sha3_Finalize(ctx), 384 / 8);
  return(0);
}

static int sha3_512_finish_wrap(void* ctx, unsigned char* output)
{
  memcpy(output, sha3_Finalize(ctx), 512 / 8);
  return(0);
}

static int sha3_256_digest_wrap(const unsigned char* input, size_t ilen, unsigned char* output)
{
  sha3_context ctx;
  sha3_Init256(&ctx);
  sha3_Update(&ctx, input, ilen);
  memcpy(output, sha3_Finalize(&ctx), 256 / 8);
  return(0);
}

static int sha3_384_digest_wrap(const unsigned char* input, size_t ilen, unsigned char* output)
{
  sha3_context ctx;
  sha3_Init384(&ctx);
  sha3_Update(&ctx, input, ilen);
  memcpy(output, sha3_Finalize(&ctx), 384 / 8);
  return(0);
}

static int sha3_512_digest_wrap(const unsigned char* input, size_t ilen, unsigned char* output)
{
  sha3_context ctx;
  sha3_Init384(&ctx);
  sha3_Update(&ctx, input, ilen);
  memcpy(output, sha3_Finalize(&ctx), 384 / 8);
  return(0);
}

static void* sha3_ctx_alloc(void)
{
  void* ctx = mbedtls_calloc(1, sizeof(sha3_context));
  return(ctx);
}

static void sha3_ctx_free(void* ctx)
{
  mbedtls_free(ctx);
}

static void sha3_clone_wrap(void* dst, const void* src)
{
  *(sha3_context*)dst = *(const sha3_context*)src;
}

static int sha3_process_wrap(void* ctx, const unsigned char* data)
{
  sha3_Update(ctx, data, 8);
  return(0);
}

const mbedtls_md_info_t sha3_256_info = {
    0,
    "sha3-256",
    32,
    8,
    sha3_256_starts_wrap,
    sha3_update_wrap,
    sha3_256_finish_wrap,
    sha3_256_digest_wrap,
    sha3_ctx_alloc,
    sha3_ctx_free,
    sha3_clone_wrap,
    sha3_process_wrap,
};

const mbedtls_md_info_t sha3_384_info = {
    0,
    "sha3-384",
    48,
    8,
    sha3_384_starts_wrap,
    sha3_update_wrap,
    sha3_384_finish_wrap,
    sha3_384_digest_wrap,
    sha3_ctx_alloc,
    sha3_ctx_free,
    sha3_clone_wrap,
    sha3_process_wrap,
};

const mbedtls_md_info_t sha3_512_info = {
    0,
    "sha3-512",
    64,
    8,
    sha3_512_starts_wrap,
    sha3_update_wrap,
    sha3_512_finish_wrap,
    sha3_512_digest_wrap,
    sha3_ctx_alloc,
    sha3_ctx_free,
    sha3_clone_wrap,
    sha3_process_wrap,
};
