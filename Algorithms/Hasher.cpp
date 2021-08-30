//    Copyright 2019-2021 namazso <admin@namazso.eu>
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
#include "deps/crc32/Crc32.h"
#include "deps/BLAKE3/c/blake3.h"
extern "C" {
#include "KeccakHash.h"
#include "KangarooTwelve.h"
#include "SP800-185.h"
}
#include "crc64.h"

#define XXH_STATIC_LINKING_ONLY

#include "deps/xxHash/xxhash.h"

extern "C" {
#define uint512_u uint512_u_STREEBOG
#include "deps/streebog/gost3411-2012-core.h"
#undef uint512_u
}

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
    crc = crc32_fast(data, size, crc);
  }

  void Finish(uint8_t* out) override
  {
    out[0] = 0xFF & (crc >> 24);
    out[1] = 0xFF & (crc >> 16);
    out[2] = 0xFF & (crc >> 8);
    out[3] = 0xFF & (crc >> 0);
  }
};

class Crc64HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  uint64_t crc{};

public:
  Crc64HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm) {}
  ~Crc64HashContext() = default;

  void Clear() override
  {
    crc = 0;
  }

  void Update(const void* data, size_t size) override
  {
    crc = crc64(crc, data, size);
  }

  void Finish(uint8_t* out) override
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
};

template <unsigned HashBitlen>
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
    blake3_hasher_finalize(&ctx, out, HashBitlen / 8);
  }
};

using Blake3_256HashContext = Blake3HashContext<256>;
using Blake3_512HashContext = Blake3HashContext<512>;

class XXH32HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH32_state_t ctx{};

public:
  XXH32HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    XXH32_reset(&ctx, 0);
  }
  ~XXH32HashContext() = default;

  void Clear() override
  {
    XXH32_reset(&ctx, 0);
  }

  void Update(const void* data, size_t size) override
  {
    XXH32_update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
  {
    const auto xxh32 = XXH32_digest(&ctx);
    out[0] = 0xFF & (xxh32 >> 24);
    out[1] = 0xFF & (xxh32 >> 16);
    out[2] = 0xFF & (xxh32 >> 8);
    out[3] = 0xFF & (xxh32 >> 0);
  }
};

class XXH64HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH64_state_t ctx{};

public:
  XXH64HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    XXH64_reset(&ctx, 0);
  }
  ~XXH64HashContext() = default;

  void Clear() override
  {
    XXH64_reset(&ctx, 0);
  }

  void Update(const void* data, size_t size) override
  {
    XXH64_update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
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
};

class XXH3_64bitsHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH3_state_t ctx{};

public:
  XXH3_64bitsHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    XXH3_64bits_reset(&ctx);
  }
  ~XXH3_64bitsHashContext() = default;

  void Clear() override
  {
    XXH3_64bits_reset(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    XXH3_64bits_update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
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
};

class XXH3_128bitsHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  XXH3_state_t ctx{};

public:
  XXH3_128bitsHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    XXH3_128bits_reset(&ctx);
  }
  ~XXH3_128bitsHashContext() = default;

  void Clear() override
  {
    XXH3_128bits_reset(&ctx);
  }

  void Update(const void* data, size_t size) override
  {
    XXH3_128bits_update(&ctx, data, size);
  }

  void Finish(uint8_t* out) override
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
};

template <unsigned Rate, unsigned Capacity, unsigned HashBitlen, unsigned char DelimitedSuffix>
class KeccakHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  Keccak_HashInstance ctx{};

public:
  KeccakHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    Keccak_HashInitialize(&ctx, Rate, Capacity, HashBitlen, DelimitedSuffix);
  }
  ~KeccakHashContext() = default;

  void Clear() override
  {
    Keccak_HashInitialize(&ctx, Rate, Capacity, HashBitlen, DelimitedSuffix);
  }

  void Update(const void* data, size_t size) override
  {
    Keccak_HashUpdate(&ctx, (const BitSequence*)data, size * 8);
  }

  void Finish(uint8_t* out) override
  {
    Keccak_HashFinal(&ctx, (BitSequence*)out);
  }
};

//using SHAKE128HashContext = KeccakHashContext<1344,  256,   0, 0x1F>;
//using SHAKE256HashContext = KeccakHashContext<1088,  512,   0, 0x1F>;
using SHA3_224HashContext = KeccakHashContext<1152,  448, 224, 0x06>;
using SHA3_256HashContext = KeccakHashContext<1088,  512, 256, 0x06>;
using SHA3_384HashContext = KeccakHashContext< 832,  768, 384, 0x06>;
using SHA3_512HashContext = KeccakHashContext< 576, 1024, 512, 0x06>;

template <unsigned HashBitlen>
class KangarooTwelveHashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  KangarooTwelve_Instance ctx{};

public:
  KangarooTwelveHashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    KangarooTwelve_Initialize(&ctx, HashBitlen / 8);
  }
  ~KangarooTwelveHashContext() = default;

  void Clear() override
  {
    KangarooTwelve_Initialize(&ctx, HashBitlen / 8);
  }

  void Update(const void* data, size_t size) override
  {
    KangarooTwelve_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    KangarooTwelve_Final(&ctx, out, (const unsigned char*)"", 0);
  }
};

using K12_264HashContext = KangarooTwelveHashContext<264>;
using K12_256HashContext = KangarooTwelveHashContext<256>;
using K12_512HashContext = KangarooTwelveHashContext<512>;

template <unsigned BlockByteLen, unsigned HashBitlen>
class ParallelHash128HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  ParallelHash_Instance ctx{};

public:
  ParallelHash128HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    ParallelHash128_Initialize(&ctx, BlockByteLen, HashBitlen, (const unsigned char*)"", 0);
  }
  ~ParallelHash128HashContext() = default;

  void Clear() override
  {
    ParallelHash128_Initialize(&ctx, BlockByteLen, HashBitlen, (const unsigned char*)"", 0);
  }

  void Update(const void* data, size_t size) override
  {
    ParallelHash128_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    ParallelHash128_Final(&ctx, out);
  }
};

using PH128_264HashContext = ParallelHash128HashContext<8192, 264>;

template <unsigned BlockByteLen, unsigned HashBitlen>
class ParallelHash256HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  ParallelHash_Instance ctx{};

public:
  ParallelHash256HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    ParallelHash256_Initialize(&ctx, BlockByteLen, HashBitlen, (const unsigned char*)"", 0);
  }
  ~ParallelHash256HashContext() = default;

  void Clear() override
  {
    ParallelHash256_Initialize(&ctx, BlockByteLen, HashBitlen, (const unsigned char*)"", 0);
  }

  void Update(const void* data, size_t size) override
  {
    ParallelHash256_Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    ParallelHash256_Final(&ctx, out);
  }
};

using PH256_528HashContext = ParallelHash256HashContext<8192, 528>;


template <unsigned HashBitlen>
class GOST34112012HashContext : HashContext
{
  template <typename T> friend HashContext* hash_context_factory(const HashAlgorithm* algorithm);

  GOST34112012Context ctx{};

public:
  GOST34112012HashContext(const HashAlgorithm* algorithm) : HashContext(algorithm)
  {
    GOST34112012Init(&ctx, HashBitlen);
  }
  ~GOST34112012HashContext() = default;

  void Clear() override
  {
    GOST34112012Init(&ctx, HashBitlen);
  }

  void Update(const void* data, size_t size) override
  {
    GOST34112012Update(&ctx, (const unsigned char*)data, size);
  }

  void Finish(uint8_t* out) override
  {
    GOST34112012Final(&ctx, out);
  }
};

using GOST34112012_256HashContext = GOST34112012HashContext<256>;
using GOST34112012_512HashContext = GOST34112012HashContext<512>;

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
};

extern "C" __declspec(dllexport) const HashAlgorithm* Algorithms()
{
  return HashAlgorithm::Algorithms();
}