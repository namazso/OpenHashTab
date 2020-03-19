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
#include "stdafx.h"

#include "Hasher.h"
#include "utl.h"

constexpr static auto k_reg_path = TEXT("Software\\OpenHashTab");

void HashAlgorithm::LoadEnabled()
{
  DWORD enabled;
  DWORD size = sizeof(enabled);
  const auto status = RegGetValue(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToTString(GetName()).c_str(),
    RRF_RT_REG_DWORD,
    nullptr,
    &enabled,
    &size
  );
  if(status == ERROR_SUCCESS && size == sizeof(enabled))
  {
    _is_enabled = (bool)enabled;
  }
}
void HashAlgorithm::StoreEnabled() const
{
  DWORD value = _is_enabled;
  RegSetKeyValue(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToTString(GetName()).c_str(),
    REG_DWORD,
    &value,
    sizeof(value)
  );
  // we don't care about the result
}
