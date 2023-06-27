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
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <cassert>
#include <random>

#include <Hasher.h>

int main() {
  static constexpr auto k_passes = 20u;
  // 4 MB so that it fits in (my) L2 cache
  static constexpr auto k_size = 4ull << 20;

  const auto p = (uint64_t*)VirtualAlloc(
    nullptr,
    k_size,
    MEM_RESERVE | MEM_COMMIT,
    PAGE_READWRITE
  );

  if (!p) {
    printf("VirtualAlloc failed.");
    return 1;
  }

  std::mt19937_64 engine{0}; // NOLINT(cert-msc51-cpp)
  std::generate_n(p, k_size / sizeof(*p), [&engine] { return engine(); });

  LARGE_INTEGER frequency{};
  QueryPerformanceFrequency(&frequency);

  int64_t measurements[LegacyHashAlgorithm::k_count][k_passes]{};

  for (auto i = 0u; i < k_passes; ++i) {
    for (auto j = 0u; j < LegacyHashAlgorithm::k_count; ++j) {
      auto& h = LegacyHashAlgorithm::Algorithms()[j];
      auto ctx = h.MakeContext();

      LARGE_INTEGER begin{}, end{};

      QueryPerformanceCounter(&begin);

      ctx.Update(p, k_size);
      uint8_t hash[LegacyHashAlgorithm::k_max_size + 4];
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

      // copy into volatile to ensure Finish isn't optimized away
      volatile uint8_t hash_cpy[LegacyHashAlgorithm::k_max_size];
      std::copy_n(std::begin(hash), std::size(hash_cpy), std::begin(hash_cpy));

      QueryPerformanceCounter(&end);

      measurements[j][i] = end.QuadPart - begin.QuadPart;
    }
  }

  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i) {
    auto& h = LegacyHashAlgorithm::Algorithms()[i];

    printf("%-16s\t", h.GetName());

    int64_t sum = 0;

    auto& measurement = measurements[i];
    std::sort(std::begin(measurement), std::end(measurement));

    static constexpr auto skip = (unsigned)(0.2 * k_passes);
    for (auto j = skip; j < k_passes - skip; ++j) {
      const auto v = measurement[j];
      sum += v;
      printf("%lld\t", v);
    }

    const auto avg = (double(sum) / (k_passes - 2 * skip));

    const auto mbps = (double)(k_size * frequency.QuadPart) / avg / (1ll << 20); // MB/s

    printf("%.7lf MB/s\n", mbps);
  }

  return 0;
}
