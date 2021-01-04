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
#include "stdafx.h"

#include "Settings.h"

#include "utl.h"

constexpr static auto k_reg_path = L"Software\\OpenHashTab";

DWORD detail::GetSettingDWORD(const char* name, DWORD default_value)
{
  DWORD value;
  DWORD size = sizeof(value);
  const auto status = RegGetValueW(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToWide(name).c_str(),
    RRF_RT_REG_DWORD,
    nullptr,
    &value,
    &size
  );
  return status == ERROR_SUCCESS ? value : default_value;
}

void detail::SetSettingDWORD(const char* name, DWORD new_value)
{
  RegSetKeyValueW(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToWide(name).c_str(),
    REG_DWORD,
    &new_value,
    sizeof(new_value)
  );
}

Settings::Settings()
{
  bool defaults[HashAlgorithm::k_count]{};
  for (const auto name : {"MD5", "SHA-1", "SHA-256", "SHA-512"})
    defaults[HashAlgorithm::IdxByName(name)] = true;
  for (auto i = 0u; i < HashAlgorithm::k_count; ++i)
    algorithms[i].Init(HashAlgorithm::Algorithms()[i].GetName(), defaults[i]);
}
