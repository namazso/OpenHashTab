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
#include "utl.h"

namespace wnd {
  // apparently you can get random WM_USER messages from malfunctioning other apps
  static constexpr auto k_user_magic_wparam = (WPARAM)0x1c725fcfdcbf5843;

  enum UserWindowMessages : UINT {
    WM_USER_ALL_FILES_FINISHED = WM_USER,
    WM_USER_FILE_PROGRESS
  };

#define MAKE_IDC_MEMBER(hwnd, name) HWND _hwnd_##name = GetDlgItem(hwnd, IDC_##name)

  class WindowLayoutAdapter {
    struct ItemInfo {
      HWND hwnd;
      LONG original_x;
      LONG original_y;
      LONG original_w;
      LONG original_h;
      LONG scale_x;
      LONG scale_y;
      LONG scale_w;
      LONG scale_h;

      ItemInfo(HWND parent, HWND hwnd, const SHORT* ps) {
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

      void Adjust(HDWP hdwp, LONG delta_x, LONG delta_y) const {
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
    WindowLayoutAdapter(HWND parent, int resid) {
      _hwnd = parent;
      RECT rect;
      GetClientRect(parent, &rect);
      original_w = rect.right - rect.left;
      original_h = rect.bottom - rect.top;
      const auto layout = utl::GetResource(MAKEINTRESOURCEW(resid), L"AFX_DIALOG_LAYOUT");
      const auto* pw = (const SHORT*)layout.first;
      const auto* end = (const SHORT*)(layout.first + layout.second);
      if (*pw++ != 0)
        return;
      for (auto wnd = GetWindow(parent, GW_CHILD); wnd && (pw + 4) <= end; wnd = GetWindow(wnd, GW_HWNDNEXT), pw += 4)
        _info.emplace_back(parent, wnd, pw);
    }

    void Adjust() const {
      RECT rect;
      GetClientRect(_hwnd, &rect);
      const auto new_w = rect.right - rect.left;
      const auto new_h = rect.bottom - rect.top;
      const auto delta_x = new_w - original_w;
      const auto delta_y = new_h - original_h;
      const auto hdwp = BeginDeferWindowPos(_info.size());
      for (const auto& it : _info)
        it.Adjust(hdwp, delta_x, delta_y);
      EndDeferWindowPos(hdwp);
    }
  };

  // T should be a class handling a dialog, having implemented these:
  //   T(HWND hDlg, void* user_param)
  //     hDlg: the HWND of the dialog, guaranteed to be valid for the lifetime of the object
  //     user_param: parameter passed to the function creating the dialog
  //   INT_PTR DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
  template <typename T>
  INT_PTR CALLBACK DlgProcClassBinder(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    T* p;
    if (uMsg == WM_INITDIALOG) {
      p = new T(hDlg, (void*)lParam);
      SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)p);
    } else {
      p = (T*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
    }
    // there are some unimportant messages sent before WM_INITDIALOG
    const INT_PTR ret = p ? p->DlgProc(uMsg, wParam, lParam) : (INT_PTR)FALSE;
    if (uMsg == WM_NCDESTROY) {
      delete p;
      // even if we were to somehow receive messages after WM_NCDESTROY make sure we dont call invalid ptr
      SetWindowLongPtrW(hDlg, GWLP_USERDATA, 0);
    }
    return ret;
  }

  namespace detail {
    template <typename Dialog>
    INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      if (uMsg == WM_INITDIALOG)
        lParam = ((LPPROPSHEETPAGEW)lParam)->lParam;
      return DlgProcClassBinder<Dialog>(hDlg, uMsg, wParam, lParam);
    };

    template <typename PropPage>
    UINT CALLBACK Callback(HWND hwnd, UINT msg, LPPROPSHEETPAGEW ppsp) {
      const auto object = (PropPage*)ppsp->lParam;
      UINT ret = 1;

      static const char* const msgname[] = {"ADDREF", "RELEASE", "CREATE"};
      DebugMsg("%s %p object %p ppsp %p\n", msgname[msg], hwnd, object, ppsp->lParam);

      switch (msg) {
      case PSPCB_ADDREF:
        object->AddRef(hwnd, ppsp);
        break;
      case PSPCB_RELEASE:
        object->Release(hwnd, ppsp);
        break;
      case PSPCB_CREATE:
        ret = object->Create(hwnd, ppsp);
        break;
      default:
        break;
      }

      return ret;
    };
  } // namespace detail

  // PropPage should have the following functions:
  //   PropPage(args...)
  //   AddRef(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Release(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Create(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //
  // Dialog should have the functions described in DlgProcClassBinder. Additionally, dialog receives a Coordinator*
  //   as lParam in it's constructor. A dialog may or may not get created for a property sheet during lifetime.
  template <typename PropPage, typename Dialog, typename... Args>
  HPROPSHEETPAGE MakePropPage(PROPSHEETPAGEW psp, Args&&... args) {
    // Things are generally called in the following order:
    // name           dlg   when
    // -----------------------------------------------
    // ADDREF         no    on opening properties
    // CREATE         no    on opening properties
    // *WM_INITDIALOG yes   on first click on sheet
    // *WM_*          yes   window messages
    // *WM_NCDESTROY  yes   on closing properties
    // RELEASE        no    after properties closed
    //
    // Ones marked with * won't get called when the user never selects our prop sheet page

    const auto object = new PropPage(std::forward<Args>(args)...);

    psp.pfnDlgProc = &detail::DlgProc<Dialog>;
    psp.pfnCallback = &detail::Callback<PropPage>;

    psp.lParam = (LPARAM)object;

    const auto page = CreatePropertySheetPageW(&psp);

    if (!page)
      delete object;

    return page;
  }

  enum Match : UINT {
    Match_all = 0,
    Match_l = 1 << 0,
    Match_w = 1 << 1,
    Match_wl = 1 << 2,
    Match_wh = 1 << 3,

    Match_wlh = Match_wl | Match_wh,
    Match_lw = Match_l | Match_w,
    Match_lwl = Match_l | Match_wl,
    Match_lwh = Match_l | Match_wh,
    Match_lwlh = Match_l | Match_wlh
  };

  template <typename T>
  class MessageMatcher {
    using fn_t = INT_PTR (T::*)(UINT message, WPARAM wparam, LPARAM lparam);

    fn_t _fn;
    LPARAM _lparam;
    WPARAM _wparam;
    UINT _message;
    UINT _conditions;

  public:
    constexpr MessageMatcher(fn_t fn, UINT message, UINT conditions = 0, WPARAM wparam = 0, LPARAM lparam = 0)
        : _fn(fn)
        , _lparam(lparam)
        , _wparam(wparam)
        , _message(message)
        , _conditions(conditions) {}

    bool TryRoute(T* c, UINT message, WPARAM wparam, LPARAM lparam, INT_PTR& ret) const {
      if (message != _message)
        return false;
      if (message == WM_NOTIFY)
        wparam = reinterpret_cast<LPNMHDR>(lparam)->idFrom; // this is stupid, but WM_NOTIFY docs say this wParam cannot be trusted
      if (_conditions & Match_l && lparam != _lparam)
        return false;
      if (_conditions & Match_wl && LOWORD(wparam) != LOWORD(_wparam))
        return false;
      if (_conditions & Match_wh && HIWORD(wparam) != HIWORD(_wparam))
        return false;
      if (_conditions & Match_w && wparam != _wparam) // on 64 bit lo+hi doesnt mean exact match
        return false;
      ret = (c->*_fn)(message, wparam, lparam);
      return true;
    }
  };

  template <typename T, typename It>
  FORCEINLINE bool RouteMessage(T* c, It begin, It end, UINT message, WPARAM wparam, LPARAM lparam, INT_PTR& ret) {
    // this with the if ladders might look like terrible performance, but in reality as long as the table is constexpr
    // the loop will unroll and the compiler will do compiler magic, to get us to a roughly log2(n) decision tree. Be
    // sure to thank your compiler for it's hard work!

#pragma unroll
    for (; begin != end; ++begin)
      if (begin->TryRoute(c, message, wparam, lparam, ret))
        return true;
    return false;
  }

  inline HWND CreateDialogFromChildDialogResourceParam(
    _In_opt_ HINSTANCE hInstance,
    _In_ LPCWSTR lpTemplateName,
    _In_opt_ HWND hWndParent,
    _In_opt_ DLGPROC lpDialogFunc,
    _In_ LPARAM dwInitParam
  ) {
    typedef struct {
      WORD dlgVer;
      WORD signature;
      DWORD helpID;
      DWORD exStyle;
      DWORD style;
      WORD cDlgItems;
      short x;
      short y;
      short cx;
      short cy;
      //sz_Or_Ord menu;
      //sz_Or_Ord windowClass;
      //WCHAR     title[titleLen];
      //WORD      pointsize;
      //WORD      weight;
      //BYTE      italic;
      //BYTE      charset;
      //WCHAR     typeface[stringLen];
    } DLGTEMPLATEEX;

    HWND hwnd = nullptr;
    const auto hRsrc = FindResourceExW(hInstance, RT_DIALOG, lpTemplateName, 0);
    if (hRsrc) {
      const auto hGlobal = LoadResource(hInstance, hRsrc);
      if (hGlobal) {
        const auto pOrigTemplate = static_cast<const DLGTEMPLATEEX*>(LockResource(hGlobal));
        if (pOrigTemplate) {
          const auto dwSize = SizeofResource(hInstance, hRsrc);
          if (dwSize) {
            const auto pTemplate = static_cast<DLGTEMPLATE*>(malloc(dwSize));
            const auto pTemplateEx = reinterpret_cast<DLGTEMPLATEEX*>(pTemplate);
            if (pTemplateEx) {
              memcpy(pTemplateEx, pOrigTemplate, dwSize);

              PDWORD pStyle, pexStyle;

              if (pTemplateEx->signature == 0xFFFF) {
                pStyle = &pTemplateEx->style;
                pexStyle = &pTemplateEx->exStyle;
              } else {
                pStyle = &pTemplate->style;
                pexStyle = &pTemplate->dwExtendedStyle;
              }

              *pStyle = WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | DS_SHELLFONT;
              *pexStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;

              hwnd = CreateDialogIndirectParamW(hInstance, pTemplate, hWndParent, lpDialogFunc, dwInitParam);

              free(pTemplateEx);
            }
          }
        }
        FreeResource(hGlobal);
      }
    }
    return hwnd;
  }

} // namespace wnd
