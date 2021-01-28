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

#include "SumFileParser.h"

#include <regex>
#include <string_view>

#include "base64.h"
#include "utl.h"

constexpr char k_regex_hex[] = R"(([0-9a-fA-F]{8,512}) [ \*](.+))";
constexpr char k_regex_b64[] = R"(([0-9a-zA-Z=+\/,\-_]{6,512}) [ \*](.+))";
constexpr char k_regex_sfv[] = R"((.+)\s+([0-9a-fA-F]{8}))";

class SumFileParser2
{
  std::regex _hex{ k_regex_hex };
  std::regex _b64{ k_regex_b64 };
  std::regex _sfv{ k_regex_sfv };

  enum class CommentStyle
  {
    Unknown,
    Semicolon,
    Hash
  } _comment{ CommentStyle::Unknown };

  enum class HashStyle
  {
    Unknown,
    Hex,
    Sfv,
    Base64
  } _hash{ HashStyle::Unknown };

public:
  FileSumList files{};

  bool ProcessLine(std::string_view sv)
  {
    using svmatch = std::match_results<std::string_view::iterator>;

    if (sv.find_first_not_of("\r\n\t\f\v ") == std::string_view::npos)
      return true; // empty line

    if ((_comment == CommentStyle::Unknown || _comment == CommentStyle::Hash) && sv[0] == L'#')
    {
      _comment = CommentStyle::Hash;
      return true;
    }

    if ((_comment == CommentStyle::Unknown || _comment == CommentStyle::Semicolon) && sv[0] == L';')
    {
      _comment = CommentStyle::Semicolon;
      return true;
    }

    if ((_hash == HashStyle::Unknown || _hash == HashStyle::Sfv))
    {
      svmatch pieces;
      if(std::regex_match(begin(sv), end(sv), pieces, _sfv))
      {
        _hash = HashStyle::Sfv;
        auto file = pieces[1].str();
        // According to wikipedia delimiter is always space.
        const auto notspace = file.find_last_not_of(' ');
        file.resize(notspace == std::string_view::npos ? 0 : notspace + 1);
        const auto hash_str = pieces[2].str();
        auto hash = utl::HashStringToBytes(std::string_view{ hash_str });
        if (!hash.empty())
        {
          files.emplace_back(std::move(file), std::move(hash));
          return true;
        }
      }
    }

    if ((_hash == HashStyle::Unknown || _hash == HashStyle::Hex))
    {
      svmatch pieces;
      if (std::regex_match(begin(sv), end(sv), pieces, _hex))
      {
        _hash = HashStyle::Hex;
        const auto hash_str = pieces[1].str();
        auto hash = utl::HashStringToBytes(std::string_view{ hash_str });
        if (!hash.empty())
        {
          files.emplace_back(pieces[2], std::move(hash));
          return true;
        }
      }
    }

    if ((_hash == HashStyle::Unknown || _hash == HashStyle::Base64))
    {
      svmatch pieces;
      if (std::regex_match(begin(sv), end(sv), pieces, _b64))
      {
        _hash = HashStyle::Base64;
        const auto str = pieces[1].str();
        auto hash = b64::decode(str.c_str(), str.size());
        if (!hash.empty())
        {
          files.emplace_back(pieces[2], std::move(hash));
          return true;
        }
      }
    }

    return false;
  }
};

// Returns error code if reading failed. If the file is not a sumfile or empty success is returned, but output is empty
DWORD TryParseSumFile(HANDLE h, FileSumList& output)
{
  static constexpr auto k_max_sumfile_size = 1u << 20; // 1 MB
  static constexpr auto k_min_sumfile_size = 6; // 6 bytes for base64 CRC

  output.clear();

  BY_HANDLE_FILE_INFORMATION fi;
  if (!GetFileInformationByHandle(h, &fi))
    return GetLastError();

  if (fi.nFileSizeHigh > 0 || fi.nFileSizeLow > k_max_sumfile_size || fi.nFileSizeLow < k_min_sumfile_size)
    return ERROR_SUCCESS;

  const auto size = static_cast<size_t>(fi.nFileSizeLow);
  const auto mapping = CreateFileMappingW(
    h,
    nullptr,
    PAGE_READONLY,
    0, 0,
    nullptr
  );

  if (!mapping)
    return GetLastError();

  const auto address = MapViewOfFile(
    mapping,
    FILE_MAP_READ,
    0, 0,
    0
  );

  if (!address)
  {
    const auto error = GetLastError();
    CloseHandle(mapping);
    return error;
  }

  auto first = static_cast<const char*>(address);
  const auto last = first + size;

  if (0 == memcmp(first, "\xEF\xBB\xBF", 3))
    first += 3; // file is at least 6 bytes so this is safe

  // special handling files with only a single hash in them
  {
    const std::string_view sv{ first, (size_t)(last - first) };
    const auto nwfirst = sv.find_first_not_of("\r\n\t\f\v ");
    if(nwfirst != std::string_view::npos)
    {
      const auto nwlast = sv.find_last_not_of("\r\n\t\f\v ");
      const std::string_view nwsv{ first + nwfirst, nwlast + 1 - nwfirst };
      auto hash = utl::HashStringToBytes(nwsv);
      if(!hash.empty())
      {
        output.emplace_back(std::string{}, std::move(hash));
        return ERROR_SUCCESS;
      }
    }
  }

  SumFileParser2 sfp;
  auto failed = false;
  for (auto it = first; it != last;)
  {
    // for CRLF we just accidentally interpret an extra empty line, which is valid in all formats
    const auto newline = std::find_if(it, last, [](char c) { return c == '\n' || c == '\r'; });
    // skip empty line
    if (newline == it)
    {
      ++it;
      continue;
    }

    if (!sfp.ProcessLine({ it, (size_t)(newline - it) }))
    {
      failed = true;
      break;
    }

    it = newline;
  }

  UnmapViewOfFile(address);
  CloseHandle(mapping);

  if (!failed)
    output = std::move(sfp.files);

  return ERROR_SUCCESS;
}