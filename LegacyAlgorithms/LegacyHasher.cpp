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
#define _ENABLE_EXTENDED_ALIGNED_STORAGE

#include <cassert>

#include "Hasher.h"

#include <Windows.h>
#if defined(_M_IX86) || defined(_M_X64)
#include <immintrin.h>
#endif

enum : uint32_t {
  SSE2_CPUID_MASK = 1u << 26,
  OSXSAVE_CPUID_MASK = (1u << 26) | (1u << 27),
  AVX_CPUID_MASK = 1u << 28,
  AVX2_CPUID_MASK = 1u << 5,
  AVX512F_CPUID_MASK = 1u << 16,
  AVX512VL_CPUID_MASK = 1u << 31,

  AVX_XGETBV_MASK = (1u << 2) | (1u << 1),
  AVX512_XGETBV_MASK = (7u << 5) | (1u << 2) | (1u << 1)
};

enum CPUFeatureLevel {
  CPU_None,
  CPU_SSE2,
  CPU_AVX,
  CPU_AVX2,
  CPU_AVX512,
  CPU_NEON,

  CPU_MAX
};

// Returns the algorithms dll implementation to use
static CPUFeatureLevel get_cpu_level() {
  auto best = CPU_None;
#if defined(_M_IX86)
  best = CPU_SSE2;
#elif defined(_M_X64)
  int abcdi[4];
  // Check how many CPUID pages we have
  __cpuidex(abcdi, 0, 0);
  const auto max_leaves = abcdi[0];

  // Shouldn't happen on hardware, but happens on some QEMU configs.
  if (max_leaves == 0)
    return best;

  // Check for SSE2, OSXSAVE and xgetbv
  __cpuidex(abcdi, 1, 0);

  uint32_t abcd[4];
  abcd[0] = (uint32_t)abcdi[0];
  abcd[1] = (uint32_t)abcdi[1];
  abcd[2] = (uint32_t)abcdi[2];
  abcd[3] = (uint32_t)abcdi[3];

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

  const auto xgetbv_val = _xgetbv(0);

  // Validate that the OS supports YMM registers
  if ((xgetbv_val & AVX_XGETBV_MASK) != AVX_XGETBV_MASK)
    return best;

  // Validate that AVX is supported by the CPU
  if ((abcd[2] & AVX_CPUID_MASK) != AVX_CPUID_MASK)
    return best;

  // AVX supported
  best = CPU_AVX;

  // CPUID check for AVX2 features
  __cpuidex(abcdi, 7, 0);
  abcd[0] = (uint32_t)abcdi[0];
  abcd[1] = (uint32_t)abcdi[1];
  abcd[2] = (uint32_t)abcdi[2];
  abcd[3] = (uint32_t)abcdi[3];

  // Validate that AVX2 is supported by the CPU
  if ((abcd[1] & AVX2_CPUID_MASK) != AVX2_CPUID_MASK)
    return best;

  // AVX2 supported
  best = CPU_AVX2;

  // Check if AVX512F is supported by the CPU
  if ((abcd[1] & AVX512F_CPUID_MASK) != AVX512F_CPUID_MASK)
    return best;

  // Check if AVX512VL is supported by the CPU
  if ((abcd[1] & AVX512VL_CPUID_MASK) != AVX512VL_CPUID_MASK)
    return best;

  // Validate that the OS supports ZMM registers
  if ((xgetbv_val & AVX512_XGETBV_MASK) != AVX512_XGETBV_MASK)
    return best;

  // AVX512 supported
  best = CPU_AVX512;
#elif defined(_M_ARM64)
  best = CPU_NEON;
#endif
  return best;
}

extern "C" const HashAlgorithm* get_algorithms_begin_SSE2();
extern "C" const HashAlgorithm* get_algorithms_end_SSE2();
extern "C" const HashAlgorithm* get_algorithms_begin_AVX();
extern "C" const HashAlgorithm* get_algorithms_end_AVX();
extern "C" const HashAlgorithm* get_algorithms_begin_AVX2();
extern "C" const HashAlgorithm* get_algorithms_end_AVX2();
extern "C" const HashAlgorithm* get_algorithms_begin_AVX512();
extern "C" const HashAlgorithm* get_algorithms_end_AVX512();
extern "C" const HashAlgorithm* get_algorithms_begin_ARM64();
extern "C" const HashAlgorithm* get_algorithms_end_ARM64();

#if defined(_M_X64)
static const HashAlgorithm* get_algorithms_begin(CPUFeatureLevel level) {
  switch (level) {
  default:
    return nullptr;
  case CPU_SSE2:
    return get_algorithms_begin_SSE2();
  case CPU_AVX:
    return get_algorithms_begin_AVX();
  case CPU_AVX2:
    return get_algorithms_begin_AVX2();
  case CPU_AVX512:
    return get_algorithms_begin_AVX512();
  }
}

static const HashAlgorithm* get_algorithms_end(CPUFeatureLevel level) {
  switch (level) {
  case CPU_None:
  case CPU_NEON:
  case CPU_MAX:
  default:
    return nullptr;
  case CPU_SSE2:
    return get_algorithms_end_SSE2();
  case CPU_AVX:
    return get_algorithms_end_AVX();
  case CPU_AVX2:
    return get_algorithms_end_AVX2();
  case CPU_AVX512:
    return get_algorithms_end_AVX512();
  }
}
#elif defined(_M_ARM64)
const HashAlgorithm* get_algorithms_begin(CPUFeatureLevel level) {
  switch (level) {
  case CPU_None:
  case CPU_SSE2:
  case CPU_AVX:
  case CPU_AVX2:
  case CPU_AVX512:
  case CPU_MAX:
  default:
    return nullptr;
  case CPU_NEON:
    return get_algorithms_begin_ARM64();
  }
}

const HashAlgorithm* get_algorithms_end(CPUFeatureLevel level) {
  switch (level) {
  default:
    return nullptr;
  case CPU_NEON:
    return get_algorithms_end_ARM64();
  }
}
#else
#error "Unsupported architecture"
#endif

struct AlgorithmsDll {
  const HashAlgorithm* algorithms_begin{};
  const HashAlgorithm* algorithms_end{};

  AlgorithmsDll() {
    const auto level = get_cpu_level();
    algorithms_begin = get_algorithms_begin(level);
    algorithms_end = get_algorithms_end(level);
  }

  ~AlgorithmsDll() = default;
};

const AlgorithmsDll& get_algorithms_dll() {
  static AlgorithmsDll algorithms_dll;
  return algorithms_dll;
}

LegacyHashAlgorithm::LegacyHashAlgorithm(
  const char* name,
  size_t expected_size,
  const char* const* extensions,
  const char* alg_name,
  const uint64_t* params
)
    : _name(name)
    , _extensions(extensions)
    , _params(params) {
  auto& dll = get_algorithms_dll();
  for (auto it = dll.algorithms_begin; it != dll.algorithms_end; ++it) {
    if (0 == strcmp(alg_name, it->name)) {
      _algorithm = it;
      _size = (uint32_t)it->ParamCheck(params);
      assert(_size);
      assert(_size == expected_size);
      _is_secure = it->is_secure;

      break;
    }
  }
}

template <uint64_t... Params>
static constexpr uint64_t as_param[] = {Params...};

LegacyHashAlgorithm::AlgorithmsType& LegacyHashAlgorithm::Algorithms() {
  // these are what I found with a quick FTP search
  static const char* const no_exts[] = {nullptr};
  static const char* const md5_exts[] = {"md5", "md5sum", "md5sums", nullptr};
  static const char* const ripemd160_exts[] = {"ripemd160", nullptr};
  static const char* const sha1_exts[] = {"sha1", "sha1sum", "sha1sums", nullptr};
  static const char* const sha224_exts[] = {"sha224", "sha224sum", nullptr};
  static const char* const sha256_exts[] = {"sha256", "sha256sum", "sha256sums", nullptr};
  static const char* const sha384_exts[] = {"sha384", nullptr};
  static const char* const sha512_exts[] = {"sha512", "sha512sum", "sha512sums", nullptr};
  static const char* const sha3_512_exts[] = {"sha3", "sha3-512", nullptr};

  // i made these up so people don't complain about default filenames
  static const char* const sha3_224_exts[] = {"sha3-224", nullptr};
  static const char* const sha3_256_exts[] = {"sha3-256", nullptr};
  static const char* const sha3_384_exts[] = {"sha3-384", nullptr};
  static const char* const k12_264_exts[] = {"k12-264", nullptr};
  static const char* const ph128_264_exts[] = {"ph128-264", nullptr};
  static const char* const ph256_528_exts[] = {"ph256-528", nullptr};
  static const char* const blake3_exts[] = {"blake3", nullptr};
  static const char* const blake2sp_exts[] = {"blake2sp", nullptr};
  static const char* const xxh32_exts[] = {"xxh32", nullptr};
  static const char* const xxh64_exts[] = {"xxh64", nullptr};
  static const char* const xxh3_64_exts[] = {"xxh3-64", nullptr};
  static const char* const xxh3_128_exts[] = {"xxh3-128", nullptr};
  static const char* const md4_exts[] = {"md4", nullptr};

  static LegacyHashAlgorithm algorithms[] =
    {
      {"CRC32", 4, no_exts, "CRC32"},
      {"CRC64", 8, no_exts, "CRC64"},
      {"XXH32", 4, xxh32_exts, "XXH32"},
      {"XXH64", 8, xxh64_exts, "XXH64"},
      {"XXH3-64", 8, xxh3_64_exts, "XXH3-64"},
      {"XXH3-128", 16, xxh3_128_exts, "XXH3-128"},
      {"MD4", 16, md4_exts, "MD4"},
      {"MD5", 16, md5_exts, "MD5"},
      {"RipeMD160", 20, ripemd160_exts, "RipeMD160"},
      {"SHA-1", 20, sha1_exts, "SHA-1"},
      {"SHA-224", 28, sha224_exts, "SHA-224"},
      {"SHA-256", 32, sha256_exts, "SHA-256"},
      {"SHA-384", 48, sha384_exts, "SHA-384"},
      {"SHA-512", 64, sha512_exts, "SHA-512"},
      {"Blake2sp", 32, blake2sp_exts, "BLAKE2sp"},
      {"SHA3-224", 28, sha3_224_exts, "Keccak", as_param<1152, 448, 224, 0x06>},
      {"SHA3-256", 32, sha3_256_exts, "Keccak", as_param<1088, 512, 256, 0x06>},
      {"SHA3-384", 48, sha3_384_exts, "Keccak", as_param<832, 768, 384, 0x06>},
      {"SHA3-512", 64, sha3_512_exts, "Keccak", as_param<576, 1024, 512, 0x06>},
      {"K12-264", 33, k12_264_exts, "K12", as_param<264>},
      {"K12-256", 32, no_exts, "K12", as_param<256>},
      {"K12-512", 64, no_exts, "K12", as_param<512>},
      {"PH128-264", 33, ph128_264_exts, "PH128", as_param<8192, 264>},
      {"PH256-528", 66, ph256_528_exts, "PH256", as_param<8192, 528>},
      {"BLAKE3", 32, blake3_exts, "BLAKE3", as_param<256>},
      {"BLAKE3-512", 64, no_exts, "BLAKE3", as_param<512>},
      {"GOST 2012 (256)", 32, no_exts, "GOST 2012 (256)"},
      {"GOST 2012 (512)", 64, no_exts, "GOST 2012 (512)"},
      {"eD2k", 16, no_exts, "eD2k"},
      {"eD2k (Old)", 16, no_exts, "eD2k (Old)"},
      {"QuickXorHash", 20, no_exts, "QuickXorHash"},
  };

  return algorithms;
}

HashBox LegacyHashAlgorithm::MakeContext() const {
  return _algorithm->MakeContext(_params);
}
