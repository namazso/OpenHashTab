//    Copyright 2019-2021 namazso <admin@namazso.eu>
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
#include "../Algorithms/Hasher.h"

namespace detail
{
  DWORD GetSettingDWORD(const char* name, DWORD default_value);
  void SetSettingDWORD(const char* name, DWORD new_value);
}

// 4 byte arbitrary setting storage
template <typename T>
class RegistrySetting
{
  static_assert(sizeof(T) <= sizeof(DWORD));
  static_assert(std::is_fundamental_v<T>);

  const char* _key{};
  T _value{};

  void Load(T default_value)
  {
    DWORD dw{};
    memcpy(&dw, &default_value, sizeof(T));
    dw = detail::GetSettingDWORD(_key, dw);
    memcpy(&_value, &dw, sizeof(T));
  }

  void Store()
  {
    DWORD dw{};
    memcpy(&dw, &_value, sizeof(T));
    detail::SetSettingDWORD(_key, dw);
  }

public:
  constexpr RegistrySetting() = default;

  RegistrySetting(const char* key, T default_value)
    : _key(key)
  {
    Load(default_value);
  }

  void Init(const char* key, T default_value)
  {
    _key = key;
    Load(default_value);
  }

  operator T() const { return _value; }
  T Get() const { return _value; }
  void Set(T v) { _value = v; Store(); }
  void SetNoSave(T v) { _value = v; }
};

struct Settings
{
  Settings();

  RegistrySetting<bool> algorithms[HashAlgorithm::k_count]{};

  RegistrySetting<bool> display_uppercase{ "DisplayUppercase", true };
  RegistrySetting<bool> look_for_sumfiles{ "LookForSumfiles", false };
  RegistrySetting<bool> sumfile_uppercase{ "SumfileUppercase", true };
  RegistrySetting<bool> sumfile_unix_endings{ "SumfileLF", true };
  RegistrySetting<bool> sumfile_use_double_space{ "SumfileDoubleSpace", false };
  RegistrySetting<bool> sumfile_forward_slashes{ "SumfileForwardSlash", true };
  RegistrySetting<bool> sumfile_dot_hash_compatible{ "SumfileDotHashCompat", true };
  RegistrySetting<bool> sumfile_banner{ "SumfileBanner", true };
  RegistrySetting<bool> sumfile_banner_date{ "SumfileBannerDate", false };
  RegistrySetting<bool> virustotal_tos{ "VTToS", false };
  RegistrySetting<bool> clipboard_autoenable{ "ClipboardAutoenable", true };
  RegistrySetting<bool> clipboard_autoenable_if_none{ "ClipboardAutoenableIfNone", true };
  RegistrySetting<bool> clipboard_autoenable_exclusive{ "ClipboardAutoenableExclusive", false };
  RegistrySetting<bool> checkagainst_autoformat{ "CheckAgainstAutoformat", false };
  RegistrySetting<bool> checkagainst_strict{ "CheckAgainstStruct", false };
  RegistrySetting<bool> hash_sumfile_too{ "HashSumfileToo", false };
  RegistrySetting<bool> sumfile_algorithm_only{ "SumfileAlgorithmOnly", true };
};