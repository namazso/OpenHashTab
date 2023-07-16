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

#ifdef _DEBUG
inline void DebugMsg(PCSTR fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char text[4096];
  vsprintf_s(text, fmt, args);
  va_end(args);
  OutputDebugStringA(text);
}
#else
inline void DebugMsg(PCSTR fmt, ...) {
  UNREFERENCED_PARAMETER(fmt);
}
#endif

namespace utl {
  inline HINSTANCE GetInstance() {
    const auto base = __builtin_assume_aligned((const char*)&__ImageBase, 0x1000);
    return (HINSTANCE)base;
  }

  template <typename Char>
  Char hex(uint8_t n, bool upper = true) {
    if (n < 0xA)
      return (Char)((unsigned)'0' + (unsigned)n);
    return (Char)((unsigned)(upper ? 'A' : 'a') + ((unsigned)n - 0xA));
  }

  template <typename Char>
  uint8_t unhex(Char ch) {
    if ((unsigned)ch >= 0x80)
      return 0xFF;

    const auto c = (char)(uint8_t)(unsigned)ch;

#define TEST_RANGE(c, a, b, offset)                         \
  if (uint8_t(c) >= uint8_t(a) && uint8_t(c) <= uint8_t(b)) \
  return uint8_t(c) - uint8_t(a) + (offset)

    TEST_RANGE(c, '0', '9', 0x0);
    TEST_RANGE(c, 'a', 'f', 0xa);
    TEST_RANGE(c, 'A', 'F', 0xA);

#undef TEST_RANGE

    return 0xFF;
  }

  template <typename Char>
  void HashBytesToString(Char* str, const std::vector<uint8_t>& hash, bool upper = true) {
    for (auto b : hash) {
      *str++ = utl::hex<Char>(b >> 4, upper);
      *str++ = utl::hex<Char>(b & 0xF, upper);
    }
    *str = Char(0);
  }

  template <typename Char>
  std::vector<uint8_t> HashStringToBytes(std::basic_string_view<Char> str) {
    std::vector<uint8_t> res;

    for (size_t i = 0u; i < str.size() - 1; i += 2) {
      while (str[i] == ' ') {
        i++;
        if (i >= str.size() - 1)
          break;
      }
      const auto a = utl::unhex<Char>(str[i]);
      const auto b = utl::unhex<Char>(str[i + 1]);
      if (a == 0xFF || b == 0xFF)
        return {}; // invalid
      res.push_back((uint8_t)((unsigned)a << 4 | b));
    }
    return res;
  }

  std::vector<uint8_t> FindHashInString(std::wstring_view wv);

  template <typename Char>
  auto FormatStringV(const _In_z_ _Printf_format_string_ Char* fmt, va_list va) -> std::basic_string<Char> {
    using cfn_t = int (*)(const Char*, va_list);
    using fn_t = int (*)(Char*, size_t, const Char*, va_list);
    cfn_t cfn;
    fn_t fn;
    if constexpr (std::is_same_v<Char, char>) {
      cfn = &_vscprintf;
      fn = &vsprintf_s;
    } else {
      cfn = &_vscwprintf;
      fn = &vswprintf_s;
    }

    std::basic_string<Char> str;

    int len = cfn(fmt, va);
    str.resize((size_t)len);
    fn(str.data(), str.size() + 1, fmt, va);

    return str;
  }

  template <typename Char>
  auto FormatString(const _In_z_ _Printf_format_string_ Char* fmt, ...) -> std::basic_string<Char> {
    va_list args;
    va_start(args, fmt);
    auto str = FormatStringV(fmt, args);
    va_end(args);
    return str;
  }

  int FormattedMessageBox(HWND hwnd, LPCWSTR caption, UINT type, _In_z_ _Printf_format_string_ LPCWSTR fmt, ...);

  std::wstring GetString(UINT id);

  std::wstring GetWindowTextString(HWND hwnd);

  void SetWindowTextStringFromTable(HWND hwnd, UINT id);

  long FloorIconSize(long size);

  HICON SetIconButton(HWND button, int resource);

  bool AreFilesTheSame(HANDLE a, HANDLE b);

  std::wstring MakePathLongCompatible(std::wstring file);

  HANDLE OpenForRead(const std::wstring& file, bool async = false);

  DWORD SetClipboardText(HWND hwnd, std::wstring_view text);

  std::wstring GetClipboardText(HWND hwnd);

  std::wstring SaveDialog(HWND hwnd, const wchar_t* defpath, const wchar_t* defname);

  DWORD SaveMemoryAsFile(const wchar_t* path, const void* p, DWORD size);

  std::wstring UTF8ToWide(const char* p);
  std::string WideToUTF8(const wchar_t* p);

  std::wstring ErrorToString(DWORD error);

  std::pair<const char*, size_t> GetResource(LPCWSTR name, LPCWSTR type);

  struct GdiObjectDeleter {
    void operator()(HGDIOBJ hobj) const {
      if (hobj)
        DeleteObject(hobj);
    }
  };

  template <typename T>
  using UniqueObject = std::unique_ptr<std::remove_pointer_t<T>, GdiObjectDeleter>;

  using UniquePen = UniqueObject<HPEN>;
  using UniqueBrush = UniqueObject<HBRUSH>;
  using UniqueRgn = UniqueObject<HRGN>;
  using UniquePalette = UniqueObject<HPALETTE>;
  using UniqueFont = UniqueObject<HFONT>;
  using UniqueBitmap = UniqueObject<HBITMAP>;

  UniqueFont GetDPIScaledFont(const wchar_t* face_name = nullptr);

  void SetFontForChildren(HWND hwnd, HFONT font);

  int GetDPIScaledPixels(HWND hwnd, int px);

  struct Version {
    uint16_t major{};
    uint16_t minor{};
    uint16_t patch{};

    constexpr Version(uint16_t major, uint16_t minor, uint16_t patch)
        : major(major)
        , minor(minor)
        , patch(patch) {}

    constexpr Version() = default;

    [[nodiscard]] constexpr uint64_t AsNumber() const { return ((uint64_t)major << 32) | ((uint64_t)minor << 16) | patch; }

    constexpr bool operator==(const Version& rhs) const { return AsNumber() == rhs.AsNumber(); }

    constexpr bool operator<(const Version& rhs) const { return AsNumber() < rhs.AsNumber(); }

    constexpr bool operator>(const Version& rhs) const { return AsNumber() > rhs.AsNumber(); }
  };

  Version GetLatestVersion();
} // namespace utl
