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
#include "Coordinator.h"

#include "FileHashTask.h"
#include "Settings.h"
#include "utl.h"
#include "wnd.h"

Coordinator::Coordinator(std::list<std::wstring> files)
    : _files_raw(std::move(files)) {}

Coordinator::~Coordinator() {
  Cancel();
  while (_references != 0)
    ;
}

void Coordinator::RegisterWindow(HWND window) {
  // Turns out the dialog can sometimes be still running at the time we receive a RELEASE, so let's reference here
  Reference();
  std::lock_guard guard{_window_mutex};
  assert(_window == nullptr);
  _window = window;
}

void Coordinator::UnregisterWindow() {
  {
    std::lock_guard guard{_window_mutex};
    assert(_window != nullptr);
    _window = nullptr;
  }
  Dereference();
}

unsigned Coordinator::Reference() {
  const auto references = ++_references;
  DebugMsg("ref+ %d\n", references);
  return references;
}

unsigned Coordinator::Dereference() {
  const auto references = --_references;
  DebugMsg("ref- %d\n", references);
  return references;
}

void Coordinator::AddFile(const std::wstring& path, const ProcessedFileList::FileInfo& fi) {
  const auto task = new FileHashTask(this, path, fi);
  _size_total += task->GetSize();
  _file_tasks.emplace_back(task);
}

void Coordinator::AddFiles() {
  _files = ProcessEverything(_files_raw, &settings);
  const auto type = _files.sumfile_type;
  if (type != -2) {
    _is_sumfile = true;
    if (type != -1) {
      if (settings.sumfile_algorithm_only)
        for (auto& a : settings.algorithms)
          a.SetNoSave(false);                    // disable all algorithms
      settings.algorithms[type].SetNoSave(true); // enable algorithm the sumfile is made with
    }
  }
  for (const auto& file : _files.files)
    AddFile(file.first, file.second);
}

void Coordinator::ProcessFiles() {
  // We have 0 files, oops!
  if (_file_tasks.empty() && _window) {
    SendNotifyMessageW(_window, wnd::WM_USER_ALL_FILES_FINISHED, wnd::k_user_magic_wparam, 0);
    return;
  }
  for (const auto& task : _file_tasks) {
    ++_files_not_finished;
    task->StartProcessing();
  }
}

void Coordinator::Cancel(bool wait) {
  for (const auto& file : _file_tasks)
    file->SetCancelled();

  if (wait)
    while (_files_not_finished > 0)
      Sleep(1);
}

void Coordinator::FileCompletionCallback(FileHashTask* file) {
  UNREFERENCED_PARAMETER(file);

  std::lock_guard guard{_window_mutex};

  const auto not_finished = --_files_not_finished;

  if (_window && not_finished == 0)
    SendNotifyMessageW(_window, wnd::WM_USER_ALL_FILES_FINISHED, wnd::k_user_magic_wparam, 0);
}

void Coordinator::FileProgressCallback(uint64_t size_progress) {
  if (_size_total == 0)
    return;

  const auto old_progress = _size_progressed.fetch_add(size_progress);
  const auto new_progress = old_progress + size_progress;
  const auto old_part = old_progress * k_progress_resolution / _size_total;
  const auto new_part = new_progress * k_progress_resolution / _size_total;

  if (old_part != new_part) {
    std::lock_guard guard{_window_mutex};
    if (_window)
      SendNotifyMessageW(
        _window,
        wnd::WM_USER_FILE_PROGRESS,
        wnd::k_user_magic_wparam,
        (LPARAM)new_part
      );
  }
}

std::pair<std::wstring, std::wstring> Coordinator::GetSumfileDefaultSavePathAndBaseName() {
  std::wstring name{L"checksums"};
  if (_files.files.size() == 1) {
    const auto& file = _files.files.begin()->first;
    const auto file_path = file.c_str();
    const auto file_name = (LPCWSTR)PathFindFileNameW(file_path);
    name = file_name;
  }
  return {_files.base_path, std::move(name)};
}
