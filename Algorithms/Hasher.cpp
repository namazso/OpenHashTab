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

  std::vector<uint8_t> Finish() override
  {
    std::vector<uint8_t> result;
    // mbedTLS says this should be 32 for SHA224 so we can't just use Size
    result.resize(MBEDTLS_MD_MAX_SIZE);
    FinishRet(&ctx, result.data());
    result.resize(Size);
    return result;
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

  std::vector<uint8_t> Finish() override
  {
    std::vector<uint8_t> result;
    result.resize(BLAKE2S_DIGEST_SIZE);
    Blake2sp_Final(&ctx, result.data());
    return result;
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

  std::vector<uint8_t> Finish() override
  {
    const auto begin = (const uint8_t*)sha3_Finalize(&ctx);
    std::vector<uint8_t> result{ begin, begin + Size };
    return result;
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

  std::vector<uint8_t> Finish() override
  {
    std::vector<uint8_t> result;
    result.resize(4);
    result[0] = 0xFF & (crc >> 24);
    result[1] = 0xFF & (crc >> 16);
    result[2] = 0xFF & (crc >> 8);
    result[3] = 0xFF & (crc >> 0);
    return result;
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

  std::vector<uint8_t> Finish() override
  {
    std::vector<uint8_t> result;
    result.resize(BLAKE3_OUT_LEN);
    blake3_hasher_finalize(&ctx, result.data(), BLAKE3_OUT_LEN);
    return result;
  }
};

template <typename T> HashContext* hash_context_factory(const HashAlgorithm* algorithm) { return new T(algorithm); }

// these are what I found with a quick FTP search
static const char* const no_exts[] = { nullptr };
static const char* const md5_exts[] = { "md5", "md5sum", "md5sums", nullptr };
static const char* const ripemd160_exts[] = { "ripemd160", nullptr };
static const char* const sha1_exts[] = { "sha1", "sha1sum", "sha1sums", nullptr };
static const char* const sha224_exts[] = { "sha224", "sha224sum", nullptr };
static const char* const sha256_exts[] = { "sha256", "sha256sum", "sha256sums", nullptr };
static const char* const sha384_exts[] = { "sha384", nullptr };
static const char* const sha512_exts[] = { "sha512", "sha512sum", "sha512sums", nullptr };
static const char* const sha3_512_exts[] = { "sha3", nullptr };

constexpr HashAlgorithm HashAlgorithm::g_hashers[] =
{
  { "CRC32", 4, no_exts, hash_context_factory<Crc32HashContext>, false },
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
  { "SHA3-256", 32, no_exts, hash_context_factory<Sha3_256HashContext>, true },
  { "SHA3-384", 48, no_exts, hash_context_factory<Sha3_384HashContext>, true },
  { "SHA3-512", 64, sha3_512_exts, hash_context_factory<Sha3_512HashContext>, true },
  { "BLAKE3", 32, no_exts, hash_context_factory<Blake3HashContext>, true },
};