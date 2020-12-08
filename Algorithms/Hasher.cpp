//    Copyright 2019-2020 namazso <admin@namazso.eu>
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
#include "Hasher.h"

#include <mbedtls/md.h>
#include <mbedtls/md2.h>
#include <mbedtls/md4.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/ripemd160.h>
#include "blake2sp.h"
#include "sha3.h"
#include "crc32.h"
#include "blake3.h"

#define XXH_STATIC_LINKING_ONLY

#include "../xxHash/xxhash.h"


template <typename T> HashContext* hash_context_factory(const HashAlgorithm* algorithm) { return new T(algorithm); }


template <
  typename Ctx,
  size_t Size,
  void (*Init)(Ctx* ctx),
  int (*StartsRet)(Ctx* ctx),
  void (*Free)(Ctx* ctx),
  int (*UpdateRet)(Ctx* ctx, const unsigned char*, size_t),
  int (*FinishRet)(Ctx* ctx, unsigned char*)
>
class MbedHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  Ctx ctx{};

public:
  MbedHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    Init(&ctx);
    StartsRet(&ctx);
  }
  ~MbedHashContext()
  {
    Free(&ctx);
  }

  void Clear() override
  {
    Free(&ctx);
    Init(&ctx);
    StartsRet(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    UpdateRet(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    FinishRet(&ctx, out);
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

class Blake2SpHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  CBlake2sp ctx{};

public:
  Blake2SpHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    Blake2sp_Init(&ctx);
  }
  ~Blake2SpHashContext() = default;

  void Clear() override
  {
    Blake2sp_Init(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    Blake2sp_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    Blake2sp_Final(&ctx, out);
  }
};

template <void (*Init)(void*), size_t Size>
class Sha3HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  sha3_context ctx{};

public:
  Sha3HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    Init(&ctx);
  }
  ~Sha3HashContext() = default;

  void Clear() override
  {
    Init(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    sha3_Update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto begin = (const uint8_t*)sha3_Finalize(&ctx);
    memcpy(out, begin, Size);
  }
};

using Sha3_256HashContext = Sha3HashContext<&sha3_Init256, 32>;
using Sha3_384HashContext = Sha3HashContext<&sha3_Init384, 48>;
using Sha3_512HashContext = Sha3HashContext<&sha3_Init512, 64>;


class Crc32HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  uint32_t crc{};

public:
  Crc32HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm) {}
  ~Crc32HashContext() = default;

  void Clear() override
  {
    crc = 0;
  }

  void Update(const void* data, size_t size) override
  {
    crc = Crc32_ComputeBuf(crc, data, size);
  }

  void Finish(uint8_t* out) override
  {
    out[0] = 0xFF & (crc >> 24);
    out[1] = 0xFF & (crc >> 16);
    out[2] = 0xFF & (crc >> 8);
    out[3] = 0xFF & (crc >> 0);
  }
};

class Blake3HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  blake3_hasher ctx{};

public:
  Blake3HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    blake3_hasher_init(&ctx);
  }
  ~Blake3HashContext() = default;

  void Clear() override
  {
    blake3_hasher_init(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    blake3_hasher_update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    blake3_hasher_finalize(&ctx, out, BLAKE3_OUT_LEN);
  }
};



#define SSE2_CPUID_MASK (1 << 26)
#define OSXSAVE_CPUID_MASK ((1 << 26) | (1 << 27))
#define AVX2_CPUID_MASK (1 << 5)
#define AVX2_XGETBV_MASK ((1 << 2) | (1 << 1))
#define AVX512F_CPUID_MASK (1 << 16)
#define AVX512F_XGETBV_MASK ((7 << 5) | (1 << 2) | (1 << 1))

enum CPUFeatureLevel
{
  CPU_None,
  CPU_SSE2,
  CPU_AVX2,
  CPU_AVX512,
  CPU_NEON,

  CPU_MAX
};

/* Returns the best XXH3 implementation */
static CPUFeatureLevel get_cpu_level()
{
  auto best = CPU_None;
#if defined(_M_IX86) || defined(_M_X64)
  int abcd[4];
  // Check how many CPUID pages we have
  __cpuidex(abcd, 0, 0);
  const auto max_leaves = abcd[0];

  // Shouldn't happen on hardware, but happens on some QEMU configs.
  if (max_leaves == 0)
    return best;

  // Check for SSE2, OSXSAVE and xgetbv
  __cpuidex(abcd, 1, 0);

  // Test for SSE2. The check is redundant on x86_64, but it doesn't hurt.
  if ((abcd[3] & SSE2_CPUID_MASK) != SSE2_CPUID_MASK)
    return best;

  best = CPU_SSE2;
  // Make sure we have enough leaves
  if (max_leaves < 7)
    return best;

  // Test for OSXSAVE and XGETBV
  if ((abcd[2] & OSXSAVE_CPUID_MASK) != OSXSAVE_CPUID_MASK)
    return best;

  // CPUID check for AVX features
  __cpuidex(abcd, 7, 0);

  const auto xgetbv_val = _xgetbv(0);
  // Validate that AVX2 is supported by the CPU
  if ((abcd[1] & AVX2_CPUID_MASK) != AVX2_CPUID_MASK)
    return best;

  // Validate that the OS supports YMM registers
  if ((xgetbv_val & AVX2_XGETBV_MASK) != AVX2_XGETBV_MASK)
    return best;

  // AVX2 supported
  best = CPU_AVX2;

  // Check if AVX512F is supported by the CPU
  if ((abcd[1] & AVX512F_CPUID_MASK) != AVX512F_CPUID_MASK)
    return best;

  // Validate that the OS supports ZMM registers
  if ((xgetbv_val & AVX512F_XGETBV_MASK) != AVX512F_XGETBV_MASK)
    return best;

  // AVX512F supported
  best = CPU_AVX512;
#endif
#ifdef _M_ARM64
  best = CPU_NEON;
#endif
  return best;
}


#define DEFINE_XXH_FOR_EXT_NAMESPACE(n) \
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH32_reset(XXH32_state_t* statePtr, XXH32_hash_t seed);\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH32_update(XXH32_state_t* statePtr, const void* input, size_t length);\
XXH_PUBLIC_API extern "C" XXH32_hash_t  n ## _XXH32_digest(const XXH32_state_t* statePtr);\
\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH64_reset(XXH64_state_t* statePtr, XXH64_hash_t seed);\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH64_update(XXH64_state_t* statePtr, const void* input, size_t length);\
XXH_PUBLIC_API extern "C" XXH64_hash_t  n ## _XXH64_digest(const XXH64_state_t* statePtr);\
\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH3_64bits_reset(XXH3_state_t* statePtr);\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH3_64bits_update(XXH3_state_t* statePtr, const void* input, size_t length);\
XXH_PUBLIC_API extern "C" XXH64_hash_t  n ## _XXH3_64bits_digest(const XXH3_state_t* statePtr);\
\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH3_128bits_reset(XXH3_state_t* statePtr);\
XXH_PUBLIC_API extern "C" XXH_errorcode n ## _XXH3_128bits_update(XXH3_state_t* statePtr, const void* input, size_t length);\
XXH_PUBLIC_API extern "C" XXH128_hash_t n ## _XXH3_128bits_digest(const XXH3_state_t* statePtr);\

DEFINE_XXH_FOR_EXT_NAMESPACE(scalar);
DEFINE_XXH_FOR_EXT_NAMESPACE(sse2);
DEFINE_XXH_FOR_EXT_NAMESPACE(avx2);
DEFINE_XXH_FOR_EXT_NAMESPACE(avx512);
DEFINE_XXH_FOR_EXT_NAMESPACE(neon);

template <decltype(&XXH32_reset) reset, decltype(&XXH32_update) update, decltype(&XXH32_digest) digest>
class XXH32HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH32_state_t ctx{};

public:
  XXH32HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    reset(&ctx, 0);
  }
  ~XXH32HashContext() = default;

  void Clear() override
  {
    reset(&ctx, 0);
  }

  void Update(const void* data, size_t size) override
  {
    update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto xxh32 = digest(&ctx);
    out[0] = 0xFF & (xxh32 >> 24);
    out[1] = 0xFF & (xxh32 >> 16);
    out[2] = 0xFF & (xxh32 >> 8);
    out[3] = 0xFF & (xxh32 >> 0);
  }
};

template <decltype(&XXH64_reset) reset, decltype(&XXH64_update) update, decltype(&XXH64_digest) digest>
class XXH64HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH64_state_t ctx{};

public:
  XXH64HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    reset(&ctx, 0);
  }
  ~XXH64HashContext() = default;

  void Clear() override
  {
    reset(&ctx, 0);
  }

  void Update(const void* data, size_t size) override
  {
    update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto xxh64 = digest(&ctx);
    out[0] = 0xFF & (xxh64 >> 56);
    out[1] = 0xFF & (xxh64 >> 48);
    out[2] = 0xFF & (xxh64 >> 40);
    out[3] = 0xFF & (xxh64 >> 32);
    out[4] = 0xFF & (xxh64 >> 24);
    out[5] = 0xFF & (xxh64 >> 16);
    out[6] = 0xFF & (xxh64 >> 8);
    out[7] = 0xFF & (xxh64 >> 0);
  }
};

template <decltype(&XXH3_64bits_reset) reset, decltype(&XXH3_64bits_update) update, decltype(&XXH3_64bits_digest) digest>
class XXH3_64bitsHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH3_state_t ctx{};

public:
  XXH3_64bitsHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    reset(&ctx);
  }
  ~XXH3_64bitsHashContext() = default;

  void Clear() override
  {
    reset(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto xxh64 = digest(&ctx);
    out[0] = 0xFF & (xxh64 >> 56);
    out[1] = 0xFF & (xxh64 >> 48);
    out[2] = 0xFF & (xxh64 >> 40);
    out[3] = 0xFF & (xxh64 >> 32);
    out[4] = 0xFF & (xxh64 >> 24);
    out[5] = 0xFF & (xxh64 >> 16);
    out[6] = 0xFF & (xxh64 >> 8);
    out[7] = 0xFF & (xxh64 >> 0);
  }
};

template <decltype(&XXH3_128bits_reset) reset, decltype(&XXH3_128bits_update) update, decltype(&XXH3_128bits_digest) digest>
class XXH3_128bitsHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH3_state_t ctx{};

public:
  XXH3_128bitsHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    reset(&ctx);
  }
  ~XXH3_128bitsHashContext() = default;

  void Clear() override
  {
    reset(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto xxh128 = digest(&ctx);
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
};

#define XXH_HashContext(cpu, algo) algo ## HashContext<cpu ## _ ## algo ## _reset, cpu ## _ ## algo ## _update, cpu ## _ ## algo ## _digest>
#define XXH_HashContextAll(cpu) \
  hash_context_factory<XXH_HashContext(cpu, XXH32)>,\
  hash_context_factory<XXH_HashContext(cpu, XXH64)>,\
  hash_context_factory<XXH_HashContext(cpu, XXH3_64bits)>,\
  hash_context_factory<XXH_HashContext(cpu, XXH3_128bits)>

static constexpr HashAlgorithm::FactoryFn* xxh_factories[CPU_MAX][4] = {
#if !(defined(_M_X64) || defined(_M_ARM64))
  { XXH_HashContextAll(scalar) },   // None
#else
  { nullptr, nullptr, nullptr, nullptr },
#endif
#if defined(_M_IX86) || defined(_M_X64)
  { XXH_HashContextAll(sse2) },   // SSE2
  { XXH_HashContextAll(avx2) },   // AVX2
  { XXH_HashContextAll(avx512) }, // AVX512
#else
  { nullptr, nullptr, nullptr, nullptr },
  { nullptr, nullptr, nullptr, nullptr },
  { nullptr, nullptr, nullptr, nullptr },
#endif
#ifdef _M_ARM64
  { XXH_HashContextAll(neon) },   // NEON
#else
  { nullptr, nullptr, nullptr, nullptr },
#endif
};

template <size_t Idx>
HashContext* xxh_context_factory(const HashAlgorithm* algorithm)
{
  static HashAlgorithm::FactoryFn* fn = nullptr;
  if (!fn)
    fn = xxh_factories[get_cpu_level()][Idx];
  return fn(algorithm);
}

// these are what I found with a quick FTP search
static const char* const no_exts[] = { nullptr };
static const char* const md5_exts[] = { "md5", "md5sum", "md5sums", nullptr };
static const char* const ripemd160_exts[] = { "ripemd160", nullptr };
static const char* const sha1_exts[] = { "sha1", "sha1sum", "sha1sums", nullptr };
static const char* const sha224_exts[] = { "sha224", "sha224sum", nullptr };
static const char* const sha256_exts[] = { "sha256", "sha256sum", "sha256sums", nullptr };
static const char* const sha384_exts[] = { "sha384", nullptr };
static const char* const sha512_exts[] = { "sha512", "sha512sum", "sha512sums", nullptr };
static const char* const sha3_256_exts[] = { "sha3-256", nullptr };
static const char* const sha3_384_exts[] = { "sha3-384", nullptr };
static const char* const sha3_512_exts[] = { "sha3", "sha3-512",nullptr };

constexpr HashAlgorithm HashAlgorithm::g_hashers[] =
{
  { "CRC32", 4, no_exts, hash_context_factory<Crc32HashContext>, false },
  { "XXH32", 4, no_exts, xxh_context_factory<0>, false },
  { "XXH64", 8, no_exts, xxh_context_factory<1>, false },
  { "XXH3-64", 8, no_exts, xxh_context_factory<2>, false },
  { "XXH3-128", 16, no_exts, xxh_context_factory<3>, false },
  { "MD2", 16, no_exts, hash_context_factory<Md2HashContext>, false },
  { "MD4", 16, no_exts, hash_context_factory<Md4HashContext>, false },
  { "MD5", 16, md5_exts, hash_context_factory<Md5HashContext>, false },
  { "RipeMD160", 20, ripemd160_exts, hash_context_factory<RipeMD160HashContext>, true },
  { "SHA-1", 20, sha1_exts, hash_context_factory<Sha1HashContext>, true },
  { "SHA-224", 28, sha224_exts, hash_context_factory<Sha224HashContext>, true },
  { "SHA-256", 32, sha256_exts, hash_context_factory<Sha256HashContext>, true },
  { "SHA-384", 48, sha384_exts, hash_context_factory<Sha384HashContext>, true },
  { "SHA-512", 64, sha512_exts, hash_context_factory<Sha512HashContext>, true },
  { "Blake2sp", 32, no_exts, hash_context_factory<Blake2SpHashContext>, true },
  { "SHA3-256", 32, sha3_256_exts, hash_context_factory<Sha3_256HashContext>, true },
  { "SHA3-384", 48, sha3_384_exts, hash_context_factory<Sha3_384HashContext>, true },
  { "SHA3-512", 64, sha3_512_exts, hash_context_factory<Sha3_512HashContext>, true },
  { "BLAKE3", 32, no_exts, hash_context_factory<Blake3HashContext>, true },
};
