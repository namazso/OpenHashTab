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

#include "Settings.h"
#include "utl.h"

constexpr static auto k_reg_path = L"Software\\OpenHashTab";

Settings Settings::instance{};

void Settings::LoadHashEnabled(const HashAlgorithm* algorithm)
{
  DWORD enabled;
  DWORD size = sizeof(enabled);
  const auto status = RegGetValueW(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToTString(algorithm->GetName()).c_str(),
    RRF_RT_REG_DWORD,
    nullptr,
    &enabled,
    &size
  );
  if (status == ERROR_SUCCESS && size == sizeof(enabled))
  {
    _enabled[algorithm->Idx()] = (bool)enabled;
  }
}

void Settings::StoreHashEnabled(const HashAlgorithm* algorithm) const
{
  DWORD value = _enabled[algorithm->Idx()];
  RegSetKeyValueW(
    HKEY_CURRENT_USER,
    k_reg_path,
    utl::UTF8ToTString(algorithm->GetName()).c_str(),
    REG_DWORD,
    &value,
    sizeof(value)
  );
  // we don't care about the result
}