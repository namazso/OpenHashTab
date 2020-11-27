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

#include "utl.h"

bool utl::AreFilesTheSame(HANDLE a, HANDLE b)
{
  if (IsWindows8OrGreater())
  {
    if (const auto kernel32 = GetModuleHandleW(L"kernel32"))
    {
      using fn_t = decltype(GetFileInformationByHandleEx);
      if (const auto pfn = reinterpret_cast<fn_t*>(GetProcAddress(kernel32, "GetFileInformationByHandleEx")))
      {
        struct xFILE_ID_INFO {
          ULONGLONG VolumeSerialNumber;
          FILE_ID_128 FileId;
        };
        constexpr static auto FileIdInfo = static_cast<FILE_INFO_BY_HANDLE_CLASS>(18);

        xFILE_ID_INFO fiia{}, fiib{};

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

std::wstring utl::MakePathLongCompatible(std::wstring file)
{
  constexpr static wchar_t prefix[] = L"\\\\";
  constexpr static auto prefixlen = std::size(prefix) - 1;
  const auto file_cstr = file.c_str();
  if (file.size() < prefixlen || 0 != wcsncmp(file_cstr, prefix, prefixlen))
    file.insert(0, L"\\\\?\\");
  return file;
}

std::wstring utl::CanonicalizePath(const std::wstring& path)
{
  // PathCanonicalize doesn't support long paths, and pathcch.h isn't backward compatible, PathCch*
  // functions are only available on Windows 8+, so since I really don't feel like reimplementing it
  // myself long paths are only supported on Windows 8+

  using tPathAllocCanonicalize = decltype(&PathAllocCanonicalize);
  static const auto pPathAllocCanonicalize = []
  {
    if (const auto kernelbase = GetModuleHandleW(L"kernelbase"))
      if (const auto fn = GetProcAddress(kernelbase, "PathAllocCanonicalize"))
        return reinterpret_cast<tPathAllocCanonicalize>(fn);
    return static_cast<tPathAllocCanonicalize>(nullptr);
  } ();

  if (pPathAllocCanonicalize)
  {
    PWSTR outpath;
    const auto ret = pPathAllocCanonicalize(
      path.c_str(),
      PATHCCH_ALLOW_LONG_PATHS | PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS,
      &outpath
    );
    if (ret == S_OK)
    {
      auto result = std::wstring{ outpath };
      LocalFree(outpath);
      return result;
    }

    // fall through if PathAllocCanonicalize didn't work out
  }


  wchar_t canonical[MAX_PATH + 1];
  if(PathCanonicalizeW(canonical, path.c_str()))
    return { canonical };
  return {};
}

HANDLE utl::OpenForRead(const std::wstring& file, bool async)
{
  return CreateFileW(
    MakePathLongCompatible(file).c_str(),
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | (async ? FILE_FLAG_OVERLAPPED : 0),
    nullptr
  );
}

DWORD utl::SetClipboardText(HWND hwnd, LPCWSTR text)
{
  DWORD error;

  if (OpenClipboard(hwnd))
  {
    EmptyClipboard();

    const auto len = wcslen(text);

    if (const auto cb = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t)))
    {
      // Lock the handle and copy the text to the buffer. 
      if (const auto lcb = static_cast<LPWSTR>(GlobalLock(cb)))
      {
        memcpy(lcb, text, (len + 1) * sizeof(wchar_t));
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

std::wstring utl::GetClipboardText(HWND hwnd)
{
  std::wstring wstr;
  if (OpenClipboard(hwnd))
  {
    const auto hglb = GetClipboardData(CF_UNICODETEXT);
    if(hglb)
    {
      const auto text = static_cast<LPCWSTR>(GlobalLock(hglb));

      if(text)
      {
        wstr = text;
        GlobalUnlock(hglb);
      }
    }

    CloseClipboard();
  }

  return wstr;
}

std::wstring utl::SaveDialog(HWND hwnd, LPCWSTR defpath, LPCWSTR defname)
{
  wchar_t name[PATHCCH_MAX_CCH];
  wcscpy_s(name, defname);

  OPENFILENAME of = { sizeof(OPENFILENAME), hwnd };
  of.lpstrFile = name;
  of.nMaxFile = static_cast<DWORD>(std::size(name));
  of.lpstrInitialDir = defpath;
  of.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
  if (!GetSaveFileNameW(&of))
  {
    const auto error = CommDlgExtendedError();
    // if error is 0 the user just cancelled the action
    if (error)
      utl::FormattedMessageBox(
        hwnd,
        L"Error",
        MB_ICONERROR | MB_OK,
        L"GetSaveFileName returned with error: %08X",
        error
      );
    return {};
  }
  // Compiler keeps crying about it even though it's impossible
  name[std::size(name) - 1] = 0;
  return { name };
}

DWORD utl::SaveMemoryAsFile(LPCWSTR path, const void* p, DWORD size)
{
  const auto h = CreateFileW(
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
  if (!WriteFile(h, p, size, &written, nullptr))
    error = GetLastError();

  CloseHandle(h);
  return error;
}

std::wstring utl::UTF8ToTString(const char* p)
{
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
}

std::string utl::TStringToUTF8(LPCWSTR p)
{
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
}

std::wstring utl::ErrorToString(DWORD error)
{
  wchar_t buf[0x1000];

  FormatMessageW(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,
    error,
    MAKELANGID(LANG_USER_DEFAULT, SUBLANG_DEFAULT),
    buf,
    static_cast<DWORD>(std::size(buf)),
    nullptr
  );
  std::wstring wstr{ buf };
  const auto pos = wstr.find_last_not_of(L"\r\n");
  if (pos != std::wstring::npos)
    wstr.resize(pos);
  return wstr;
}

std::pair<const char*, size_t> utl::GetResource(LPCWSTR name, LPCWSTR type)
{
  const auto rc = FindResourceW(
    GetInstance(),
    name,
    type
  );
  const auto rc_data = LoadResource(GetInstance(), rc);
  const auto size = SizeofResource(GetInstance(), rc);
  const auto data = static_cast<const char*>(LockResource(rc_data));
  return { data, size };
}
