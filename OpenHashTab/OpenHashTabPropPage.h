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
  std::list<tstring> _files;

  tstring _base;

  HWND _hwnd{};

  std::mutex _list_view_lock;

  std::list<FileHashTask*> _file_tasks;

  std::atomic<unsigned> _references{};

  std::atomic<unsigned> _files_not_finished{};

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
