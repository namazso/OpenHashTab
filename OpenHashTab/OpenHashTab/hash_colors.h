//    Copyright 2019-2025 namazso <admin@namazso.eu>
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
#include "Settings.h"

enum class HashColorType {
  Error,
  Match,
  Insecure,
  Mismatch,
  Unknown
};

struct HashColorSettingEntry {
  RegistrySetting<bool> Settings::*fg_enabled;
  RegistrySetting<COLORREF> Settings::*fg_color;
  RegistrySetting<bool> Settings::*bg_enabled;
  RegistrySetting<COLORREF> Settings::*bg_color;
  int settings_dlg_group;
  int settings_dlg_fg_check;
  int settings_dlg_fg_btn;
  int settings_dlg_bg_check;
  int settings_dlg_bg_btn;
  int settings_dlg_sample;
  int settings_dlg_group_str;
};

static constexpr HashColorSettingEntry HASH_COLOR_SETTING_MAP[5] = {
  {   &Settings::error_fg_enabled,
   &Settings::error_fg_color,
   &Settings::error_bg_enabled,
   &Settings::error_bg_color,
   IDC_ERROR_GROUP,
   IDC_ERROR_FG_CHECK,
   IDC_ERROR_FG_BTN,
   IDC_ERROR_BG_CHECK,
   IDC_ERROR_BG_BTN,
   IDC_ERROR_SAMPLE,
   IDS_ERROR_GROUP   },
  {   &Settings::match_fg_enabled,
   &Settings::match_fg_color,
   &Settings::match_bg_enabled,
   &Settings::match_bg_color,
   IDC_MATCH_GROUP,
   IDC_MATCH_FG_CHECK,
   IDC_MATCH_FG_BTN,
   IDC_MATCH_BG_CHECK,
   IDC_MATCH_BG_BTN,
   IDC_MATCH_SAMPLE,
   IDS_MATCH_GROUP   },
  {&Settings::insecure_fg_enabled,
   &Settings::insecure_fg_color,
   &Settings::insecure_bg_enabled,
   &Settings::insecure_bg_color,
   IDC_INSECURE_GROUP,
   IDC_INSECURE_FG_CHECK,
   IDC_INSECURE_FG_BTN,
   IDC_INSECURE_BG_CHECK,
   IDC_INSECURE_BG_BTN,
   IDC_INSECURE_SAMPLE,
   IDS_INSECURE_GROUP},
  {&Settings::mismatch_fg_enabled,
   &Settings::mismatch_fg_color,
   &Settings::mismatch_bg_enabled,
   &Settings::mismatch_bg_color,
   IDC_MISMATCH_GROUP,
   IDC_MISMATCH_FG_CHECK,
   IDC_MISMATCH_FG_BTN,
   IDC_MISMATCH_BG_CHECK,
   IDC_MISMATCH_BG_BTN,
   IDC_MISMATCH_SAMPLE,
   IDS_MISMATCH_GROUP},
  { &Settings::unknown_fg_enabled,
   &Settings::unknown_fg_color,
   &Settings::unknown_bg_enabled,
   &Settings::unknown_bg_color,
   IDC_UNKNOWN_GROUP,
   IDC_UNKNOWN_FG_CHECK,
   IDC_UNKNOWN_FG_BTN,
   IDC_UNKNOWN_BG_CHECK,
   IDC_UNKNOWN_BG_BTN,
   IDC_UNKNOWN_SAMPLE,
   IDS_UNKNOWN_GROUP },
};
