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
#include "path.h"

#include "Settings.h"
#include "SumFileParser.h"
#include "utl.h"

// This function will normalize and un-shorten a path.
// Unfortunately unshortening a path with GetLongPathNameW requires that all directories in the way exist. This might
// not be the case for us, for example we might receive `C:\FOLDER~1\SUBFOL~1` where first exists and second doesn't.
// To fix this scenario we find the first folder from the end that does exist, and unshorten until that point, so that
// the previous example will become `C:\FolderWithLongName\SUBFOL~1`
static std::wstring NormalizePath(std::wstring_view path) {
  const auto long_compat = utl::MakePathLongCompatible(std::wstring{path});
  const auto pbuf = std::make_unique<wchar_t[]>(PATHCCH_MAX_CCH);
  std::wstring full;
  {
    const auto ret = GetFullPathNameW(
      long_compat.c_str(),
      PATHCCH_MAX_CCH,
      pbuf.get(),
      nullptr
    );
    if (ret == 0)
      return utl::MakePathLongCompatible(long_compat);
    full = pbuf.get();
  }
  auto slash = full.rbegin();
  while (true) {
    const auto ret = GetLongPathNameW(
      std::wstring{full.begin(), slash.base()}.c_str(),
      pbuf.get(),
      PATHCCH_MAX_CCH
    );
    if (ret != 0)
      return utl::MakePathLongCompatible(std::wstring{pbuf.get()} + std::wstring{slash.base(), full.end()});

    const auto result = std::find(slash, full.rend(), L'\\');
    if (result == full.rend())
      return utl::MakePathLongCompatible(std::move(full)); // entire path is wrong
    slash = result + 1;
  }
}

ProcessedFileList ProcessEverything(std::list<std::wstring> list, const Settings* settings) {
  ProcessedFileList pfl;

  pfl.sumfile_type = -2;
  std::list<std::pair<std::wstring, std::vector<uint8_t>>> fsl_absolute;

  if (list.size() == 1) {
    auto& file = *list.begin();

    const auto sumfile_path = file.c_str();
    const auto sumfile_name = static_cast<LPCWSTR>(PathFindFileNameW(sumfile_path));
    const auto sumfile_base_path = std::wstring{sumfile_path, sumfile_name};

    // if there is only one file that exists, the base path is surely the containing dir
    pfl.base_path = sumfile_base_path;

    const auto handle = utl::OpenForRead(file); // OpenForRead handles overlong paths
    if (handle != INVALID_HANDLE_VALUE) {
      FileSumList fsl;
      // we ignore the error returned, result will just be empty
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      size_t has_at_least_one_filename = false;
      for (auto& filesum : fsl) {
        if (!filesum.first.empty()) {
          has_at_least_one_filename = true;
          break;
        }
      }
      if (has_at_least_one_filename) {
        pfl.sumfile_type = -1;
        auto extension = PathFindExtensionW(sumfile_path);
        if (*extension == L'.') {
          ++extension;
          const auto ext_char = utl::WideToUTF8(extension);
          for (const auto& algo : LegacyHashAlgorithm::Algorithms())
            for (auto ext = algo.GetExtensions(); *ext; ++ext)
              if (0 == strcmp(*ext, ext_char.c_str()))
                pfl.sumfile_type = algo.Idx();
        }

        for (auto& filesum : fsl) {
          // we disallow no filename when sumfile is main file
          if (filesum.first.empty())
            continue;

          const auto path = sumfile_base_path + utl::UTF8ToWide(filesum.first.c_str());

          // absolutize paths we found in the sumfile
          fsl_absolute.emplace_back(path, std::move(filesum.second));
        }

        if (!settings->hash_sumfile_too)
          list.erase(list.begin());
      }
    }
  } else {
    list.sort();

    const auto& front = list.front();
    const auto& back = list.back();

    const auto mismatch = std::mismatch(begin(front), end(front), begin(back), end(back));

    auto base = std::wstring{begin(front), mismatch.first};

    const auto slashn = base.rfind(L'\\');

    if (slashn != std::wstring::npos)
      base.resize(slashn);

    pfl.base_path = std::move(base);
  }

  if (!pfl.base_path.empty()) {
    if (pfl.base_path[pfl.base_path.size() - 1] != L'\\')
      pfl.base_path.append(L"\\");
    pfl.base_path = NormalizePath(pfl.base_path);
  }

  for (const auto& entry : fsl_absolute) {
    const auto normalized = NormalizePath(entry.first);

    std::wstring relative_path;

    if (normalized.rfind(pfl.base_path, 0) == 0)
      relative_path = normalized.substr(pfl.base_path.size());
    else
      relative_path = normalized;

    const auto exist = pfl.files.find(normalized);
    if (exist != pfl.files.end())
      exist->second.expected_hashes.emplace_back(entry.second);
    else {
      ProcessedFileList::FileInfo fi;
      fi.relative_path = std::move(relative_path);
      fi.expected_hashes.emplace_back(entry.second);
      pfl.files[normalized] = fi;
    }
  }

  for (const auto& file : list) {
    const auto normalized = NormalizePath(file);

    if (PathIsDirectoryW(normalized.c_str())) {
      DWORD error = 0;

      {
        WIN32_FIND_DATA find_data;
        const auto find_handle = FindFirstFileW((normalized + L"\\*").c_str(), &find_data);

        if (find_handle != INVALID_HANDLE_VALUE) {
          do {
            if ((0 == wcscmp(L".", find_data.cFileName)) || (0 == wcscmp(L"..", find_data.cFileName)))
              continue; // For whatever reason if you use long paths with FindFirstFile it returns "." and ".."

            // TODO: figure out what to do with reparse points
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
              continue;

            list.push_back(normalized + L"\\" + find_data.cFileName);
          } while (FindNextFileW(find_handle, &find_data) != 0);
          error = GetLastError();
          FindClose(find_handle);
        } else {
          error = GetLastError();
        }
      }
      // BUG: We just handle it as file if we can't list so some error message will be displayed.
      //   This may or may not be the actual error, but gets basic ones like no perms right.
      if (error && error != ERROR_NO_MORE_FILES)
        goto not_a_directory; // NOLINT(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
    } else {
not_a_directory:

      ProcessedFileList::FileInfo fi;

      // Look for sumfile for this file. If we're already processing a sumfile, don't look for one for security.
      if (pfl.sumfile_type == -2 && settings->look_for_sumfiles) {
        for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i) {
          if (!settings->algorithms[i])
            continue;

          for (auto ext = LegacyHashAlgorithm::Algorithms()[i].GetExtensions(); *ext; ++ext) {
            const auto sumfile_path = normalized + L"." + utl::UTF8ToWide(*ext);
            const auto handle = utl::OpenForRead(sumfile_path);
            if (handle != INVALID_HANDLE_VALUE) {
              FileSumList fsl;
              // we ignore the error returned, result will just be empty
              TryParseSumFile(handle, fsl);
              CloseHandle(handle);
              for (const auto& sum : fsl)
                fi.expected_hashes.push_back(sum.second);
            }
          }
        }
      }

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
