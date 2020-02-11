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

#include "PropPage.h"
#include "utl.h"
#include "wnd.h"
#include "SumFileParser.h"
#include "Hasher.h"
#include "FileHashTask.h"

static std::vector<std::uint8_t> TryGetExpectedSumForFile(const tstring& path)
{
  std::vector<std::uint8_t> hash{};

  const auto file = utl::OpenForRead(path);
  if (file == INVALID_HANDLE_VALUE)
    return hash;

  const auto file_path = path.c_str();
  const auto file_name = (LPCTSTR)PathFindFileName(file_path);
  const auto base_path = tstring{ file_path, file_name };

  for (auto hasher : HashAlgorithm::g_hashers)
  {
    if (!hasher.IsEnabled())
      continue;

    auto sumfile_path = path + _T(".");
    auto handle = INVALID_HANDLE_VALUE;
    for (auto it = hasher.GetExtensions(); handle == INVALID_HANDLE_VALUE && *it; ++it)
      handle = utl::OpenForRead(sumfile_path + utl::UTF8ToTString(*it));

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
          const auto file_sum_path = base_path + utl::UTF8ToTString(file_sum.first.c_str());
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
}

PropPage::PropPage(std::list<tstring> files, tstring base)
  : _files(std::move(files))
  , _base(std::move(base)) {}

PropPage::~PropPage()
{
  Cancel();
  while (_references != 0)
    ;
}

void PropPage::RegisterWindow(HWND window)
{
  // Turns out the dialog can sometimes be still running at the time we receive a RELEASE, so let's reference here
  Reference();
  std::lock_guard<std::mutex> guard{_window_mutex};
  assert(_window == nullptr);
  _window = window;
}

void PropPage::UnregisterWindow()
{
  {
    std::lock_guard<std::mutex> guard{ _window_mutex };
    assert(_window != nullptr);
    _window = nullptr;
  }
  Dereference();
}

unsigned PropPage::Reference()
{
  const auto references = ++_references;
  DebugMsg("ref+ %d\n", references);
  return references;
}

unsigned PropPage::Dereference()
{
  const auto references = --_references;
  DebugMsg("ref- %d\n", references);
  return references;
}

void PropPage::AddFile(const tstring& path, const std::vector<std::uint8_t>& expected_hash)
{
  auto dispname = utl::CanonicalizePath(path);

  // If path looks like _base + filename use filename as displayname, else the canonical name.
  // Optimally you'd use PathRelativePathTo for this, however that function not only doesn't support long paths, it also
  // doesn't have a PathCch alternative on Win8+. Additionally, seeing ".." and similar in the Name part could be confusing
  // to users, so printing full path instead is probably a better idea anyways.
  if (dispname.size() >= _base.size())
    if (std::equal(begin(_base), end(_base), begin(dispname)))
      dispname = dispname.substr(_base.size());

  const auto task = new FileHashTask(path, this, std::move(dispname), expected_hash);
  _file_tasks.emplace_back(task);
}

void PropPage::AddFiles()
{
  // for each directory in _files remove it from the list add it's content to the end.
  // since we push elements to the end end iterator is intentionally not saved.
  for (auto it = begin(_files); it != end(_files);)
  {
    if (PathIsDirectory(it->c_str()))
    {
      WIN32_FIND_DATA find_data;
      const auto find_handle = FindFirstFile(utl::MakePathLongCompatible(*it + _T("\\*")).c_str(), &find_data);

      DWORD error;

      if (find_handle != INVALID_HANDLE_VALUE)
      {
        do
          _files.push_back(*it + _T("\\") + find_data.cFileName);
        while (FindNextFile(find_handle, &find_data) != 0);
        error = GetLastError();
        FindClose(find_handle);
      }
      else
      {
        error = GetLastError();
      }

      // TODO: maybe handle error differently?
      (void)error;

      _files.erase(it++);
    }
    else
    {
      ++it;
    }
  }

  if (_files.empty())
    return;

  if (_files.size() == 1)
  {
    auto& file = *_files.begin();
    const auto handle = utl::OpenForRead(file);
    if (handle != INVALID_HANDLE_VALUE)
    {
      FileSumList fsl;
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      if (!fsl.empty())
      {
        _is_sumfile = true;

        const auto sumfile_path = file.c_str();
        const auto sumfile_name = (LPCTSTR)PathFindFileName(sumfile_path);
        const auto sumfile_base_path = tstring{ sumfile_path, sumfile_name };
        for (auto& filesum : fsl)
        {
          // we disallow no filename when sumfile is main file
          if (filesum.first.empty())
            continue;

          const auto path = sumfile_base_path + utl::UTF8ToTString(filesum.first.c_str());
          AddFile(path, filesum.second);
        }

        // fall through - let it calculate the sumfile's sum, in case the user needs that
      }
    }
  }

  for (auto& file : _files)
  {
    const auto expected = TryGetExpectedSumForFile(file);
    AddFile(file, expected);
  }
}

void PropPage::ProcessFiles()
{
  for (const auto& task : _file_tasks)
  {
    ++_files_not_finished;
    task->StartProcessing();
  }
}

void PropPage::Cancel(bool wait)
{
  for (const auto& file : _file_tasks)
    file->SetCancelled();

  if(wait)
    while (_files_not_finished > 0)
      Sleep(1);
}

void PropPage::FileCompletionCallback(FileHashTask* file)
{
  std::lock_guard<std::mutex> guard{ _window_mutex };

  const auto not_finished = --_files_not_finished;

  if (_window)
  {
    SendNotifyMessage(_window, wnd::WM_USER_FILE_FINISHED, wnd::k_user_magic_wparam, (LPARAM)file);
    if (not_finished == 0)
      SendNotifyMessage(_window, wnd::WM_USER_ALL_FILES_FINISHED, wnd::k_user_magic_wparam, 0);
  }
}

std::pair<tstring, tstring> PropPage::GetSumfileDefaultSavePathAndBaseName()
{
  const auto& file = *_files.begin();
  const auto file_path = file.c_str();
  const auto file_name = (LPCTSTR)PathFindFileName(file_path);
  const auto dir = tstring{ file_path, file_name };
  auto name = _files.size() == 1 ? tstring{ file_name } : tstring{};
  return { _base, std::move(name) };
}
