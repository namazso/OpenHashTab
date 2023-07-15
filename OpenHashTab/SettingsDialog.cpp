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
#include "SettingsDialog.h"

#include "Settings.h"
#include "utl.h"

struct SettingCheckbox {
  RegistrySetting<bool> Settings::*setting;
  int control_id;
  int string_id;
};

#define CTLSTR(name) IDC_CHECK_##name, IDS_##name

// clang-format off
static constexpr SettingCheckbox s_boxes[] =
{
  { &Settings::display_uppercase,               CTLSTR(DISPLAY_UPPERCASE              ) },
  { &Settings::display_monospace,               CTLSTR(DISPLAY_MONOSPACE              ) },
  { &Settings::look_for_sumfiles,               CTLSTR(LOOK_FOR_SUMFILES              ) },
  { &Settings::sumfile_uppercase,               CTLSTR(SUMFILE_UPPERCASE              ) },
  { &Settings::sumfile_unix_endings,            CTLSTR(SUMFILE_UNIX_ENDINGS           ) },
  { &Settings::sumfile_use_double_space,        CTLSTR(SUMFILE_USE_DOUBLE_SPACE       ) },
  { &Settings::sumfile_forward_slashes,         CTLSTR(SUMFILE_FORWARD_SLASHES        ) },
  { &Settings::sumfile_dot_hash_compatible,     CTLSTR(SUMFILE_DOT_HASH_COMPATIBLE    ) },
  { &Settings::sumfile_banner,                  CTLSTR(SUMFILE_BANNER                 ) },
  { &Settings::sumfile_banner_date,             CTLSTR(SUMFILE_BANNER_DATE            ) },
  { &Settings::clipboard_autoenable,            CTLSTR(CLIPBOARD_AUTOENABLE           ) },
  { &Settings::clipboard_autoenable_if_none,    CTLSTR(CLIPBOARD_AUTOENABLE_IF_NONE   ) },
  { &Settings::clipboard_autoenable_exclusive,  CTLSTR(CLIPBOARD_AUTOENABLE_EXCLUSIVE ) },
  { &Settings::checkagainst_autoformat,         CTLSTR(CHECKAGAINST_AUTOFORMAT        ) },
  { &Settings::checkagainst_strict,             CTLSTR(CHECKAGAINST_STRICT            ) },
  { &Settings::hash_sumfile_too,                CTLSTR(HASH_SUMFILE_TOO               ) },
  { &Settings::sumfile_algorithm_only,          CTLSTR(SUMFILE_ALGORITHM_ONLY         ) },
};
// clang-format on

#undef CTLSTR

static bool DoColorButton(HWND owner, RegistrySetting<COLORREF>& setting) {
  static COLORREF defcolors[16] = {
    RGB(45, 170, 23),
    RGB(170, 82, 23),
    RGB(230, 55, 23),
    RGB(255, 55, 23),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
  };

  CHOOSECOLORW ccw{};
  ccw.lStructSize = sizeof(ccw);
  ccw.hwndOwner = owner;
  ccw.rgbResult = setting.Get();
  ccw.lpCustColors = defcolors;
  ccw.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
  if (ChooseColorW(&ccw)) {
    setting.Set(ccw.rgbResult);
    return true;
  }
  return false;
}

static std::wstring FormatColor(COLORREF clr) {
  return utl::FormatString(L"#%06X", ((uint32_t)GetRValue(clr) << 16) | ((uint32_t)GetGValue(clr) << 8) | GetBValue(clr));
}

void SettingsDialog::UpdateCheckboxAvailability() {
  const auto banner = BST_CHECKED == Button_GetCheck(_hwnd_CHECK_SUMFILE_BANNER);
  Button_Enable(_hwnd_CHECK_SUMFILE_BANNER_DATE, banner);

  const auto clipboard = BST_CHECKED == Button_GetCheck(_hwnd_CHECK_CLIPBOARD_AUTOENABLE);
  Button_Enable(_hwnd_CHECK_CLIPBOARD_AUTOENABLE_IF_NONE, clipboard);
  Button_Enable(_hwnd_CHECK_CLIPBOARD_AUTOENABLE_EXCLUSIVE, clipboard);
}

void SettingsDialog::UpdateColorItems() {
  for (const auto& e : HASH_COLOR_SETTING_MAP) {
    Button_SetCheck(GetDlgItem(_hwnd, e.settings_dlg_fg_check), _settings->*e.fg_enabled ? BST_CHECKED : 0);
    Button_SetCheck(GetDlgItem(_hwnd, e.settings_dlg_bg_check), _settings->*e.bg_enabled ? BST_CHECKED : 0);
    SetWindowTextW(GetDlgItem(_hwnd, e.settings_dlg_fg_btn), FormatColor((_settings->*e.fg_color).Get()).c_str());
    SetWindowTextW(GetDlgItem(_hwnd, e.settings_dlg_bg_btn), FormatColor((_settings->*e.bg_color).Get()).c_str());
    RedrawWindow(GetDlgItem(_hwnd, e.settings_dlg_sample), nullptr, nullptr, RDW_INVALIDATE);
  }
}

INT_PTR SettingsDialog::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  const auto pnmhdr = (LPNMHDR)lParam;
  switch (uMsg) {
  case WM_INITDIALOG: {
    const auto icon = LoadIconW(utl::GetInstance(), MAKEINTRESOURCEW(IDI_ICON_COG));
    SendMessageW(_hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

    SetWindowTextW(_hwnd_PROJECT_NAME, L"OpenHashTab " CI_VERSION);

    utl::SetWindowTextStringFromTable(_hwnd, IDS_SETTINGS_TITLE);
    utl::SetWindowTextStringFromTable(_hwnd_BUTTON_CHECK_FOR_UPDATES, IDS_CHECK_FOR_UPDATES);

    const auto fg_text = utl::GetString(IDS_FOREGROUND);
    const auto bg_text = utl::GetString(IDS_BACKGROUND);

    for (const auto& e : HASH_COLOR_SETTING_MAP) {
      const auto wnd = GetDlgItem(_hwnd, e.settings_dlg_group);
      const auto str = utl::GetString(e.settings_dlg_group_str);
      SetWindowTextW(wnd, str.c_str());
      SetWindowTextW(GetDlgItem(_hwnd, e.settings_dlg_fg_check), fg_text.c_str());
      SetWindowTextW(GetDlgItem(_hwnd, e.settings_dlg_bg_check), bg_text.c_str());
    }

    UpdateColorItems();

    utl::SetFontForChildren(_hwnd, _font.get());

    const auto list = _hwnd_ALGORITHM_LIST;
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    auto width = 0;
    for (const auto& algorithm : LegacyHashAlgorithm::Algorithms()) {
      const auto name = utl::UTF8ToWide(algorithm.GetName());
      width = std::max(width, ListView_GetStringWidth(list, name.c_str()));
      LVITEMW lvitem{
        LVIF_PARAM,
        INT_MAX,
        0,
        0,
        0,
        const_cast<LPWSTR>(L"")};
      lvitem.lParam = reinterpret_cast<LPARAM>(&algorithm);
      const auto item = ListView_InsertItem(list, &lvitem);
      ListView_SetItemText(list, item, 0, const_cast<LPWSTR>(name.c_str()));
      ListView_SetCheckState(list, item, _settings->algorithms[algorithm.Idx()]);
    }

    // let's hope 40 px padding is good enough
    ListView_SetColumnWidth(list, 0, (intptr_t)width + utl::GetDPIScaledPixels(list, 40));

    for (const auto& ctl : s_boxes) {
      const auto ctl_hwnd = GetDlgItem(_hwnd, ctl.control_id);
      Button_SetCheck(ctl_hwnd, _settings->*ctl.setting);
      SetWindowTextW(ctl_hwnd, utl::GetString((UINT)ctl.string_id).c_str());
    }

    UpdateCheckboxAvailability();

    _done_setup = true;
    return FALSE; // do not select default control
  }

  case WM_COMMAND: {
    const auto id = LOWORD(wParam);
    const auto code = HIWORD(wParam);
    if (code == BN_CLICKED && (id == IDOK || id == IDCANCEL)) {
      EndDialog(_hwnd, LOWORD(wParam));
      return TRUE;
    }
    if (code == BN_CLICKED && id == IDC_BUTTON_CHECK_FOR_UPDATES) {
      try {
        static constexpr auto current = utl::Version{CI_VERSION_MAJOR, CI_VERSION_MINOR, CI_VERSION_PATCH};
        if constexpr (current.AsNumber() == 0) {
          MessageBoxW(
            _hwnd,
            utl::GetString(IDS_UPDATE_DISABLED_IN_DEV_BUILD).c_str(),
            utl::GetString(IDS_ERROR).c_str(),
            MB_OK | MB_ICONERROR
          );
        } else if (const auto v = utl::GetLatestVersion(); v > current)
          utl::FormattedMessageBox(
            _hwnd,
            utl::GetString(IDS_UPDATE_AVAILABLE_TITLE).c_str(),
            MB_OK | MB_ICONINFORMATION,
            utl::GetString(IDS_UPDATE_AVAILABLE_TEXT).c_str(),
            v.major,
            v.minor,
            v.patch
          );
        else
          MessageBoxW(
            _hwnd,
            utl::GetString(IDS_UPDATE_NEWEST_TEXT).c_str(),
            utl::GetString(IDS_UPDATE_NEWEST_TITLE).c_str(),
            MB_OK
          );

      } catch (const std::runtime_error& e) {
        MessageBoxW(
          _hwnd,
          utl::UTF8ToWide(e.what()).c_str(),
          L"Runtime error",
          MB_ICONERROR | MB_OK
        );
      }
      break;
    }
    if (code == BN_CLICKED) {
      for (const auto& ctl : s_boxes)
        if (id == ctl.control_id)
          (_settings->*ctl.setting).Set(BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, ctl.control_id)));

      UpdateCheckboxAvailability();

      for (const auto& e : HASH_COLOR_SETTING_MAP) {
        bool changed = true;
        if (id == e.settings_dlg_fg_check)
          (_settings->*e.fg_enabled).Set(BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, e.settings_dlg_fg_check)));
        else if (id == e.settings_dlg_bg_check)
          (_settings->*e.bg_enabled).Set(BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, e.settings_dlg_bg_check)));
        else if (id == e.settings_dlg_fg_btn)
          changed = DoColorButton(_hwnd, _settings->*e.fg_color);
        else if (id == e.settings_dlg_bg_btn)
          changed = DoColorButton(_hwnd, _settings->*e.bg_color);
        else
          changed = false;

        if (changed)
          UpdateColorItems();
      }
    }
    break;
  }

  case WM_NOTIFY:
    if (pnmhdr->idFrom == IDC_ALGORITHM_LIST && pnmhdr->code == LVN_ITEMCHANGED && _done_setup) {
      const auto pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      const auto idx = pnmlv->iItem;
      const auto list = pnmhdr->hwndFrom;
      const auto check = static_cast<bool>(ListView_GetCheckState(list, idx));
      LVITEMW lvitem{
        LVIF_PARAM,
        static_cast<int>(idx)};
      ListView_GetItem(list, &lvitem);
      const auto algorithm = reinterpret_cast<const LegacyHashAlgorithm*>(lvitem.lParam);
      _settings->algorithms[algorithm->Idx()].Set(check);
      return TRUE;
    } else if (pnmhdr->idFrom == IDC_PROJECT_LINK && (pnmhdr->code == NM_CLICK || pnmhdr->code == NM_RETURN)) {
      const auto pnmlink = (PNMLINK)pnmhdr;
      ShellExecuteW(
        nullptr,
        L"open",
        pnmlink->item.szUrl,
        nullptr,
        nullptr,
        SW_SHOW
      );
      return TRUE;
    }
    break;

  case WM_CTLCOLORSTATIC:
    for (size_t i = 0; i < std::size(HASH_COLOR_SETTING_MAP); ++i) {
      const auto& e = HASH_COLOR_SETTING_MAP[i];
      if ((HWND)lParam == _samples[i]) {
        auto hdc = (HDC)wParam;

        if (_settings->*e.fg_enabled)
          SetTextColor(hdc, _settings->*e.fg_color);
        else
          SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

        if (_settings->*e.bg_enabled)
          SetBkColor(hdc, _settings->*e.bg_color);
        else
          SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

        return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
      }
    }
    break;
  default:
    break;
  }
  return FALSE;
}
