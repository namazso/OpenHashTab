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
#include "base64.h"

// clang-format off
static const char encode_table[64]{
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

// clang-format on

std::string b64::encode(const uint8_t* src, size_t len) {
  const auto olen = 4 * ((len + 2) / 3);

  std::string str;
  str.resize(olen);

  const auto begin = src;
  const auto end = begin + len;
  auto it = begin;
  auto out = str.begin();
  while (end - it >= 3) {
    *out++ = encode_table[it[0] >> 2];
    *out++ = encode_table[((it[0] & 0x03) << 4) | (it[1] >> 4)];
    *out++ = encode_table[((it[1] & 0x0f) << 2) | (it[2] >> 6)];
    *out++ = encode_table[it[2] & 0x3f];
    it += 3;
  }

  if (end - it) {
    *out++ = encode_table[it[0] >> 2];
    if (end - it == 1) {
      *out++ = encode_table[(it[0] & 0x03) << 4];
      *out++ = '=';
    } else {
      *out++ = encode_table[((it[0] & 0x03) << 4) | (it[1] >> 4)];
      *out++ = encode_table[(it[1] & 0x0f) << 2];
    }
    *out = '=';
  }

  return str;
}

// clang-format off
static const uint32_t decode_table[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32,
  33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

// clang-format on

std::vector<uint8_t> b64::decode(const char* str, const size_t len) {
  const auto p = reinterpret_cast<const uint8_t*>(str);
  const auto pad = len > 0 && (len % 4 || p[len - 1] == '=');
  const auto L = ((len + 3) / 4 - pad) * 4;
  std::vector<uint8_t> vec(L / 4 * 3 + pad, 0);

  for (size_t i = 0, j = 0; i < L; i += 4) {
    const auto n = decode_table[p[i]] << 18 | decode_table[p[i + 1]] << 12 | decode_table[p[i + 2]] << 6 | decode_table[p[i + 3]];
    vec[j++] = n >> 16 & 0xFF;
    vec[j++] = n >> 8 & 0xFF;
    vec[j++] = n & 0xFF;
  }
  if (pad) {
    auto n = decode_table[p[L]] << 18 | decode_table[p[L + 1]] << 12;
    vec[vec.size() - 1] = n >> 16 & 0xFF;

    if (len > L + 2 && p[L + 2] != '=') {
      n |= decode_table[p[L + 2]] << 6;
      vec.push_back(n >> 8 & 0xFF);
    }
  }
  return vec;
}
