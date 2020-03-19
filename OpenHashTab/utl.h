//    Copyright 2019 namazso <admin@namazso.eu>
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
      SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)p);
    }
    else
    {
      p = (T*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    }
    // there are some unimportant messages sent before WM_INITDIALOG
    const INT_PTR ret = p ? p->DlgProc(uMsg, wParam, lParam) : (INT_PTR)FALSE;
    if (uMsg == WM_NCDESTROY)
    {
      delete p;
      // even if we were to somehow receive messages after WM_NCDESTROY make sure we dont call invalid ptr
      SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
    }
    return ret;
  }

  // PropPage should have the following functions:
  //   PropPage(args...)
  //   AddRef(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Release(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //   Create(HWND hwnd, LPPROPSHEETPAGE ppsp);
  //
  // Dialog should have the functions described in DlgProcClassBinder. Additionally, dialog receives a PropPage*
  //   as lParam in it's constructor. A dialog may or may not get created for a property sheet during lifetime.
  template <typename PropPage, typename Dialog, typename... Args>
  HPROPSHEETPAGE MakePropPage(PROPSHEETPAGE psp, Args&&... args)
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

    psp.pfnDlgProc = [](HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) -> INT_PTR
    {
      if (uMsg == WM_INITDIALOG)
        lParam = ((LPPROPSHEETPAGE)lParam)->lParam;
      return DlgProcClassBinder<Dialog>(hDlg, uMsg, wParam, lParam);
    };
    psp.pfnCallback = [](HWND hwnd, UINT msg, LPPROPSHEETPAGE ppsp) -> UINT
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

    psp.lParam = (LPARAM)object;

    const auto page = CreatePropertySheetPage(&psp);

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

  inline int FormattedMessageBox(HWND hwnd, LPCTSTR caption, UINT type, LPCTSTR fmt, ...)
  {
    va_list args;
    va_start(args, fmt);
    TCHAR text[4096];
    _vstprintf_s(text, fmt, args);
    va_end(args);
    return MessageBox(hwnd, text, caption, type);
  }

  inline tstring GetString(UINT uID)
  {
    PCTSTR v = nullptr;
    const auto len = LoadString((HINSTANCE)&__ImageBase, uID, (LPTSTR)&v, 0);
    return {v, v + len};
  }

  bool AreFilesTheSame(HANDLE a, HANDLE b);

  tstring MakePathLongCompatible(const tstring& file);

  tstring CanonicalizePath(const tstring& path);

  HANDLE OpenForRead(const tstring& file, bool async = false);

  DWORD SetClipboardText(HWND hwnd, LPCTSTR text);

  tstring GetClipboardText(HWND hwnd);

  tstring SaveDialog(HWND hwnd, LPCTSTR defpath, LPCTSTR defname);

  DWORD SaveMemoryAsFile(LPCTSTR path, const void* p, size_t size);

  tstring UTF8ToTString(const char* p);
  std::string TStringToUTF8(LPCTSTR p);

  tstring ErrorToString(DWORD error);
}
