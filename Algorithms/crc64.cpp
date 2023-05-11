// public domain, by duk
#include "crc64.h"
#include <array>
#include <cstdint>

constexpr uint64_t poly = 0xC96C5795D7870F42ULL;
constexpr uint32_t max_slice = 16;

static constexpr std::array<std::array<uint64_t, 256>, max_slice> crc_table = []() {
  std::array<std::array<uint64_t, 256>, max_slice> out{};
  for (uint32_t i = 0; i <= 0xFF; ++i) {
    uint64_t crc = i;
    for (uint32_t i = 0; i < 8; ++i) {
      crc = (crc >> 1) ^ ((crc & 1) * poly);
    }
    out[0][i] = crc;
  }

  for (uint32_t slice = 1; slice < max_slice; slice++) {
    for (uint32_t i = 0; i <= 0xFF; ++i) {
      out[slice][i] = (out[slice - 1][i] >> 8) ^ out[0][out[slice - 1][i] & 0xFF];
    }
  }

  return out;
}();

uint64_t crc64(uint64_t crc, const void* buf_, size_t len)
{
  auto buf = (const uint8_t*)buf_;

  crc = ~crc;

  const auto to_align = std::min(((uintptr_t)buf) & 7, len);
  for (size_t i = 0; i < to_align; ++i)
  {
    crc = crc_table[0][(buf[i] ^ crc) & 0xFF] ^ (crc >> 8);
  }
  buf += to_align;
  len -= to_align;

  uint32_t off = 0;

  for (; off < len - (len % 8); off += 8)
  {
    uint64_t value = *(uint64_t*)&buf[off] ^ crc;
    crc = crc_table[0][(value >> 56) & 0xFF];

#pragma unroll
    for (uint32_t i = 1; i < 8; ++i)
    {
      crc ^= crc_table[i][(value >> (64 - ((i + 1) * 8))) & 0xFF];
    }
  }

  for (; off < len; ++off)
  {
    crc = crc_table[0][(buf[off] ^ crc) & 0xFF] ^ (crc >> 8);
  }

  return ~crc;
}
