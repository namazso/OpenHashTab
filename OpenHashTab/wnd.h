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
#pragma once
#include "utl.h"

namespace wnd
{
  // apparently you can get random WM_USER messages from malfunctioning other apps
  static constexpr auto k_user_magic_wparam = (WPARAM)0x1c725fcfdcbf5843;

  enum UserWindowMessages : UINT
  {
    WM_USER_FILE_FINISHED = WM_USER,
    WM_USER_ALL_FILES_FINISHED,
    WM_USER_FILE_PROGRESS
  };

  class WindowLayoutAdapter
  {
    struct ItemInfo
    {
      HWND hwnd;
      LONG original_x;
      LONG original_y;
      LONG original_w;
      LONG original_h;
      LONG scale_x;
      LONG scale_y;
      LONG scale_w;
      LONG scale_h;

      ItemInfo(HWND parent, HWND hwnd, const SHORT* ps)
      {
        this->hwnd = hwnd;
        RECT rect;
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(HWND_DESKTOP, parent, (LPPOINT)&rect, 2);
        original_x = rect.left;
        original_y = rect.top;
        original_w = rect.right - rect.left;
        original_h = rect.bottom - rect.top;
        scale_x = *ps++;
        scale_y = *ps++;
        scale_w = *ps++;
        scale_h = *ps++;
      }

      void Adjust(HDWP hdwp, LONG delta_x, LONG delta_y) const
      {
        // some elements get angry at negative values
        const auto x = std::max(0l, original_x + delta_x * scale_x / 100);
        const auto y = std::max(0l, original_y + delta_y * scale_y / 100);
        const auto w = std::max(0l, original_w + delta_x * scale_w / 100);
        const auto h = std::max(0l, original_h + delta_y * scale_h / 100);
        const auto flags = SWP_NOZORDER | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOCOPYBITS;
        DeferWindowPos(hdwp, hwnd, HWND_TOP, x, y, w, h, flags);
      }
    };

    std::vector<ItemInfo> _info;
    HWND _hwnd;
    LONG original_w;
    LONG original_h;

  public:
    WindowLayoutAdapter(HWND parent, int resid)
    {
      _hwnd = parent;
      RECT rect;
      GetClientRect(parent, &rect);
      original_w = rect.right - rect.left;
      original_h = rect.bottom - rect.top;
      const auto layout = utl::GetResource(MAKEINTRESOURCEW(resid), L"AFX_DIALOG_LAYOUT");
      const auto* pw = (PCSHORT)layout.first;
      const auto* end = (PCSHORT)(layout.first + layout.second);
      if (*pw++ != 0)
        return;
      for (auto wnd = GetWindow(parent, GW_CHILD); wnd && (pw + 4) <= end; wnd = GetWindow(wnd, GW_HWNDNEXT), pw += 4)
        _info.emplace_back(parent, wnd, pw);
    }

    void Adjust() const
    {
      RECT rect;
      GetClientRect(_hwnd, &rect);
      const auto new_w = rect.right - rect.left;
      const auto new_h = rect.bottom - rect.top;
      const auto delta_x = new_w - original_w;
      const auto delta_y = new_h - original_h;
      HDWP hdwp = BeginDeferWindowPos(_info.size());
      for (const auto& it : _info)
        it.Adjust(hdwp, delta_x, delta_y);
      EndDeferWindowPos(hdwp);
    }
  };
}
