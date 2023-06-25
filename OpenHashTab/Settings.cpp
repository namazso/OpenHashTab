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
#include "Settings.h"

#include "utl.h"

static constexpr auto k_reg_path = L"Software\\OpenHashTab";

DWORD detail::GetMachineSettingDWORD(const char* name, DWORD default_value) {
  DWORD value;
  DWORD size = sizeof(value);
  const auto status = RegGetValueW(
    HKEY_LOCAL_MACHINE,
    k_reg_path,
    utl::UTF8ToWide(name).c_str(),
    RRF_RT_REG_DWORD,
    nullptr,
    &value,
    &size
  );
  return status == ERROR_SUCCESS ? value : default_value;
}

DWORD detail::GetSettingDWORD(const char* name, DWORD default_value) {
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

void detail::SetSettingDWORD(const char* name, DWORD new_value) {
  RegSetKeyValueW(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToWide(name).c_str(),
    REG_DWORD,
    &new_value,
    sizeof(new_value)
  );
}

Settings::Settings() {
  bool defaults[LegacyHashAlgorithm::k_count]{};
  for (const auto name : {"MD5", "SHA-1", "SHA-256", "SHA-512"})
    defaults[LegacyHashAlgorithm::IdxByName(name)] = true;
  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i)
    algorithms[i].Init(LegacyHashAlgorithm::Algorithms()[i].GetName(), defaults[i]);
}
