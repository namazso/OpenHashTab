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
#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>

class HashAlgorithm;

#define CONTEXT_ALIGN (64)
#define CONTEXT_SIZE (2048)

class alignas(CONTEXT_ALIGN) HashContext
{
public:
  constexpr HashContext() = default;
  HashContext(const HashContext&) = delete;
  HashContext(HashContext&&) = delete;
  HashContext& operator=(const HashContext&) = delete;
  HashContext& operator=(HashContext&&) = delete;

  // Hash contexts should always be POD
  // virtual ~HashContext() = default;

  virtual void Update(const void* data, size_t size) = 0;
  virtual void Finish(uint8_t* out) = 0;
  virtual size_t GetOutputSize() = 0;
};

union alignas(alignof(HashContext)) HashContextStorage
{
private:
  struct MyHashContext : HashContext
  {
    void Update(const void* data, size_t size) override {}
    void Finish(uint8_t* out) override {}
    size_t GetOutputSize() override { return 0; }
  };

  MyHashContext _hc;
  char _padding[CONTEXT_SIZE]{};

public:
  constexpr HashContextStorage() {}
};


class HashAlgorithm
{
public:
  using FactoryFn = void (const uint64_t* params, void* memory);
  using ParamCheckFn = size_t (const uint64_t* params); // returns: length of the output for the given params, 0 if invalid
  
  const char* name;
  FactoryFn* factory_fn;
  ParamCheckFn* param_check_fn;
  const char* const* params;
  uint32_t params_size;
  bool is_secure;

  constexpr HashAlgorithm(
    const char* name,
    FactoryFn* factory_fn,
    ParamCheckFn* param_check_fn,
    bool is_secure,
    const char* const* params,
    uint32_t params_size
  ) : name(name)
    , factory_fn(factory_fn)
    , param_check_fn(param_check_fn)
    , params(params)
    , params_size(params_size)
    , is_secure(is_secure) {}

  template <size_t N>
  constexpr HashAlgorithm(
    const char* name,
    FactoryFn* factory_fn,
    ParamCheckFn* param_check_fn,
    bool is_secure,
    const char* const(&params)[N]
  ) : name(name)
    , factory_fn(factory_fn)
    , param_check_fn(param_check_fn)
    , params(params)
    , params_size(N)
    , is_secure(is_secure) {}
};