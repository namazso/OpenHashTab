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

#include <Windows.h>

enum : uint32_t {
  SSE2_CPUID_MASK       = 1u << 26,
  OSXSAVE_CPUID_MASK    = (1u << 26) | (1u << 27),
  AVX_CPUID_MASK        = 1u << 28,
  AVX2_CPUID_MASK       = 1u << 5,
  AVX512F_CPUID_MASK    = 1u << 16,
  AVX512VL_CPUID_MASK   = 1u << 31,

  AVX_XGETBV_MASK       = (1u << 2) | (1u << 1),
  AVX512_XGETBV_MASK    = (7u << 5) | (1u << 2) | (1u << 1)
};

enum CPUFeatureLevel
{
  CPU_None,
  CPU_SSE2,
  CPU_AVX,
  CPU_AVX2,
  CPU_AVX512,
  CPU_NEON,

  CPU_MAX
};

// Returns the algorithms dll implementation to use
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
  __cpuidex(abcd, 7, 0);

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

static const wchar_t* get_cpu_level_wstr(CPUFeatureLevel level)
{
  switch (level)
  {
  default:
  case CPU_MAX:
  case CPU_None:
    return L"none";
  case CPU_SSE2:
    return L"sse2";
  case CPU_AVX:
    return L"avx";
  case CPU_AVX2:
    return L"avx2";
  case CPU_AVX512:
    return L"avx512";
  case CPU_NEON:
    return L"neon";
  }
}

static HMODULE g_hmodule{};

extern "C" char __ImageBase;

decltype(HashAlgorithm::k_algorithms)& HashAlgorithm::Algorithms()
{
  static auto algorithms = []
  {
    wchar_t dll[64] = L"AlgorithmsDll-"
#if defined(_M_IX86)
    L"Win32-"
#elif defined(_M_X64)
    L"x64-"
#elif defined(_M_ARM64)
    L"ARM64-"
#else
#error "wtf"
#endif
    ;

    wcscat_s(dll, get_cpu_level_wstr(get_cpu_level()));

    WCHAR path[MAX_PATH]{};
    GetModuleFileNameW((HMODULE)&__ImageBase, path, std::size(path));
    const auto fname = wcsrchr(path, L'\\');

    if (fname)
      wcscpy_s(fname + 1, std::end(path) - (fname + 1), dll);
    else
      wcscpy_s(path, dll);

    g_hmodule = LoadLibraryW(path);
    if (g_hmodule)
      atexit([]() { FreeLibrary(g_hmodule); });

    using fn_t = const HashAlgorithm* (*)();

    const auto fn = (fn_t)GetProcAddress(g_hmodule, "Algorithms");
    return fn();
  } ();

  return *reinterpret_cast<decltype(&HashAlgorithm::k_algorithms)>(algorithms);
}
