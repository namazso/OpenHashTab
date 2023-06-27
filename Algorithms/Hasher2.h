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
#include <cstdint>
#include <cstddef>

#define ALGORITHMS_CC __stdcall

class HashContext;

class HashAlgorithm
{
  friend class HashBox;

  using ParamCheckFn = size_t ALGORITHMS_CC(const uint64_t* params); // returns: length of the output for the given params, 0 if invalid
  using FactoryFn = HashContext* ALGORITHMS_CC(const uint64_t* params);

  using UpdateFn = void ALGORITHMS_CC(HashContext* ctx, const void* data, size_t size);
  using FinishFn = void ALGORITHMS_CC(HashContext* ctx, uint8_t* out);
  using GetOutputSizeFn = size_t ALGORITHMS_CC(HashContext* ctx);

  using DeleteFn = void ALGORITHMS_CC(HashContext* ctx);

  ParamCheckFn* _param_check_fn;
  FactoryFn* _factory_fn;
  UpdateFn* _update_fn;
  FinishFn* _finish_fn;
  GetOutputSizeFn* _get_output_size_fn;
  DeleteFn* _delete_fn;

public:
  const char* name;
  const char* const* params;
  uint32_t params_size;
  bool is_secure;

  HashBox MakeContext(const uint64_t* params) const;
  size_t ParamCheck(const uint64_t* _params) const { return _param_check_fn(_params); }

  constexpr HashAlgorithm(
    ParamCheckFn* param_check_fn,
    FactoryFn* factory_fn,
    UpdateFn* update_fn,
    FinishFn* finish_fn,
    GetOutputSizeFn* get_output_size_fn,
    DeleteFn* delete_fn,
    const char* name,
    bool is_secure,
    const char* const* params,
    uint32_t params_size
  ) : _param_check_fn(param_check_fn)
    , _factory_fn(factory_fn)
    , _update_fn(update_fn)
    , _finish_fn(finish_fn)
    , _get_output_size_fn(get_output_size_fn)
    , _delete_fn(delete_fn)
    , name(name)
    , params(params)
    , params_size(params_size)
    , is_secure(is_secure) {}

  template <size_t N>
  constexpr HashAlgorithm(
    ParamCheckFn* param_check_fn,
    FactoryFn* factory_fn,
    UpdateFn* update_fn,
    FinishFn* finish_fn,
    GetOutputSizeFn* get_output_size_fn,
    DeleteFn* delete_fn,
    const char* name,
    bool is_secure,
    const char* const(&params)[N]
  ) : _param_check_fn(param_check_fn)
    , _factory_fn(factory_fn)
    , _update_fn(update_fn)
    , _finish_fn(finish_fn)
    , _get_output_size_fn(get_output_size_fn)
    , _delete_fn(delete_fn)
    , name(name)
    , params(params)
    , params_size(N)
    , is_secure(is_secure) {}
};

class HashBox
{
  const HashAlgorithm* _algorithm{};
  HashContext* _ctx{};

public:
  constexpr HashBox() {}

  HashBox(const HashAlgorithm& algorithm, const uint64_t* params)
    : _algorithm(&algorithm)
    , _ctx(_algorithm->_factory_fn(params)) {}

  ~HashBox() { if(_ctx) _algorithm->_delete_fn(_ctx); }

  HashBox(const HashBox&) = delete;
  HashBox(HashBox&& rhs) noexcept
    : _algorithm(rhs._algorithm)
    , _ctx(rhs._ctx)
  {
    rhs._algorithm = nullptr;
    rhs._ctx = nullptr;
  }

  HashBox& operator=(const HashBox&) = delete;
  HashBox& operator=(HashBox&& rhs) noexcept
  {
    _algorithm = rhs._algorithm;
    _ctx = rhs._ctx;
    rhs._algorithm = nullptr;
    rhs._ctx = nullptr;
    return *this;
  }

  void Initialize(const HashAlgorithm& algorithm, const uint64_t* params)
  {
    _algorithm = &algorithm;
    _ctx = _algorithm->_factory_fn(params);
  }

  bool IsInitialized() const { return _ctx != nullptr; }

  void Update(const void* data, size_t size) { _algorithm->_update_fn(_ctx, data, size); }
  void Finish(uint8_t* out) { _algorithm->_finish_fn(_ctx, out); }
  size_t GetOutputSize() const { return _algorithm->_get_output_size_fn(_ctx); }
};

inline HashBox HashAlgorithm::MakeContext(const uint64_t* params_) const
{
  return { *this, params_ };
}
