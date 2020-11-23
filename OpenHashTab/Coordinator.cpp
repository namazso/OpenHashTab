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

#include "Coordinator.h"
#include "utl.h"
#include "wnd.h"
#include "SumFileParser.h"
#include "Settings.h"
#include "FileHashTask.h"

static std::vector<std::uint8_t> TryGetExpectedSumForFile(const std::wstring& path)
{
  std::vector<std::uint8_t> hash{};

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

Coordinator::Coordinator(std::list<std::wstring> files)
  : _files_raw(std::move(files)) {}

Coordinator::~Coordinator()
{
  Cancel();
  while (_references != 0)
    ;
}

void Coordinator::RegisterWindow(HWND window)
{
  // Turns out the dialog can sometimes be still running at the time we receive a RELEASE, so let's reference here
  Reference();
  std::lock_guard<std::mutex> guard{_window_mutex};
  assert(_window == nullptr);
  _window = window;
}

void Coordinator::UnregisterWindow()
{
  {
    std::lock_guard<std::mutex> guard{ _window_mutex };
    assert(_window != nullptr);
    _window = nullptr;
  }
  Dereference();
}

unsigned Coordinator::Reference()
{
  const auto references = ++_references;
  DebugMsg("ref+ %d\n", references);
  return references;
}

unsigned Coordinator::Dereference()
{
  const auto references = --_references;
  DebugMsg("ref- %d\n", references);
  return references;
}

void Coordinator::AddFile(std::wstring path, const ProcessedFileList::FileData& fd)
{
  // BUG: we ignore what kind of hash we're looking for for now
  const auto expected = !fd.expected_unknown_hash.empty()
    ? fd.expected_unknown_hash
    : *std::max_element(begin(fd.expected_hashes), end(fd.expected_hashes), [] (const auto& a, const auto& b)
      {
        return a.size() < b.size();
      });

  const auto task = new FileHashTask(std::move(path), this, fd.relative_path, expected);
  _size_total += task->GetSize();
  _file_tasks.emplace_back(task);
}

void Coordinator::AddFiles()
{
  _files = ProcessEverything(_files_raw);

  for (const auto& file : _files.files)
    AddFile(file.first, file.second);
}

void Coordinator::ProcessFiles()
{
  // We have 0 files, oops!
  if(_file_tasks.empty() && _window)
  {
    SendNotifyMessageW(_window, wnd::WM_USER_ALL_FILES_FINISHED, wnd::k_user_magic_wparam, 0);
    return;
  }
  for (const auto& task : _file_tasks)
  {
    ++_files_not_finished;
    task->StartProcessing();
  }
}

void Coordinator::Cancel(bool wait)
{
  for (const auto& file : _file_tasks)
    file->SetCancelled();

  if(wait)
    while (_files_not_finished > 0)
      Sleep(1);
}

void Coordinator::FileCompletionCallback(FileHashTask* file)
{
  std::lock_guard<std::mutex> guard{ _window_mutex };

  const auto not_finished = --_files_not_finished;

  if (_window)
  {
    SendNotifyMessageW(_window, wnd::WM_USER_FILE_FINISHED, wnd::k_user_magic_wparam, (LPARAM)file);
    if (not_finished == 0)
      SendNotifyMessageW(_window, wnd::WM_USER_ALL_FILES_FINISHED, wnd::k_user_magic_wparam, 0);
  }
}

void Coordinator::FileProgressCallback(uint64_t size_progress)
{
  if (_size_total == 0)
    return;

  const auto old_progress = _size_progressed.fetch_add(size_progress);
  const auto new_progress = old_progress + size_progress;
  const auto old_part = old_progress * k_progress_resolution / _size_total;
  const auto new_part = new_progress * k_progress_resolution / _size_total;

  if(old_part != new_part)
  {
    std::lock_guard<std::mutex> guard{ _window_mutex };
    if(_window)
      SendNotifyMessageW(
        _window,
        wnd::WM_USER_FILE_PROGRESS,
        wnd::k_user_magic_wparam,
        new_part
      );
  }
}

std::pair<std::wstring, std::wstring> Coordinator::GetSumfileDefaultSavePathAndBaseName()
{
  std::wstring name{};
  if(_files.files.size() == 1)
  {
    const auto& file = _files.files.begin()->first;
    const auto file_path = file.c_str();
    const auto file_name = (LPCWSTR)PathFindFileNameW(file_path);
    name = file_name;
  }
  return { _files.base_path, std::move(name) };
}
