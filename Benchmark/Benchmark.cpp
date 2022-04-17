//    Copyright 2019-2022 namazso <admin@namazso.eu>
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
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <cassert>
#include <Windows.h>
#include <random>

#include "../AlgorithmsLoader/Hasher.h"

int main()
{
  //constexpr static auto k_passes = 5u;
  //constexpr static auto k_size = 1ull << 30;
  constexpr static auto k_passes = 3u;
  constexpr static auto k_size = 100ull << 20;

  const auto p = (uint64_t*)VirtualAlloc(
    nullptr,
    k_size,
    MEM_RESERVE | MEM_COMMIT,
    PAGE_READWRITE
  );

  if (!p)
  {
    printf("VirtualAlloc failed.");
    return 1;
  }

  std::mt19937_64 engine{ 0 };  // NOLINT(cert-msc51-cpp)
  std::generate_n(p, k_size / sizeof(*p), [&engine] { return engine(); });

  LARGE_INTEGER frequency{};
  QueryPerformanceFrequency(&frequency);

  uint64_t measurements[k_passes][LegacyHashAlgorithm::k_count]{};

  for (auto& measurement : measurements)
  {
    for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i)
    {
      auto& h = LegacyHashAlgorithm::Algorithms()[i];
      auto ctx = h.MakeContext();

      LARGE_INTEGER begin{}, end{};

      QueryPerformanceCounter(&begin);

      ctx.Update(p, k_size);
      uint8_t hash[LegacyHashAlgorithm::k_max_size+4];
#ifndef NDEBUG
      const auto size = h.GetSize();
      hash[size - 4] = 0xFF;
      hash[size - 3] = 0xFF;
      hash[size - 2] = 0xFF;
      hash[size - 1] = 0xFF;
      hash[size] = 0xFF;
      hash[size + 1] = 0xFF;
      hash[size + 2] = 0xFF;
      hash[size + 3] = 0xFF;
#endif
      ctx.Finish(hash);
#ifndef NDEBUG
      const auto size_according_to_ctx = ctx.GetOutputSize();
      assert(size == size_according_to_ctx);
      const auto doesnt_overflow = std::all_of(
        &hash[size],
        &hash[size + 4],
        [](uint8_t v) { return v == 0xFF; }
      );
      assert(doesnt_overflow);
      const auto fills_space = !std::all_of(
        &hash[size - 4],
        &hash[size],
        [](uint8_t v) { return v == 0xFF; }
      );
      assert(fills_space);
#endif

      // copy into volatile to ensure Finish isnt optimized away
      volatile uint8_t hash_cpy[LegacyHashAlgorithm::k_max_size];
      std::copy_n(std::begin(hash), std::size(hash_cpy), std::begin(hash_cpy));

      QueryPerformanceCounter(&end);
      
      measurement[i] = end.QuadPart - begin.QuadPart;
    }
  }

  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i)
  {
    auto& h = LegacyHashAlgorithm::Algorithms()[i];

    printf("%s\t", h.GetName());

    uint64_t sum = 0;

    for (const auto& measurement : measurements)
    {
      const auto v = measurement[i];
      sum += v;
      printf("%llu\t", v);
    }

    const auto mbps = (k_size * frequency.QuadPart) / (double(sum) / k_passes) / (1ull << 20); // MB/s

    printf("%.7lf MB/s\n", mbps);
  }

  return 0;
}
