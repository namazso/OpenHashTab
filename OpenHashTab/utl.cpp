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
#include "stdafx.h"

#include "utl.h"

bool utl::AreFilesTheSame(HANDLE a, HANDLE b)
{
  if (IsWindows8OrGreater())
  {
    if (const auto kernel32 = GetModuleHandle(_T("kernel32")))
    {
      using fn_t = decltype(GetFileInformationByHandleEx);
      if (const auto pfn = (fn_t*)GetProcAddress(kernel32, "GetFileInformationByHandleEx"))
      {
        typedef struct _FILE_ID_INFO {
          ULONGLONG VolumeSerialNumber;
          FILE_ID_128 FileId;
        } FILE_ID_INFO, * PFILE_ID_INFO;
        constexpr static auto FileIdInfo = (FILE_INFO_BY_HANDLE_CLASS)18;

        FILE_ID_INFO fiia, fiib;

        if(pfn(a, FileIdInfo, &fiia, sizeof(fiia)) && pfn(b, FileIdInfo, &fiib, sizeof(fiib)))
        {
          const auto& ida = fiia.FileId.Identifier;
          const auto& idb = fiib.FileId.Identifier;
          return fiia.VolumeSerialNumber == fiib.VolumeSerialNumber
            && std::equal(std::begin(ida), std::end(ida), std::begin(idb));
        }
      }
    }
  }

  BY_HANDLE_FILE_INFORMATION fia, fib;
  if (!GetFileInformationByHandle(a, &fia) || !GetFileInformationByHandle(b, &fib))
    return false;

  return fia.dwVolumeSerialNumber == fib.dwVolumeSerialNumber
    && fia.nFileIndexLow == fib.nFileIndexLow
    && fia.nFileIndexHigh == fib.nFileIndexHigh;
}

HANDLE utl::OpenForRead(tstring file, bool async)
{
  auto file_cstr = file.c_str();
#ifdef _UNICODE
  if (file_cstr[0] != '\\' || file_cstr[1] != '\\' || file_cstr[2] != '?' || file_cstr[3] != '\\')
    file.insert(0, _T("\\\\?\\"));
  file_cstr = file.c_str();
#endif
  return CreateFile(
    file_cstr,
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | (async ? FILE_FLAG_OVERLAPPED : 0),
    nullptr
  );
}

DWORD utl::SetClipboardText(HWND hwnd, LPCTSTR text)
{
  if (!hwnd)
    return ERROR_INVALID_PARAMETER;

  DWORD error = ERROR_SUCCESS;

  if (OpenClipboard(hwnd))
  {
    EmptyClipboard();

    const auto len = _tcslen(text);

    if (const auto cb = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(TCHAR)))
    {
      // Lock the handle and copy the text to the buffer. 
      if (const auto lcb = (LPTSTR)GlobalLock(cb))
      {
        memcpy(lcb, text, (len + 1) * sizeof(TCHAR));
        const auto ref = GlobalUnlock(cb);
        error          = GetLastError();
        if (ref != 0 || error == ERROR_SUCCESS)
        {
          // Place the handle on the clipboard.
          if (SetClipboardData(CF_UNICODETEXT, cb) == nullptr)
            error = GetLastError();
        }
      }
      else
        error = GetLastError();
    }
    else
      error = GetLastError();

    CloseClipboard();
  }
  else
    error = GetLastError();

  return error;
}

void utl::SaveMemoryAsFile(HWND hwnd, const void* p, size_t size, LPCTSTR defpath, LPCTSTR defname)
{
  TCHAR name[4097];
  _tcscpy_s(name, defname);

  OPENFILENAME of    = {sizeof(OPENFILENAME), hwnd};
  of.lpstrFile       = name;
  of.nMaxFile        = (DWORD)std::size(name);
  of.lpstrInitialDir = defpath;
  of.Flags           = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
  if (!GetSaveFileName(&of))
  {
    const auto error = CommDlgExtendedError();
    // if error is 0 the user just cancelled the action
    if (error)
      utl::FormattedMessageBox(
        hwnd,
        _T("Error"),
        MB_ICONERROR | MB_OK,
        _T("GetSaveFileName returned with error: %08X"),
        error
      );
    return;
  }

  const auto h = CreateFile(
    name,
    GENERIC_WRITE,
    0,
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  );

  if (h == INVALID_HANDLE_VALUE)
  {
    utl::FormattedMessageBox(
      hwnd,
      _T("Error"),
      MB_ICONERROR | MB_OK,
      _T("CreateFile returned with error: %08X"),
      GetLastError()
    );
    return;
  }

  DWORD written = 0;
  if (!WriteFile(h, p, (DWORD)size, &written, nullptr))
  {
    utl::FormattedMessageBox(
      hwnd,
      _T("Error"),
      MB_ICONERROR | MB_OK,
      _T("WriteFile returned with error: %08X"),
      GetLastError()
    );
  }

  CloseHandle(h);
}

tstring utl::UTF8ToTString(const char* p)
{
#ifdef _UNICODE
  const auto wsize = MultiByteToWideChar(
    CP_UTF8,
    0,
    p,
    -1,
    nullptr,
    0
  );

  std::wstring wstr;
  // size includes null
  wstr.resize(wsize - 1);

  MultiByteToWideChar(
    CP_UTF8,
    0,
    p,
    -1,
    wstr.data(),
    wsize
  );

  return wstr;
#else
  return { p };
#endif
}

std::string utl::TStringToUTF8(LPCTSTR p)
{
#ifdef _UNICODE
  const auto size = WideCharToMultiByte(
    CP_UTF8,
    0,
    p,
    -1,
    nullptr,
    0,
    nullptr,
    nullptr
  );

  std::string str;
  // size includes null
  str.resize(size - 1);

  WideCharToMultiByte(
    CP_UTF8,
    0,
    p,
    -1,
    str.data(),
    size,
    nullptr,
    nullptr
  );

  return str;
#else
  return { p };
#endif
}
