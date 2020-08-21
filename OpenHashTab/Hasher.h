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

class HashAlgorithm;

class HashContext
{
  HashAlgorithm* _algorithm{};

protected:
  HashContext(HashAlgorithm* algorithm) : _algorithm(algorithm) {}

public:
  HashContext(const HashContext&) = delete;
  HashContext(HashContext&&) = delete;
  HashContext& operator=(const HashContext&) = delete;
  HashContext& operator=(HashContext&&) = delete;
  virtual ~HashContext() = default;
  virtual void Clear() = 0;
  virtual void Update(const void* data, size_t size) = 0;
  virtual std::vector<uint8_t> Finish() = 0;
  HashAlgorithm* GetAlgorithm() const { return _algorithm; };
};

class HashAlgorithm
{
public:
  using FactoryFn = HashContext*(HashAlgorithm* algorithm);
  constexpr static auto k_count = 15;
  constexpr static auto k_max_size = 64;
  static HashAlgorithm g_hashers[k_count];
  static HashAlgorithm* ByName(std::string_view name)
  {
    const auto begin = std::begin(g_hashers);
    const auto end = std::end(g_hashers);
    const auto it = std::find_if(begin, end, [&name](const HashAlgorithm& a)
      {
        return name == a.GetName();
      });
    return it == end ? nullptr : it;
  }
  static int IdxByName(std::string_view name)
  {
    const auto it = ByName(name);
    return it == nullptr ? -1 : (int)(it - std::begin(g_hashers));
  }

private:
  const char* _name;
  const char* const* _extensions;
  FactoryFn* _factory_fn;
  uint32_t _size;
  bool _is_secure;
  bool _is_enabled;

  void LoadEnabled();
  void StoreEnabled() const;

public:
  HashAlgorithm(
    const char* name,
    uint32_t size,
    const char* const* extensions,
    FactoryFn* factory_fn,
    bool is_secure,
    bool is_enabled
  ) : _name(name)
    , _extensions(extensions)
    , _factory_fn(factory_fn)
    , _size(size)
    , _is_secure(is_secure)
    , _is_enabled(is_enabled)
  {
    LoadEnabled();
  }

  bool IsSecure() const { return _is_secure; }
  bool IsEnabled() const { return _is_enabled; }
  void SetEnabled(bool enabled)
  {
    if(enabled != _is_enabled)
    {
      _is_enabled = enabled;
      StoreEnabled();
    }
  }
  const char* GetName() const { return _name; }
  uint32_t GetSize() const { return _size; }
  const char* const* GetExtensions() const { return _extensions; }
  HashContext* MakeContext() { return _factory_fn(this); }
};
