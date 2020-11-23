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

#ifdef _DEBUG
inline void DebugMsg(PCSTR fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  char text[4096];
  vsprintf_s(text, fmt, args);
  va_end(args);
  OutputDebugStringA(text);
}
#else
inline void DebugMsg(PCSTR fmt, ...) { }
#endif

namespace utl
{
  inline HINSTANCE GetInstance() { return (HINSTANCE)&__ImageBase; }

  // T should be a class handling a dialog, having implemented these:
  //   T(HWND hDlg, void* user_param)
  //     hDlg: the HWND of the dialog, guaranteed to be valid for the lifetime of the object
  //     user_param: parameter passed to the function creating the dialog
  //   INT_PTR DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
  template <typename T>
  INT_PTR CALLBACK DlgProcClassBinder(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    T* p;
    if (uMsg == WM_INITDIALOG)
    {
      p = new T(hDlg, (void*)lParam);
      SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)p);
    }
    else
    {
      p = (T*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
    }
    // there are some unimportant messages sent before WM_INITDIALOG
    const INT_PTR ret = p ? p->DlgProc(uMsg, wParam, lParam) : (INT_PTR)FALSE;
    if (uMsg == WM_NCDESTROY)
    {
      delete p;
      // even if we were to somehow receive messages after WM_NCDESTROY make sure we dont call invalid ptr
      SetWindowLongPtrW(hDlg, GWLP_USERDATA, 0);
    }
    return ret;
  }

  namespace detail
  {
    template <typename Dialog>
    INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      if (uMsg == WM_INITDIALOG)
        lParam = ((LPPROPSHEETPAGEW)lParam)->lParam;
      return DlgProcClassBinder<Dialog>(hDlg, uMsg, wParam, lParam);
    };

    template <typename PropPage>
    UINT CALLBACK Callback(HWND hwnd, UINT msg, LPPROPSHEETPAGEW ppsp)
    {
      const auto object = (PropPage*)ppsp->lParam;
      UINT ret = 1;

      static const char* const msgname[] = { "ADDREF", "RELEASE", "CREATE" };
      DebugMsg("%s %p object %p ppsp %p\n", msgname[msg], hwnd, object, ppsp->lParam);

      switch (msg)
      {
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
  }

  // PropPage should have the following functions:
  //   PropPage(args...)
  //   AddRef(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Release(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Create(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //
  // Dialog should have the functions described in DlgProcClassBinder. Additionally, dialog receives a Coordinator*
  //   as lParam in it's constructor. A dialog may or may not get created for a property sheet during lifetime.
  template <typename PropPage, typename Dialog, typename... Args>
  HPROPSHEETPAGE MakePropPage(PROPSHEETPAGEW psp, Args&&... args)
  {
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

  template <typename Char>
  Char hex(std::uint8_t n)
  {
    if (n < 0xA)
      return Char('0') + n;
    return Char('A') + (n - 0xA);
  };

  template <typename Char>
  std::uint8_t unhex(Char ch)
  {
    if (ch >= 0x80 || ch <= 0)
      return 0xFF;

    const auto c = (char)ch;

#define TEST_RANGE(c, a, b, offset) if (uint8_t(c) >= uint8_t(a) && uint8_t(c) <= uint8_t(b))\
  return uint8_t(c) - uint8_t(a) + (offset)

    TEST_RANGE(c, '0', '9', 0x0);
    TEST_RANGE(c, 'a', 'f', 0xa);
    TEST_RANGE(c, 'A', 'F', 0xA);

#undef TEST_RANGE

    return 0xFF;
  };

  template <typename Char>
  void HashBytesToString(Char* str, const std::vector<std::uint8_t>& hash)
  {
    for (auto b : hash)
    {
      *str++ = utl::hex<Char>(b >> 4);
      *str++ = utl::hex<Char>(b & 0xF);
    }
    *str = Char(0);
  }

  template <typename Char>
  std::vector<std::uint8_t> HashStringToBytes(Char* str)
  {
    auto it = str;
    do
      if (const auto c = *it; !c || utl::unhex<Char>(c) != 0xFF)
        break;
    while (++it);

    std::vector<std::uint8_t> res;

    uint8_t byte = 0;
    for (auto i = 0; it[i]; ++i)
      if (const auto nibble = utl::unhex<Char>(it[i]); nibble == 0xFF)
        if (i % 2 == 0)
          break;
        else
          return {};
      else
        if (i % 2 == 0)
          byte = nibble << 4;
        else
          res.push_back(byte | nibble);

    return res;
  }

  inline int FormattedMessageBox(HWND hwnd, LPCWSTR caption, UINT type, LPCWSTR fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    wchar_t text[4096];
    vswprintf_s(text, fmt, args);
    va_end(args);
    return MessageBoxW(hwnd, text, caption, type);
  }

  inline std::wstring GetString(UINT uID)
  {
    LPCWSTR v = nullptr;
    const auto len = LoadStringW(utl::GetInstance(), uID, (LPWSTR)&v, 0);
    return {v, v + len};
  }

  inline std::wstring GetWindowTextString(HWND hwnd)
  {
    SetLastError(0);
    // GetWindowTextLength may return more than actual length, so we can't use a std::wstring directly
    const auto len = GetWindowTextLengthW(hwnd);
    // if text is 0 long, GetWindowTextLength returns 0, same as when error happened
    if (len == 0 && GetLastError() != 0)
      return {};
    const auto p = std::make_unique<wchar_t[]>(len + 1);
    GetWindowTextW(hwnd, p.get(), len + 1);
    return { p.get() };
  }

  bool AreFilesTheSame(HANDLE a, HANDLE b);

  std::wstring MakePathLongCompatible(const std::wstring& file);

  std::wstring CanonicalizePath(const std::wstring& path);

  HANDLE OpenForRead(const std::wstring& file, bool async = false);

  DWORD SetClipboardText(HWND hwnd, LPCWSTR text);

  std::wstring GetClipboardText(HWND hwnd);

  std::wstring SaveDialog(HWND hwnd, LPCWSTR defpath, LPCWSTR defname);

  DWORD SaveMemoryAsFile(LPCWSTR path, const void* p, size_t size);

  std::wstring UTF8ToTString(const char* p);
  std::string TStringToUTF8(LPCWSTR p);

  std::wstring ErrorToString(DWORD error);

  std::pair<const char*, size_t> GetResource(LPCWSTR name, LPCWSTR type);
}

#define MAKE_IDC_MEMBER(hwnd, name) HWND _hwnd_ ## name = GetDlgItem(hwnd, IDC_ ## name)
