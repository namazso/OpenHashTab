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

#include "path.h"

#include "Settings.h"
#include "SumFileParser.h"
#include "utl.h"

#include <algorithm>


/*static std::wstring QuerySymbolicLink(std::wstring_view path)
{
  UNICODE_STRING name{
    (USHORT)(path.size() * sizeof(wchar_t)),
    (USHORT)(path.size() * sizeof(wchar_t)),
    (PWCH)path.data()
  };

  OBJECT_ATTRIBUTES attr;
  InitializeObjectAttributes(
    &attr,
    &name,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL
  );
  HANDLE handle{};
  auto status = NtOpenSymbolicLinkObject(
    &handle,
    SYMBOLIC_LINK_QUERY,
    &attr
  );
  if (!NT_SUCCESS(status))
    return {}; // probably not a symlink

  wchar_t buf[0x10000 / 2];
  UNICODE_STRING result{ USHRT_MAX, USHRT_MAX, buf };
  status = NtQuerySymbolicLinkObject(handle, &result, nullptr);
  NtClose(handle);
  if (NT_SUCCESS(status))
    return { result.Buffer, result.Buffer + (result.Length / sizeof(wchar_t)) };
  return {};
}*/


/*static std::vector<uint8_t> TryGetExpectedSumForFile(const std::wstring& path)
{
  std::vector<uint8_t> hash{};

  const auto file = utl::OpenForRead(path);
  if (file == INVALID_HANDLE_VALUE)
    return hash;

  const auto file_path = path.c_str();
  const auto file_name = (LPCWSTR)PathFindFileNameW(file_path);
  const auto base_path = std::wstring{ file_path, file_name };

  for (const auto& hasher : HashAlgorithm::g_hashers)
  {
    if (!Settings::instance.IsHashEnabled(&hasher))
      continue;

    auto sumfile_path = path + L".";
    auto handle = INVALID_HANDLE_VALUE;
    for (auto it = hasher.GetExtensions(); handle == INVALID_HANDLE_VALUE && *it; ++it)
      handle = utl::OpenForRead(sumfile_path + utl::UTF8ToWide(*it));

    if (handle != INVALID_HANDLE_VALUE)
    {
      FileSumList fsl;
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      if (fsl.size() == 1)
      {
        const auto& file_sum = *fsl.begin();

        auto valid = false;

        if (file_sum.first.empty())
        {
          valid = true;
        }
        else
        {
          const auto file_sum_path = base_path + utl::UTF8ToWide(file_sum.first.c_str());
          const auto sum_handle = utl::OpenForRead(file_sum_path);
          if (sum_handle != INVALID_HANDLE_VALUE)
          {
            const auto same = utl::AreFilesTheSame(sum_handle, file);
            CloseHandle(sum_handle);
            if (same)
              valid = true;
          }
        }

        if (valid)
        {
          hash = file_sum.second;
          break;
        }
      }
    }
  }

  CloseHandle(file);
  return hash;
}*/

// This function will normalize and un-shorten a path.
// Unfortunately unshortening a path with GetLongPathNameW requires that all directories in the way exist. This might
// not be the case for us, for example we might receive `C:\FOLDER~1\SUBFOL~1` where first exists and second doesn't.
// To fix this scenario we find the first folder from the end that does exist, and unshorten until that point, so that
// the previous example will become `C:\FolderWithLongName\SUBFOL~1`
static std::wstring NormalizePath(std::wstring_view path)
{
  const auto long_compat = utl::MakePathLongCompatible(std::wstring{ path });
  std::wstring full;
  {
    wchar_t buf[0x10000 / 2];
    const auto ret = GetFullPathNameW(
      long_compat.c_str(),
      std::size(buf),
      buf,
      nullptr
    );
    if (ret == 0)
      return utl::MakePathLongCompatible(long_compat);
    full = buf;
  }
  auto slash = full.rbegin();
  while(true)
  {
    wchar_t buf2[0x10000 / 2];
    const auto ret = GetLongPathNameW(
      std::wstring{ full.begin(), slash.base() }.c_str(),
      buf2,
      std::size(buf2)
    );
    if (ret != 0)
      return utl::MakePathLongCompatible(std::wstring{ buf2 } + std::wstring{ slash.base(), full.end() });

    const auto result = std::find(slash, full.rend(), L'\\');
    if (result == full.rend())
      return utl::MakePathLongCompatible(std::move(full)); // entire path is wrong
    slash = result + 1;
  }
}

ProcessedFileList ProcessEverything(std::list<std::wstring> list)
{
  ProcessedFileList pfl;

  pfl.sumfile_type = -2;
  std::list<std::pair<std::wstring, std::vector<uint8_t>>> fsl_absolute;

  if (list.size() == 1)
  {
    auto& file = *list.begin();

    const auto sumfile_path = file.c_str();
    const auto sumfile_name = static_cast<LPCWSTR>(PathFindFileNameW(sumfile_path));
    const auto sumfile_base_path = std::wstring{ sumfile_path, sumfile_name };

    // if there is only one file that exists, the base path is surely the containing dir
    pfl.base_path = sumfile_base_path;

    const auto handle = utl::OpenForRead(file); // OpenForRead handles overlong paths
    if (handle != INVALID_HANDLE_VALUE)
    {
      FileSumList fsl;
      // we ignore the error returned, result will just be empty
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      if (!fsl.empty())
      {
        pfl.sumfile_type = -1;
        auto extension = PathFindExtensionW(sumfile_path);
        if (*extension == L'.')
        {
          ++extension;
          const auto ext_char = utl::WideToUTF8(extension);
          for(const auto& algo : HashAlgorithm::g_hashers)
            for(auto ext = algo.GetExtensions(); *ext; ++ext)
              if (0 == strcmp(*ext, ext_char.c_str()))
                pfl.sumfile_type = algo.Idx();
        }

        for (auto& filesum : fsl)
        {
          // we disallow no filename when sumfile is main file
          if (filesum.first.empty())
            continue;

          const auto path = sumfile_base_path + utl::UTF8ToWide(filesum.first.c_str());

          // absolutize paths we found in the sumfile
          fsl_absolute.emplace_back(path, std::move(filesum.second));
        }

        // fall through - let it calculate the sumfile's sum, in case the user needs that
      }
    }
  }
  else
  {
    list.sort();

    const auto& front = list.front();
    const auto& back = list.back();

    const auto mismatch = std::mismatch(begin(front), end(front), begin(back), end(back));

    auto base = std::wstring{ begin(front), mismatch.first };

    const auto slashn = base.rfind(L'\\');

    if (slashn != std::wstring::npos)
      base.resize(slashn);

    pfl.base_path = std::move(base);
  }

  if (!pfl.base_path.empty())
  {
    if (pfl.base_path[pfl.base_path.size() - 1] != L'\\')
      pfl.base_path.append(L"\\");
    pfl.base_path = NormalizePath(pfl.base_path);
  }

  for(const auto& entry : fsl_absolute)
  {
    const auto normalized = NormalizePath(entry.first);

    std::wstring relative_path;

    if (normalized.rfind(pfl.base_path, 0) == 0)
      relative_path = normalized.substr(pfl.base_path.size());
    else
      relative_path = normalized;

    const auto exist = pfl.files.find(normalized);
    if (exist != pfl.files.end())
      exist->second.expected_hashes.emplace_back(entry.second);
    else
    {
      ProcessedFileList::FileInfo fi;
      fi.relative_path = std::move(relative_path);
      fi.expected_hashes.emplace_back(entry.second);
      pfl.files[normalized] = fi;
    }
  }

  for(const auto& file : list)
  {
    const auto normalized = NormalizePath(file);

    if(PathIsDirectoryW(normalized.c_str()))
    {
      DWORD error = 0;

      {
        WIN32_FIND_DATA find_data;
        const auto find_handle = FindFirstFileW((normalized + L"\\*").c_str(), &find_data);

        if (find_handle != INVALID_HANDLE_VALUE)
        {
          do
          {
            if ((0 == wcscmp(L".", find_data.cFileName)) || (0 == wcscmp(L"..", find_data.cFileName)))
              continue; // For whatever reason if you use long paths with FindFirstFile it returns "." and ".."

            // TODO: figure out what to do with reparse points
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
              continue;

            list.push_back(normalized + L"\\" + find_data.cFileName);
          } while (FindNextFileW(find_handle, &find_data) != 0);
          error = GetLastError();
          FindClose(find_handle);
        }
        else
        {
          error = GetLastError();
        }
      }
      // BUG: We just leave it in as file if we can't open so some random error message will be displayed
      if (error && error != ERROR_NO_MORE_FILES)
        goto not_a_directory;  // NOLINT(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
    }
    else
    {
      not_a_directory:

      // TODO: look for sumfile

      ProcessedFileList::FileInfo fi;

      if (normalized.rfind(pfl.base_path, 0) == 0)
        fi.relative_path = normalized.substr(pfl.base_path.size());
      else
        fi.relative_path = normalized;

      const auto exist = pfl.files.find(normalized);
      if (exist == pfl.files.end())
        pfl.files[normalized] = fi;
    }

  }

  return pfl;
}