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

#include "SettingsDialog.h"

#include "Settings.h"
#include "utl.h"

#include <stdexcept>

struct SettingCheckbox
{
  RegistrySetting<bool> Settings::* setting;
  int control_id;
  int string_id;
};

#define CTLSTR(name) IDC_CHECK_ ## name, IDS_ ## name

static constexpr SettingCheckbox s_boxes[] =
{
  { &Settings::display_uppercase,               CTLSTR(DISPLAY_UPPERCASE              ) },
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

#undef CTLSTR

void SettingsDialog::UpdateCheckboxAvailability()
{
  const auto banner = BST_CHECKED == Button_GetCheck(_hwnd_CHECK_SUMFILE_BANNER);
  Button_Enable(_hwnd_CHECK_SUMFILE_BANNER_DATE, banner);

  const auto clipboard = BST_CHECKED == Button_GetCheck(_hwnd_CHECK_CLIPBOARD_AUTOENABLE);
  Button_Enable(_hwnd_CHECK_CLIPBOARD_AUTOENABLE_IF_NONE, clipboard);
  Button_Enable(_hwnd_CHECK_CLIPBOARD_AUTOENABLE_EXCLUSIVE, clipboard);
}

INT_PTR SettingsDialog::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const auto pnmhdr = (LPNMHDR)lParam;
  switch (uMsg)
  {
  case WM_INITDIALOG:
  {
    utl::SetWindowTextStringFromTable(_hwnd, IDS_SETTINGS_TITLE);
    utl::SetWindowTextStringFromTable(_hwnd_BUTTON_CHECK_FOR_UPDATES, IDS_CHECK_FOR_UPDATES);
    
    utl::SetFontForChildren(_hwnd, _font.get());

    const auto list = _hwnd_ALGORITHM_LIST;
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    auto width = 0;
    for (const auto& algorithm : HashAlgorithm::Algorithms())
    {
      const auto name = utl::UTF8ToWide(algorithm.GetName());
      width = std::max(width, ListView_GetStringWidth(list, name.c_str()));
      LVITEMW lvitem
      {
        LVIF_PARAM,
        INT_MAX,
        0,
        0,
        0,
        const_cast<LPWSTR>(L"")
      };
      lvitem.lParam = reinterpret_cast<LPARAM>(&algorithm);
      const auto item = ListView_InsertItem(list, &lvitem);
      ListView_SetItemText(list, item, 0, const_cast<LPWSTR>(name.c_str()));
      ListView_SetCheckState(list, item, _settings->algorithms[algorithm.Idx()]);
    }

    // let's hope 40 px padding is good enough
    ListView_SetColumnWidth(list, 0, width + utl::GetDPIScaledPixels(list, 40));

    for (const auto& ctl : s_boxes)
    {
      const auto ctl_hwnd = GetDlgItem(_hwnd, ctl.control_id);
      Button_SetCheck(ctl_hwnd, _settings->*ctl.setting);
      SetWindowTextW(ctl_hwnd, utl::GetString(ctl.string_id).c_str());
    }

    UpdateCheckboxAvailability();

    _done_setup = true;
    return FALSE; // do not select default control
  }

  case WM_COMMAND:
  {
    const auto id = LOWORD(wParam);
    const auto code = HIWORD(wParam);
    if (code == BN_CLICKED && (id == IDOK || id == IDCANCEL))
    {
      EndDialog(_hwnd, LOWORD(wParam));
      return TRUE;
    }
    if (code == BN_CLICKED && id == IDC_BUTTON_CHECK_FOR_UPDATES)
    {
      try
      {
        constexpr static auto current = utl::Version{ CI_VERSION_MAJOR, CI_VERSION_MINOR, CI_VERSION_PATCH };
        if constexpr (current.AsNumber() == 0)
        {
          MessageBoxW(
            _hwnd,
            utl::GetString(IDS_UPDATE_DISABLED_IN_DEV_BUILD).c_str(),
            utl::GetString(IDS_ERROR).c_str(),
            MB_OK | MB_ICONERROR
          );
        }
        else if (const auto v = utl::GetLatestVersion(); v > current)
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

      }
      catch (const std::runtime_error& e)
      {
        MessageBoxW(
          _hwnd,
          utl::UTF8ToWide(e.what()).c_str(),
          L"Runtime error",
          MB_ICONERROR | MB_OK
        );
      }
      break;
    }
    if (code == BN_CLICKED)
    {
      for (const auto& ctl : s_boxes)
        if (id == ctl.control_id)
          (_settings->*ctl.setting).Set(BST_CHECKED == Button_GetCheck(GetDlgItem(_hwnd, ctl.control_id)));

      UpdateCheckboxAvailability();
    }
    break;
  }

  case WM_NOTIFY:
    if(pnmhdr->idFrom == IDC_ALGORITHM_LIST && pnmhdr->code == LVN_ITEMCHANGED && _done_setup)
    {
      const auto pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      const auto idx = pnmlv->iItem;
      const auto list = pnmhdr->hwndFrom;
      const auto check = static_cast<bool>(ListView_GetCheckState(list, idx));
      LVITEMW lvitem
      {
        LVIF_PARAM,
        static_cast<int>(idx)
      };
      ListView_GetItem(list, &lvitem);
      const auto algorithm = reinterpret_cast<const HashAlgorithm*>(lvitem.lParam);
      _settings->algorithms[algorithm->Idx()].Set(check);
      return TRUE;
    }
  }
  return FALSE;
}
