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

#include "SumFileParser.h"
#include "Hasher.h"
#include "utl.h"

class SumFileParser
{
  std::vector<uint8_t> _current_hash{};
  std::string _current_filename{};
  FileSumList _files{};

  uint8_t _half_byte{};

  enum class State
  {
    LineBegin,
    Comment,
    Hash1,
    Hash2,
    Space,
    SpaceOrStar,
    FileName,

    Invalid
  };

  State _state{ State::LineBegin };


public:
  const FileSumList& GetFiles() const { return _files; }
  FileSumList& GetFiles() { return _files; }

  bool Process(int c)
  {
    switch (_state)
    {
    case State::LineBegin:
      switch (c)
      {
      case '\r':
      case '\n':
      case EOF:
        break;
      case '#':
        _state = State::Comment;
        break;
      default:
        if (utl::unhex(c) != 0xFF)
        {
          _state = State::Hash1;
          Process(c);
        }
        else
        {
          _state = State::Invalid;
        }
        break;
      }
      break;
    case State::Comment:
      switch (c)
      {
      case '\r':
      case '\n':
        _state = State::LineBegin;
        break;
      default:
        break;
      }
      break;
    case State::Hash1:
    {
      const auto hexchar = utl::unhex(c);
      if (hexchar != 0xFF)
      {
        _half_byte = hexchar << 4;
        _state = State::Hash2;
      }
      else
      {
        _state = State::Space;
        Process(c);
      }
    }
    break;
    case State::Hash2:
    {
      const auto hexchar = utl::unhex(c);
      if (hexchar != 0xFF)
      {
        _current_hash.push_back(hexchar | _half_byte);
        if (_current_hash.size() > HashAlgorithm::k_max_size)
          _state = State::Space;
        else
          _state = State::Hash1;
      }
      else
      {
        _state = State::Invalid;
      }
    }
    break;
    case State::Space:
      switch(c)
      {
      case ' ':
        _state = State::SpaceOrStar;
        break;
      // as a special exception allow no filename, to work with files containing a single hash
      case '\r':
      case '\n':
      case EOF:
        _state = State::FileName;
        Process(c);
        break;
      default:
        _state = State::Invalid;
        break;
      }
      break;
    case State::SpaceOrStar:
      _state = (c == ' ' || c == '*') ? State::FileName : State::Invalid;
      break;
    case State::FileName:
      switch (c)
      {
      case '\r':
      case '\n':
      case EOF:
        _files.emplace_back(std::move(_current_filename), std::move(_current_hash));
        _current_filename.clear();
        _current_hash.clear();
        _state = State::LineBegin;
        break;
      case '\0': // disallow filenames containing null
        _state = State::Invalid;
        break;
      default:
        _current_filename += char(c);
        break;
      }
      break;
    case State::Invalid:
      break;
    }

    return _state != State::Invalid;
  }
};

// Returns error code if reading failed. If the file is not a sumfile or an empty one no error is returned, but output is empty
DWORD TryParseSumFile(HANDLE h, FileSumList& output)
{
  static constexpr auto k_max_sumfile_size = 1u << 20; // 1 MB
  output.clear();

  BY_HANDLE_FILE_INFORMATION fi;
  if (!GetFileInformationByHandle(h, &fi))
    return GetLastError();

  if (fi.nFileSizeHigh > 0 || fi.nFileSizeLow > k_max_sumfile_size || fi.nFileSizeLow == 0)
    return ERROR_SUCCESS;

  const auto size = size_t(fi.nFileSizeLow);
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

  SumFileParser sfp;
  const auto first = (char*)address;
  const auto last = first + size;
  for (auto it = first; it != last; ++it)
    if (!sfp.Process(*it))
      break;

  UnmapViewOfFile(address);
  CloseHandle(mapping);

  if (sfp.Process(EOF))
  {
    output = std::move(sfp.GetFiles());
    return ERROR_SUCCESS;
  }

  return ERROR_SUCCESS;
}