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

tstring utl::MakePathLongCompatible(const tstring& file)
{
#ifdef UNICODE
  constexpr static TCHAR prefix[] = _T("\\\\?\\");
  constexpr static auto prefixlen = std::size(prefix) - 1;
  const auto file_cstr = file.c_str();
  if (file.size() < prefixlen || 0 != _tcsncmp(file_cstr, prefix, prefixlen))
    return tstring{ prefix } + file;
#endif
  return file;
}

tstring utl::CanonicalizePath(const tstring& path)
{
  // PathCanonicalize doesn't support long paths, and pathcch.h isn't backward compatible, PathCch*
  // functions are only available on Windows 8+, so since I really don't feel like reimplementing it
  // myself long paths are only supported on Windows 8+

#ifdef UNICODE

  using tPathAllocCanonicalize = decltype(&PathAllocCanonicalize);
  static const auto pPathAllocCanonicalize = []
  {
    if (const auto kernelbase = GetModuleHandle(_T("kernelbase")))
      if (const auto fn = GetProcAddress(kernelbase, "PathAllocCanonicalize"))
        return (tPathAllocCanonicalize)fn;
    return (tPathAllocCanonicalize)nullptr;
  } ();

  if (pPathAllocCanonicalize)
  {
    PWSTR outpath;
    const auto ret = pPathAllocCanonicalize(
      (PCWSTR)path.c_str(), // cast needed for non-UNICODE, where this will never run
      PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS,
      &outpath
    );
    if (ret == S_OK)
    {
      const auto result = tstring{ (PCTSTR)outpath };
      LocalFree(outpath);
      return result;
    }

    // fall through if PathAllocCanonicalize didn't work out
  }

#endif

  TCHAR canonical[MAX_PATH + 1];
  if(PathCanonicalize(canonical, path.c_str()))
    return { canonical };
  return {};
}

HANDLE utl::OpenForRead(const tstring& file, bool async)
{
  return CreateFile(
    MakePathLongCompatible(file).c_str(),
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

tstring utl::GetClipboardText(HWND hwnd)
{
  tstring tstr;
  if (OpenClipboard(hwnd))
  {
    const auto hglb = GetClipboardData(CF_UNICODETEXT);
    if(hglb)
    {
      const auto text = (PCTSTR)GlobalLock(hglb);

      if(text)
      {
        tstr = text;
        GlobalUnlock(hglb);
      }
    }

    CloseClipboard();
  }

  return tstr;
}

tstring utl::SaveDialog(HWND hwnd, LPCTSTR defpath, LPCTSTR defname)
{
  TCHAR name[PATHCCH_MAX_CCH];
  _tcscpy_s(name, defname);

  OPENFILENAME of = { sizeof(OPENFILENAME), hwnd };
  of.lpstrFile = name;
  of.nMaxFile = (DWORD)std::size(name);
  of.lpstrInitialDir = defpath;
  of.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
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
    return {};
  }
  return { name };
}

DWORD utl::SaveMemoryAsFile(LPCTSTR path, const void* p, size_t size)
{
  const auto h = CreateFile(
    MakePathLongCompatible(path).c_str(),
    GENERIC_WRITE,
    0,
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
  );

  DWORD error = ERROR_SUCCESS;

  if (h == INVALID_HANDLE_VALUE)
  {
    error = GetLastError();
    return error;
  }

  DWORD written = 0;
  if (!WriteFile(h, p, (DWORD)size, &written, nullptr))
    error = GetLastError();

  CloseHandle(h);
  return error;
}

tstring utl::UTF8ToTString(const char* p)
{
#ifdef UNICODE
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
#ifdef UNICODE
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
