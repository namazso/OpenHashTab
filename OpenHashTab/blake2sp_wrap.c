//    Copyright 2019 namazso <admin@namazso.eu>
//    mbedtls wrapper for 7zip blake2sp implementation
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
#include "blake2sp.h"
#include <mbedtls/md_internal.h>
#include <mbedtls/platform.h>
#include <assert.h>

static int blake2sp_starts_wrap(void *ctx)
{
  Blake2sp_Init((CBlake2sp *)ctx);
  return(0);
}

static int blake2sp_update_wrap(void *ctx, const unsigned char *input,
  size_t ilen)
{
  Blake2sp_Update((CBlake2sp *)ctx, input, ilen);
  return(0);
}

static int blake2sp_finish_wrap(void *ctx, unsigned char *output)
{
  Blake2sp_Final((CBlake2sp *)ctx, output);
  return(0);
}

static int blake2sp_digest_wrap(const unsigned char *input,
  size_t ilen,
  unsigned char* output)
{
  CBlake2sp ctx;
  Blake2sp_Init(&ctx);
  Blake2sp_Update(&ctx, input, ilen);
  Blake2sp_Final(&ctx, output);
  return(0);
}

static void *blake2sp_ctx_alloc(void)
{
  void *ctx = mbedtls_calloc(1, sizeof(CBlake2sp));

  if (ctx != NULL)
    Blake2sp_Init((CBlake2sp *)ctx);

  return(ctx);
}

static void blake2sp_ctx_free(void *ctx)
{
  mbedtls_free(ctx);
}

static void blake2sp_clone_wrap(void *dst, const void *src)
{
  *(CBlake2sp *)dst = *(const CBlake2sp *)src;
}

static int blake2sp_process_wrap(void *ctx, const unsigned char *data)
{
  Blake2sp_Update((CBlake2sp*)ctx, data, BLAKE2S_BLOCK_SIZE);
  return(0);
}

const mbedtls_md_info_t blake2sp_info = {
    0,
    "BLAKE2sp",
    32,
    BLAKE2S_BLOCK_SIZE,
    blake2sp_starts_wrap,
    blake2sp_update_wrap,
    blake2sp_finish_wrap,
    blake2sp_digest_wrap,
    blake2sp_ctx_alloc,
    blake2sp_ctx_free,
    blake2sp_clone_wrap,
    blake2sp_process_wrap,
};
