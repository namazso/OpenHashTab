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
#pragma once
#include <cstdint>
#include <vector>
#include <string_view>

class HashAlgorithm;

class HashContext
{
  const HashAlgorithm* _algorithm{};

protected:
  HashContext(const HashAlgorithm* algorithm) : _algorithm(algorithm) {}

public:
  HashContext(const HashContext&) = delete;
  HashContext(HashContext&&) = delete;
  HashContext& operator=(const HashContext&) = delete;
  HashContext& operator=(HashContext&&) = delete;
  virtual ~HashContext() = default;
  virtual void Clear() = 0;
  virtual void Update(const void* data, size_t size) = 0;
  virtual void Finish(uint8_t* out) = 0;

  std::vector<uint8_t> Finish();

  const HashAlgorithm* GetAlgorithm() const { return _algorithm; }
};

class HashAlgorithm
{
public:
  using FactoryFn = HashContext* (const HashAlgorithm* algorithm);
  constexpr static auto k_count = 19;
  constexpr static auto k_max_size = 64;
  static const HashAlgorithm g_hashers[k_count];
  static constexpr const HashAlgorithm* ByName(std::string_view name)
  {
    for (const auto& algo : g_hashers)
      if (algo.GetName() == name)
        return &algo;
    return nullptr;
  }
  static constexpr int Idx(const HashAlgorithm* algorithm)
  {
    return algorithm ? algorithm - std::begin(g_hashers) : -1;
  }
  static constexpr int IdxByName(std::string_view name)
  {
    return Idx(ByName(name));
  }

private:
  const char* _name;
  const char* const* _extensions;
  FactoryFn* _factory_fn;
  uint32_t _size;
  bool _is_secure;

  constexpr HashAlgorithm(
    const char* name,
    uint32_t size,
    const char* const* extensions,
    FactoryFn* factory_fn,
    bool is_secure
  ) : _name(name)
    , _extensions(extensions)
    , _factory_fn(factory_fn)
    , _size(size)
    , _is_secure(is_secure) {}

public:
  HashAlgorithm(const HashAlgorithm&) = delete;
  HashAlgorithm(HashAlgorithm&&) = delete;

  constexpr int Idx() const { return Idx(this); }

  constexpr bool IsSecure() const { return _is_secure; }

  constexpr const char* GetName() const { return _name; }
  constexpr uint32_t GetSize() const { return _size; }
  constexpr const char* const* GetExtensions() const { return _extensions; }
  constexpr HashContext* MakeContext() const { return _factory_fn(this); }
};

inline std::vector<uint8_t> HashContext::Finish()
{
  std::vector<uint8_t> vec;
  vec.resize(HashAlgorithm::k_max_size);
  Finish(vec.data());
  vec.resize(GetAlgorithm()->GetSize());
  return vec;
}
