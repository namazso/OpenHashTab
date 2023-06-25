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
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "../Algorithms/Hasher2.h"

class LegacyHashAlgorithm {
public:
  static constexpr auto k_count = 31;
  static constexpr auto k_max_size = 66;

  using AlgorithmsType = LegacyHashAlgorithm[k_count];

  static AlgorithmsType& Algorithms();

  static const LegacyHashAlgorithm* ByName(std::string_view name) {
    for (const auto& algo : Algorithms())
      if (algo.GetName() == name)
        return &algo;
    return nullptr;
  }

  static int Idx(const LegacyHashAlgorithm* algorithm) {
    return algorithm ? (int)(algorithm - std::begin(Algorithms())) : -1;
  }

  static int IdxByName(std::string_view name) {
    return Idx(ByName(name));
  }

private:
  const char* _name;
  const char* const* _extensions;
  const HashAlgorithm* _algorithm{};
  const uint64_t* _params{};
  uint32_t _size{};
  bool _is_secure{};

  LegacyHashAlgorithm(
    const char* name,
    size_t expected_size,
    const char* const* extensions,
    const char* alg_name,
    const uint64_t* params = nullptr
  );

public:
  LegacyHashAlgorithm(const LegacyHashAlgorithm&) = delete;
  LegacyHashAlgorithm(LegacyHashAlgorithm&&) = delete;

  int Idx() const { return Idx(this); }

  constexpr bool IsSecure() const { return _is_secure; }

  constexpr const char* GetName() const { return _name; }

  constexpr uint32_t GetSize() const { return _size; }

  constexpr const char* const* GetExtensions() const { return _extensions; }

  HashBox MakeContext() const;
};
