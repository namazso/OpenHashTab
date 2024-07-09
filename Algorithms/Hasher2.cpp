//    Copyright 2019-2023 namazso <admin@namazso.eu>
//    This file is part of OpenHashTab.
//
//    OpenHashTab is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    OpenHashTab is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.
#include "Hasher2.h"

#include <new>
#include <numeric>
#include <limits>

#include <mbedtls/md.h>
#include <mbedtls/md2.h>
#include <mbedtls/md4.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/ripemd160.h>
#include "blake2sp.h"
#include "Hasher2.h"
#include <Crc32.h>
#include <blake3.h>
extern "C" {
#include "KeccakHash.h"
#include "KangarooTwelve.h"
#include "SP800-185.h"
}
#include "crc64.h"
#include <quickxorhash.h>

#define XXH_STATIC_LINKING_ONLY

#include <xxhash.h>

extern "C" {
#define uint512_u uint512_u_STREEBOG
#include <gost3411-2012-core.h>
#undef uint512_u
}

class HashContext{};

template <
  typename Ctx,
  size_t Size,
  void (*Init)(Ctx* ctx),
  int (*StartsRet)(Ctx* ctx),
  void (*Free)(Ctx* ctx),
  int (*UpdateRet)(Ctx* ctx, const unsigned char*, size_t),
  int (*FinishRet)(Ctx* ctx, unsigned char*)
>
class MbedHashContext final : public HashContext
{
  Ctx ctx{};

public:
  MbedHashContext()
  {
    Init(&ctx);
    StartsRet(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    UpdateRet(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    FinishRet(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return Size;
  }
};

#define MBED_HASH_CONTEXT_TYPE(name, size) MbedHashContext<\
  mbedtls_ ## name ## _context,\
  (size),\
  &mbedtls_ ## name ## _init,\
  &mbedtls_ ## name ## _starts_ret,\
  &mbedtls_ ## name ## _free,\
  &mbedtls_ ## name ## _update_ret,\
  &mbedtls_ ## name ## _finish_ret\
>

template <bool is224>
int sha256_starts_ret_binder(mbedtls_sha256_context* ctx) { return mbedtls_sha256_starts_ret(ctx, is224); }

template <bool is384>
int sha512_starts_ret_binder(mbedtls_sha512_context* ctx) { return mbedtls_sha512_starts_ret(ctx, is384); }

using Md2HashContext = MBED_HASH_CONTEXT_TYPE(md2, 16);
using Md4HashContext = MBED_HASH_CONTEXT_TYPE(md4, 16);
using Md5HashContext = MBED_HASH_CONTEXT_TYPE(md5, 16);
using RipeMD160HashContext = MBED_HASH_CONTEXT_TYPE(ripemd160, 20);
using Sha1HashContext = MBED_HASH_CONTEXT_TYPE(sha1, 20);
using Sha224HashContext = MbedHashContext<
  mbedtls_sha256_context,
  28,
  &mbedtls_sha256_init,
  &sha256_starts_ret_binder<true>,
  &mbedtls_sha256_free,
  &mbedtls_sha256_update_ret,
  &mbedtls_sha256_finish_ret
>;
using Sha256HashContext = MbedHashContext<
  mbedtls_sha256_context,
  32,
  &mbedtls_sha256_init,
  &sha256_starts_ret_binder<false>,
  &mbedtls_sha256_free,
  &mbedtls_sha256_update_ret,
  &mbedtls_sha256_finish_ret
>;
using Sha384HashContext = MbedHashContext<
  mbedtls_sha512_context,
  48,
  &mbedtls_sha512_init,
  &sha512_starts_ret_binder<true>,
  &mbedtls_sha512_free,
  &mbedtls_sha512_update_ret,
  &mbedtls_sha512_finish_ret
>;
using Sha512HashContext = MbedHashContext<
  mbedtls_sha512_context,
  64,
  &mbedtls_sha512_init,
  &sha512_starts_ret_binder<false>,
  &mbedtls_sha512_free,
  &mbedtls_sha512_update_ret,
  &mbedtls_sha512_finish_ret
>;

class Blake2SpHashContext final : public HashContext
{
  CBlake2sp ctx{};

public:
  Blake2SpHashContext()
  {
    Blake2sp_Init(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    Blake2sp_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    Blake2sp_Final(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return BLAKE2S_DIGEST_SIZE;
  }
};

class Crc32HashContext final : public HashContext
{
  uint32_t crc{};

public:
  Crc32HashContext() {}

  void Update(const void* data, size_t size)
  {
    crc = crc32_fast(data, size, crc);
  }

  void Finish(uint8_t* out)
  {
    out[0] = 0xFF & (crc >> 24);
    out[1] = 0xFF & (crc >> 16);
    out[2] = 0xFF & (crc >> 8);
    out[3] = 0xFF & (crc >> 0);
  }

  size_t GetOutputSize()
  {
    return 4;
  }
};

class Crc64HashContext final : public HashContext
{
  uint64_t crc{};

public:
  Crc64HashContext() {}

  void Update(const void* data, size_t size)
  {
    crc = crc64(crc, data, size);
  }

  void Finish(uint8_t* out)
  {
    out[0] = 0xFF & (crc >> 56);
    out[1] = 0xFF & (crc >> 48);
    out[2] = 0xFF & (crc >> 40);
    out[3] = 0xFF & (crc >> 32);
    out[4] = 0xFF & (crc >> 24);
    out[5] = 0xFF & (crc >> 16);
    out[6] = 0xFF & (crc >> 8);
    out[7] = 0xFF & (crc >> 0);
  }

  size_t GetOutputSize()
  {
    return 8;
  }
};

template <bool ExtraNullVersion>
class ED2kHashContext final : public HashContext
{
  mbedtls_md4_context current_chunk{};
  mbedtls_md4_context root_hash{};
  uint8_t last_chunk_hash[16] = { 0x31, 0xd6, 0xcf, 0xe0, 0xd1, 0x6a, 0xe9, 0x31, 0xb7, 0x3c, 0x59, 0xd7, 0xe0, 0xc0, 0x89, 0xc0 };
  uint64_t hashed{};

  constexpr static auto k_chunk_size = 9728000;

  void UpdateInternal(const void* data, size_t size)
  {
    if (size == 0)
      return;

    mbedtls_md4_update_ret(&current_chunk, (const uint8_t*)data, size);
    hashed += size;

    if (hashed % k_chunk_size == 0)
    {
      mbedtls_md4_finish_ret(&current_chunk, last_chunk_hash);

      mbedtls_md4_free(&current_chunk);
      mbedtls_md4_init(&current_chunk);
      mbedtls_md4_starts_ret(&current_chunk);

      mbedtls_md4_update_ret(&root_hash, last_chunk_hash, sizeof(last_chunk_hash));
    }
  }

public:
  ED2kHashContext()
  {
    mbedtls_md4_init(&current_chunk);
    mbedtls_md4_starts_ret(&current_chunk);

    mbedtls_md4_init(&root_hash);
    mbedtls_md4_starts_ret(&root_hash);
  }

  void Update(const void* data, size_t size)
  {
    const auto bytes = (const uint8_t*)data;
    const auto needed_for_next_chunk = (size_t)(((hashed / k_chunk_size) + 1) * k_chunk_size - hashed);
    const auto first_part = std::min(needed_for_next_chunk, size);
    const auto second_part = size - first_part;

    UpdateInternal(bytes, first_part);
    if (second_part)
      UpdateInternal(bytes + first_part, second_part);
  }

  void Finish(uint8_t* out)
  {
    if (hashed < k_chunk_size)
    {
      mbedtls_md4_finish_ret(&current_chunk, out);
      return;
    }

    if (!ExtraNullVersion)
    {
      if (hashed == k_chunk_size)
      {
        memcpy(out, last_chunk_hash, sizeof(last_chunk_hash));
        return;
      }
      if (hashed % k_chunk_size == 0)
      {
        mbedtls_md4_finish_ret(&root_hash, out);
        return;
      }
    }

    mbedtls_md4_context copy_root_hash;
    mbedtls_md4_clone(&copy_root_hash, &root_hash);

    uint8_t partial_chunk[16]{};
    mbedtls_md4_finish_ret(&current_chunk, partial_chunk);
    mbedtls_md4_update_ret(&copy_root_hash, partial_chunk, sizeof(partial_chunk));
    mbedtls_md4_finish_ret(&copy_root_hash, out);
    mbedtls_md4_free(&copy_root_hash);
  }

  size_t GetOutputSize()
  {
    return 16;
  }
};

class Blake3HashContext final : public HashContext
{
  blake3_hasher ctx{};

  size_t out_len{};

public:
  constexpr static const char* k_params[] = {
    "Bits"
  };

  static size_t ParamCheck(const uint64_t* params)
  {
    const auto len = params[0];
    if (len % 8)
      return 0;
    if (len > std::numeric_limits<size_t>::max())
      return 0;
    return len / 8;
  }

  Blake3HashContext(const uint64_t* params)
    : out_len(params[0] / 8)
  {
    blake3_hasher_init(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    blake3_hasher_update(&ctx, data, size);
  }

  void Finish(uint8_t* out)
  {
    blake3_hasher_finalize(&ctx, out, out_len);
  }

  size_t GetOutputSize()
  {
    return out_len;
  }
};

class XXH32HashContext final : public HashContext
{
  XXH32_state_t ctx{};

public:
  XXH32HashContext()
  {
    XXH32_reset(&ctx, 0);
  }

  void Update(const void* data, size_t size)
  {
    XXH32_update(&ctx, data, size);
  }

  void Finish(uint8_t* out)
  {
    const auto xxh32 = XXH32_digest(&ctx);
    out[0] = 0xFF & (xxh32 >> 24);
    out[1] = 0xFF & (xxh32 >> 16);
    out[2] = 0xFF & (xxh32 >> 8);
    out[3] = 0xFF & (xxh32 >> 0);
  }

  size_t GetOutputSize()
  {
    return 4;
  }
};

class XXH64HashContext final : public HashContext
{
  XXH64_state_t ctx{};

public:
  XXH64HashContext()
  {
    XXH64_reset(&ctx, 0);
  }

  void Update(const void* data, size_t size)
  {
    XXH64_update(&ctx, data, size);
  }

  void Finish(uint8_t* out)
  {
    const auto xxh64 = XXH64_digest(&ctx);
    out[0] = 0xFF & (xxh64 >> 56);
    out[1] = 0xFF & (xxh64 >> 48);
    out[2] = 0xFF & (xxh64 >> 40);
    out[3] = 0xFF & (xxh64 >> 32);
    out[4] = 0xFF & (xxh64 >> 24);
    out[5] = 0xFF & (xxh64 >> 16);
    out[6] = 0xFF & (xxh64 >> 8);
    out[7] = 0xFF & (xxh64 >> 0);
  }

  size_t GetOutputSize()
  {
    return 8;
  }
};

class XXH3_64bitsHashContext final : public HashContext
{
  XXH3_state_t ctx{};

public:
  XXH3_64bitsHashContext()
  {
    XXH3_64bits_reset(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    XXH3_64bits_update(&ctx, data, size);
  }

  void Finish(uint8_t* out)
  {
    const auto xxh64 = XXH3_64bits_digest(&ctx);
    out[0] = 0xFF & (xxh64 >> 56);
    out[1] = 0xFF & (xxh64 >> 48);
    out[2] = 0xFF & (xxh64 >> 40);
    out[3] = 0xFF & (xxh64 >> 32);
    out[4] = 0xFF & (xxh64 >> 24);
    out[5] = 0xFF & (xxh64 >> 16);
    out[6] = 0xFF & (xxh64 >> 8);
    out[7] = 0xFF & (xxh64 >> 0);
  }

  size_t GetOutputSize()
  {
    return 8;
  }
};

class XXH3_128bitsHashContext final : public HashContext
{
  XXH3_state_t ctx{};

public:
  XXH3_128bitsHashContext()
  {
    XXH3_128bits_reset(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    XXH3_128bits_update(&ctx, data, size);
  }

  void Finish(uint8_t* out)
  {
    const auto xxh128 = XXH3_128bits_digest(&ctx);
    out[0] = 0xFF & (xxh128.high64 >> 56);
    out[1] = 0xFF & (xxh128.high64 >> 48);
    out[2] = 0xFF & (xxh128.high64 >> 40);
    out[3] = 0xFF & (xxh128.high64 >> 32);
    out[4] = 0xFF & (xxh128.high64 >> 24);
    out[5] = 0xFF & (xxh128.high64 >> 16);
    out[6] = 0xFF & (xxh128.high64 >> 8);
    out[7] = 0xFF & (xxh128.high64 >> 0);
    out[8] = 0xFF & (xxh128.low64 >> 56);
    out[9] = 0xFF & (xxh128.low64 >> 48);
    out[10] = 0xFF & (xxh128.low64 >> 40);
    out[11] = 0xFF & (xxh128.low64 >> 32);
    out[12] = 0xFF & (xxh128.low64 >> 24);
    out[13] = 0xFF & (xxh128.low64 >> 16);
    out[14] = 0xFF & (xxh128.low64 >> 8);
    out[15] = 0xFF & (xxh128.low64 >> 0);
  }

  size_t GetOutputSize()
  {
    return 16;
  }
};

class KeccakHashContext final : public HashContext
{
  Keccak_HashInstance ctx{};

public:
  constexpr static const char* k_params[] = {
    "Rate",
    "Capacity",
    "Bits",
    "Delimited suffix"
  };

  static size_t ParamCheck(const uint64_t* params)
  {
    for (size_t i = 0; i < 4; ++i)
      if (params[i] > std::numeric_limits<unsigned>::max())
        return 0;
    Keccak_HashInstance ctx{};
    const auto result = Keccak_HashInitialize(
      &ctx,
      (unsigned)params[0],
      (unsigned)params[1],
      (unsigned)params[2],
      (unsigned)params[3]
    );
    return result == KECCAK_SUCCESS ? (unsigned)params[2] / 8 : 0;
  }

  KeccakHashContext(const uint64_t* params)
  {
    Keccak_HashInitialize(
      &ctx,
      (unsigned)params[0],
      (unsigned)params[1],
      (unsigned)params[2],
      (unsigned)params[3]
    );
  }

  void Update(const void* data, size_t size)
  {
    Keccak_HashUpdate(&ctx, (const BitSequence*)data, size * 8);
  }

  void Finish(uint8_t* out)
  {
    Keccak_HashFinal(&ctx, (BitSequence*)out);
  }

  size_t GetOutputSize()
  {
    return ctx.fixedOutputLength / 8;
  }
};

class KangarooTwelveHashContext final : public HashContext
{
  KangarooTwelve_Instance ctx{};

public:
  constexpr static const char* k_params[] = {
    "Bits"
  };

  static size_t ParamCheck(const uint64_t* params)
  {
    if (params[0] % 8 != 0 || params[0] > std::numeric_limits<size_t>::max())
      return 0;
    KangarooTwelve_Instance ctx{};
    return KangarooTwelve_Initialize(&ctx, params[0]) == 0 ? (size_t)(params[0] / 8) : 0;
  }

  KangarooTwelveHashContext(const uint64_t* params)
  {
    KangarooTwelve_Initialize(&ctx, (size_t)(params[0] / 8));
  }

  void Update(const void* data, size_t size)
  {
    KangarooTwelve_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    KangarooTwelve_Final(&ctx, out, (const unsigned char*)"", 0);
  }

  size_t GetOutputSize()
  {
    return ctx.fixedOutputLength;
  }
};

class ParallelHash128HashContext final : public HashContext
{
  ParallelHash_Instance ctx{};

public:
  constexpr static const char* k_params[] = {
    "Block length",
    "Bits"
  };

  static size_t ParamCheck(const uint64_t* params)
  {
    if (params[0] > std::numeric_limits<size_t>::max() || params[1] > std::numeric_limits<size_t>::max())
      return 0;
    ParallelHash_Instance ctx{};
    const auto result = ParallelHash128_Initialize(&ctx, (size_t)params[0], (size_t)params[1], nullptr, 0);
    return result == 0 ? (size_t)(params[1] / 8) : 0;
  }

  ParallelHash128HashContext(const uint64_t* params)
  {
    ParallelHash128_Initialize(&ctx, (size_t)params[0], (size_t)params[1], nullptr, 0);
  }

  void Update(const void* data, size_t size)
  {
    ParallelHash128_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    ParallelHash128_Final(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return ctx.fixedOutputLength / 8;
  }
};

class ParallelHash256HashContext final : public HashContext
{
  ParallelHash_Instance ctx{};

public:
  constexpr static const char* k_params[] = {
    "Block length",
    "Bits"
  };

  static size_t ParamCheck(const uint64_t* params)
  {
    if (params[0] > std::numeric_limits<size_t>::max() || params[1] > std::numeric_limits<size_t>::max() || (size_t)params[1] % 8 != 0)
      return 0;
    ParallelHash_Instance ctx{};
    const auto result = ParallelHash256_Initialize(&ctx, (size_t)params[0], (size_t)params[1], nullptr, 0);
    return result == 0 ? (size_t)(params[1] / 8) : 0;
  }

  ParallelHash256HashContext(const uint64_t* params)
  {
    ParallelHash256_Initialize(&ctx, (size_t)params[0], (size_t)params[1], nullptr, 0);
  }

  void Update(const void* data, size_t size)
  {
    ParallelHash256_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    ParallelHash256_Final(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return ctx.fixedOutputLength / 8;
  }
};

template <unsigned Bits>
class GOST34112012HashContext final : public HashContext
{
  GOST34112012Context ctx{};

public:
  GOST34112012HashContext()
  {
    GOST34112012Init(&ctx, Bits);
  }

  void Update(const void* data, size_t size)
  {
    GOST34112012Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out)
  {
    GOST34112012Final(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return Bits / 8;
  }
};

using GOST34112012_256HashContext = GOST34112012HashContext<256>;
using GOST34112012_512HashContext = GOST34112012HashContext<512>;

class QuickXorHashContext final : public HashContext
{
  qxhash ctx{};

public:
  QuickXorHashContext()
  {
    qxhash_init(&ctx);
  }

  void Update(const void* data, size_t size)
  {
    qxhash_update(&ctx, (const uint8_t*)data, size);
  }

  void Finish(uint8_t* out)
  {
    qxhash_final(&ctx, out);
  }

  size_t GetOutputSize()
  {
    return QUICKXORHASH_SIZE;
  }
};

template <typename T, class = void>
class HashContextTraits
{
  static HashContext* ALGORITHMS_CC Factory(const uint64_t*)
  {
    return new T();
  }

  static size_t ALGORITHMS_CC ParamCheck(const uint64_t*)
  {
    return T{}.GetOutputSize();
  }

  static void ALGORITHMS_CC Update(HashContext* ctx, const void* data, size_t size)
  {
    ((T*)ctx)->Update(data, size);
  }

  static void ALGORITHMS_CC Finish(HashContext* ctx, uint8_t* out)
  {
    ((T*)ctx)->Finish(out);
  }

  static size_t ALGORITHMS_CC GetOutputSize(HashContext* ctx)
  {
    return ((T*)ctx)->GetOutputSize();
  }

  static void ALGORITHMS_CC Delete(HashContext* ctx)
  {
    delete ((T*)ctx);
  }
public:
  static constexpr auto param_check_fn = &ParamCheck;
  static constexpr auto factory_fn = &Factory;
  static constexpr auto update_fn = &Update;
  static constexpr auto finish_fn = &Finish;
  static constexpr auto get_output_size_fn = &GetOutputSize;
  static constexpr auto delete_fn = &Delete;
  static constexpr const char* const* params = nullptr;
  static constexpr size_t params_count = 0;
};

template <typename T>
class HashContextTraits<T, std::void_t<decltype(T::k_params)>>
{
  static HashContext* ALGORITHMS_CC Factory(const uint64_t* params)
  {
    return new T(params);
  }

  static size_t ALGORITHMS_CC ParamCheck(const uint64_t* params)
  {
    return T::ParamCheck(params);
  }

  static void ALGORITHMS_CC Update(HashContext* ctx, const void* data, size_t size)
  {
    ((T*)ctx)->Update(data, size);
  }

  static void ALGORITHMS_CC Finish(HashContext* ctx, uint8_t* out)
  {
    ((T*)ctx)->Finish(out);
  }

  static size_t ALGORITHMS_CC GetOutputSize(HashContext* ctx)
  {
    return ((T*)ctx)->GetOutputSize();
  }

  static void ALGORITHMS_CC Delete(HashContext* ctx)
  {
    delete ((T*)ctx);
  }
public:
  static constexpr auto param_check_fn = &ParamCheck;
  static constexpr auto factory_fn = &Factory;
  static constexpr auto update_fn = &Update;
  static constexpr auto finish_fn = &Finish;
  static constexpr auto get_output_size_fn = &GetOutputSize;
  static constexpr auto delete_fn = &Delete;
  static constexpr const char* const* params = T::k_params;
  static constexpr size_t params_count = std::size(T::k_params);
};

template <typename T>
constexpr HashAlgorithm make_algorithm(const char* name, bool is_secure)
{
  return HashAlgorithm{
    HashContextTraits<T>::param_check_fn,
    HashContextTraits<T>::factory_fn,
    HashContextTraits<T>::update_fn,
    HashContextTraits<T>::finish_fn,
    HashContextTraits<T>::get_output_size_fn,
    HashContextTraits<T>::delete_fn,
    name,
    is_secure,
    HashContextTraits<T>::params,
    HashContextTraits<T>::params_count
  };
}

constexpr HashAlgorithm k_algorithms[] = {
  make_algorithm<Crc32HashContext>("CRC32", false),
  make_algorithm<Crc64HashContext>("CRC64", false),
  make_algorithm<XXH32HashContext>("XXH32", false),
  make_algorithm<XXH64HashContext>("XXH64", false),
  make_algorithm<XXH3_64bitsHashContext>("XXH3-64", false),
  make_algorithm<XXH3_128bitsHashContext>("XXH3-128", false),
  make_algorithm<Md4HashContext>("MD4", false),
  make_algorithm<Md5HashContext>("MD5", false),
  make_algorithm<RipeMD160HashContext>("RipeMD160", true),
  make_algorithm<Sha1HashContext>("SHA-1", true),
  make_algorithm<Sha224HashContext>("SHA-224", true),
  make_algorithm<Sha256HashContext>("SHA-256", true),
  make_algorithm<Sha384HashContext>("SHA-384", true),
  make_algorithm<Sha512HashContext>("SHA-512", true),
  make_algorithm<Blake2SpHashContext>("BLAKE2sp", true),
  make_algorithm<KeccakHashContext>("Keccak", true),
  make_algorithm<KangarooTwelveHashContext>("K12", true),
  make_algorithm<ParallelHash128HashContext>("PH128", true),
  make_algorithm<ParallelHash256HashContext>("PH256", true),
  make_algorithm<Blake3HashContext>("BLAKE3", true),
  make_algorithm<GOST34112012_256HashContext>("GOST 2012 (256)", true),
  make_algorithm<GOST34112012_512HashContext>("GOST 2012 (512)", true),
  make_algorithm<ED2kHashContext<false>>("eD2k", false),
  make_algorithm<ED2kHashContext<true>>("eD2k (Old)", false),
  make_algorithm<QuickXorHashContext>("QuickXorHash", false),
};

constexpr const HashAlgorithm* k_algorithms_begin = std::begin(k_algorithms);
constexpr const HashAlgorithm* k_algorithms_end = std::end(k_algorithms);

extern "C" const HashAlgorithm* get_algorithms_begin() { return k_algorithms_begin; }
extern "C" const HashAlgorithm* get_algorithms_end() { return k_algorithms_end; }

/*
// these are what I found with a quick FTP search
static const char* const no_exts[] = { nullptr };
static const char* const md5_exts[] = { "md5", "md5sum", "md5sums", nullptr };
static const char* const ripemd160_exts[] = { "ripemd160", nullptr };
static const char* const sha1_exts[] = { "sha1", "sha1sum", "sha1sums", nullptr };
static const char* const sha224_exts[] = { "sha224", "sha224sum", nullptr };
static const char* const sha256_exts[] = { "sha256", "sha256sum", "sha256sums", nullptr };
static const char* const sha384_exts[] = { "sha384", nullptr };
static const char* const sha512_exts[] = { "sha512", "sha512sum", "sha512sums", nullptr };
static const char* const sha3_512_exts[] = { "sha3", "sha3-512",nullptr };

// i made these up so people don't complain about default filenames
static const char* const sha3_224_exts[] = { "sha3-224", nullptr };
static const char* const sha3_256_exts[] = { "sha3-256", nullptr };
static const char* const sha3_384_exts[] = { "sha3-384", nullptr };
static const char* const k12_264_exts[] = { "k12-264", nullptr };
static const char* const ph128_264_exts[] = { "ph128-264", nullptr };
static const char* const ph256_528_exts[] = { "ph256-528", nullptr };
static const char* const blake3_exts[] = { "blake3", nullptr };
static const char* const blake2sp_exts[] = { "blake2sp", nullptr };
static const char* const xxh32_exts[] = { "xxh32", nullptr };
static const char* const xxh64_exts[] = { "xxh64", nullptr };
static const char* const xxh3_64_exts[] = { "xxh3-64", nullptr };
static const char* const xxh3_128_exts[] = { "xxh3-128", nullptr };
static const char* const md4_exts[] = { "md4", nullptr };

constexpr HashAlgorithm HashAlgorithm::k_algorithms[] =
{
  { "CRC32", 4, no_exts, hash_context_factory<Crc32HashContext>, false },
  { "CRC64", 8, no_exts, hash_context_factory<Crc64HashContext>, false },
  { "XXH32", 4, xxh32_exts, hash_context_factory<XXH32HashContext>, false },
  { "XXH64", 8, xxh64_exts, hash_context_factory<XXH64HashContext>, false },
  { "XXH3-64", 8, xxh3_64_exts, hash_context_factory<XXH3_64bitsHashContext>, false },
  { "XXH3-128", 16, xxh3_128_exts, hash_context_factory<XXH3_128bitsHashContext>, false },
//  { "MD2", 16, no_exts, hash_context_factory<Md2HashContext>, false },
  { "MD4", 16, md4_exts, hash_context_factory<Md4HashContext>, false },
  { "MD5", 16, md5_exts, hash_context_factory<Md5HashContext>, false },
  { "RipeMD160", 20, ripemd160_exts, hash_context_factory<RipeMD160HashContext>, true },
  { "SHA-1", 20, sha1_exts, hash_context_factory<Sha1HashContext>, true },
  { "SHA-224", 28, sha224_exts, hash_context_factory<Sha224HashContext>, true },
  { "SHA-256", 32, sha256_exts, hash_context_factory<Sha256HashContext>, true },
  { "SHA-384", 48, sha384_exts, hash_context_factory<Sha384HashContext>, true },
  { "SHA-512", 64, sha512_exts, hash_context_factory<Sha512HashContext>, true },
  { "Blake2sp", 32, blake2sp_exts, hash_context_factory<Blake2SpHashContext>, true },
  { "SHA3-224", 28, sha3_224_exts, hash_context_factory<SHA3_224HashContext>, true },
  { "SHA3-256", 32, sha3_256_exts, hash_context_factory<SHA3_256HashContext>, true },
  { "SHA3-384", 48, sha3_384_exts, hash_context_factory<SHA3_384HashContext>, true },
  { "SHA3-512", 64, sha3_512_exts, hash_context_factory<SHA3_512HashContext>, true },
  { "K12-264", 33, k12_264_exts, hash_context_factory<K12_264HashContext>, true },
  { "K12-256", 32, no_exts, hash_context_factory<K12_256HashContext>, true },
  { "K12-512", 64, no_exts, hash_context_factory<K12_512HashContext>, true },
  { "PH128-264", 33, ph128_264_exts, hash_context_factory<PH128_264HashContext>, true },
  { "PH256-528", 66, ph256_528_exts, hash_context_factory<PH256_528HashContext>, true },
  { "BLAKE3", 32, blake3_exts, hash_context_factory<Blake3_256HashContext>, true },
  { "BLAKE3-512", 64, no_exts, hash_context_factory<Blake3_512HashContext>, true },
  { "GOST 2012 (256)", 32, no_exts, hash_context_factory<GOST34112012_256HashContext>, true },
  { "GOST 2012 (512)", 64, no_exts, hash_context_factory<GOST34112012_512HashContext>, true },
  { "eD2k", 16, no_exts, hash_context_factory<ED2kHashContext<false>>, false },
  { "eD2k (Old)", 16, no_exts, hash_context_factory<ED2kHashContext<true>>, false },
};*/

/*
extern "C" __declspec(dllexport) const HashAlgorithm* Algorithms()
{
  return HashAlgorithm::Algorithms();
}
*/
