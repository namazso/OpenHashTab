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
#pragma once
#include "utl.h"

class FileHashTask;

class OpenHashTabPropPage
{
  static constexpr auto k_status_update_timer_id = (UINT_PTR)0x7c253816f7ef92ea;
  static constexpr auto k_magic_user_wparam = (WPARAM)0x1c725fcfdcbf5843;

  enum UserWindowMessages : UINT
  {
    WM_USER_FILE_COMPLETED = WM_USER
  };

  std::list<tstring> _files;
  tstring _base;
  HWND _hwnd{};
  std::mutex _dont_let_the_window_delete_lock;
  std::list<FileHashTask*> _file_tasks;
  std::atomic<unsigned> _references{};
  std::atomic<unsigned> _files_not_finished{};
  unsigned _count_error{};
  unsigned _count_match{};
  unsigned _count_mismatch{};
  unsigned _count_unknown{};
  bool _temporary_status{};
  bool _is_sumfile{ false };
  volatile bool _hwnd_deleted{ false };

  enum ColIndex : int
  {
    ColIndex_Filename,
    ColIndex_Algorithm,
    ColIndex_Hash
  };

  ~OpenHashTabPropPage();

  INT_PTR CustomDrawListView(LPARAM lparam, HWND list) const;

  std::string GetSumfileAsString(size_t hasher);
  void SetTempStatus(PCTSTR status, UINT time);
  void UpdateDefaultStatus(bool force_reset = false);
  void FileCompleted(FileHashTask* file);

  void AddFiles();
  void AddFile(const tstring& path, const std::vector<std::uint8_t>& expected_hash = {});
  static std::vector<std::uint8_t> TryGetExpectedSumForFile(const tstring& path);
  void ProcessFiles();
  void Cancel();

public:
  OpenHashTabPropPage(std::list<tstring> files, tstring base);

  UINT Create(HWND hwnd, LPPROPSHEETPAGE ppsp) { return 1; }

  INT_PTR DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  void Reference(HWND hwnd, LPPROPSHEETPAGE ppsp) { Reference(); }
  unsigned Reference()
  {
    const auto references = ++_references;
    DebugMsg("ref+ %d\n", references);
    return references;
  }

  void Dereference(HWND hwnd, LPPROPSHEETPAGE ppsp) { Dereference(); }
  unsigned Dereference()
  {
    const auto references = --_references;
    DebugMsg("ref- %d\n", references);
    if(references == 0)
      delete this;

    return references;
  }

  void FileCompletionCallback(FileHashTask* file);
};
