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

#include "SettingsDialog.h"
#include "Settings.h"
#include "utl.h"

INT_PTR SettingsDialog::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const auto pnmhdr = (LPNMHDR)lParam;
  switch (uMsg)
  {
  case WM_INITDIALOG:
  {
    if (IsWindows8OrGreater())
      SetWindowTextW(_hwnd, L"\u2699");

    const auto list = GetDlgItem(_hwnd, IDC_ALGORITHM_LIST);
    ListView_SetExtendedListViewStyleEx(list, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    for (auto& algorithm : HashAlgorithm::g_hashers)
    {
      const auto name = utl::UTF8ToTString(algorithm.GetName());
      LVITEMW lvitem
      {
        LVIF_PARAM,
        INT_MAX,
        0,
        0,
        0,
        (LPWSTR)L""
      };
      lvitem.lParam = (LPARAM)&algorithm;
      const auto item = ListView_InsertItem(list, &lvitem);
      ListView_SetItemText(list, item, 0, (LPWSTR)name.c_str());
      ListView_SetCheckState(list, item, Settings::instance.IsHashEnabled(&algorithm));
    }
    _done_setup = true;
    return FALSE; // do not select default control
  }

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(_hwnd, LOWORD(wParam));
      return TRUE;
    }
    break;

  case WM_NOTIFY:
    if(pnmhdr->idFrom == IDC_ALGORITHM_LIST && pnmhdr->code == LVN_ITEMCHANGED && _done_setup)
    {
      const auto pnmlv = (LPNMLISTVIEW)lParam;
      const auto idx = pnmlv->iItem;
      const auto list = pnmhdr->hwndFrom;
      const auto check = (bool)ListView_GetCheckState(list, idx);
      LVITEMW lvitem
      {
        LVIF_PARAM,
        (int)idx
      };
      ListView_GetItem(list, &lvitem);
      const auto algorithm = (const HashAlgorithm*)lvitem.lParam;
      Settings::instance.SetHashEnabled(algorithm, check);
      return TRUE;
    }
  }
  return FALSE;
}
