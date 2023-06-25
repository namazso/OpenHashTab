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

#include "hash_colors.h"
#include "wnd.h"

struct Settings;

class SettingsDialog {
  HWND _hwnd;

  MAKE_IDC_MEMBER(_hwnd, BUTTON_CHECK_FOR_UPDATES);
  MAKE_IDC_MEMBER(_hwnd, ALGORITHM_LIST);

  MAKE_IDC_MEMBER(_hwnd, CHECK_SUMFILE_BANNER);
  MAKE_IDC_MEMBER(_hwnd, CHECK_SUMFILE_BANNER_DATE);

  MAKE_IDC_MEMBER(_hwnd, CHECK_CLIPBOARD_AUTOENABLE);
  MAKE_IDC_MEMBER(_hwnd, CHECK_CLIPBOARD_AUTOENABLE_IF_NONE);
  MAKE_IDC_MEMBER(_hwnd, CHECK_CLIPBOARD_AUTOENABLE_EXCLUSIVE);

  MAKE_IDC_MEMBER(_hwnd, PROJECT_NAME);

  HWND _samples[std::size(HASH_COLOR_SETTING_MAP)]{};

  Settings* _settings;
  utl::UniqueFont _font{utl::GetDPIScaledFont()};
  bool _done_setup = false;

  void UpdateCheckboxAvailability();
  void UpdateColorItems();

public:
  SettingsDialog(HWND handle, void* settings)
      : _hwnd(handle)
      , _settings((Settings*)settings) {
    for (size_t i = 0; i < std::size(HASH_COLOR_SETTING_MAP); ++i)
      _samples[i] = GetDlgItem(_hwnd, HASH_COLOR_SETTING_MAP[i].settings_dlg_sample);
  }

  INT_PTR DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
