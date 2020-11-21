#include "stdafx.h"

#include "Hasher.h"

#include <random>

// rundll32 OpenHashTab.dll,BenchmarkEntry
extern "C" __declspec(dllexport) void CALLBACK BenchmarkEntryW(
  _In_  HWND      hWnd,
  _In_  HINSTANCE hRunDLLInstance,
  _In_  LPCWSTR   lpCmdLine,
  _In_  int       nShowCmd
)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hRunDLLInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nShowCmd);

  //constexpr static auto k_passes = 5u;
  //constexpr static auto k_size = 1ull << 30;
  constexpr static auto k_passes = 3u;
  constexpr static auto k_size = 100ull << 20;

  FILE* fp{};
  fopen_s(&fp, "benchmark.txt", "w");

  const auto p = (uint64_t*)VirtualAlloc(
    nullptr,
    k_size,
    MEM_RESERVE | MEM_COMMIT,
    PAGE_READWRITE
  );

  if(!p)
  {
    fprintf(fp, "VirtualAlloc failed.");
    return;
  }

  std::mt19937_64 engine{ 0 };  // NOLINT(cert-msc51-cpp)
  std::generate_n(p, k_size / sizeof(*p), [&engine] { return engine(); });

  LARGE_INTEGER frequency{};
  QueryPerformanceFrequency(&frequency);

  uint64_t measurements[k_passes][HashAlgorithm::k_count]{};

  for (auto& measurement : measurements)
  {
    for (auto i = 0u; i < HashAlgorithm::k_count; ++i)
    {
      auto& h = HashAlgorithm::g_hashers[i];
      const auto ctx = h.MakeContext();

      LARGE_INTEGER begin{}, end{};

      QueryPerformanceCounter(&begin);

      ctx->Update(p, k_size);
      (void)ctx->Finish();

      QueryPerformanceCounter(&end);

      delete ctx;

      measurement[i] = end.QuadPart - begin.QuadPart;
    }
  }

  for (auto i = 0u; i < HashAlgorithm::k_count; ++i)
  {
    auto& h = HashAlgorithm::g_hashers[i];

    fprintf(fp, "%s\t", h.GetName());

    uint64_t sum = 0;

    for (auto& measurement : measurements)
    {
      const auto v = measurement[i];
      sum += v;
      fprintf(fp, "%llu\t", v);
    }

    const auto mbps = (k_size * frequency.QuadPart) / (double(sum) / k_passes) / (1ull << 20); // MB/s

    fprintf(fp, "%.7lf MB/s\n", mbps);
  }

  fclose(fp);
}
