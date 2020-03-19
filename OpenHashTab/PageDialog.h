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
#pragma once

class FileHashTask;
class PropPage;

class PageDialog
{
  static constexpr auto k_status_update_timer_id = (UINT_PTR)0x7c253816f7ef92ea;

  HWND _hwnd{};
  PropPage* _prop_page;

  unsigned _count_error{};
  unsigned _count_match{};
  unsigned _count_mismatch{};
  unsigned _count_unknown{};

  bool _temporary_status{};
  bool _finished{};

  enum ColIndex : int
  {
    ColIndex_Filename,
    ColIndex_Algorithm,
    ColIndex_Hash
  };

  INT_PTR CustomDrawListView(LPARAM lparam, HWND list) const;

  std::string GetSumfileAsString(size_t hasher);
  void SetTempStatus(PCTSTR status, UINT time);
  void UpdateDefaultStatus(bool force_reset = false);

  void InitDialog();

  void OnFileFinished(FileHashTask* file);
  void OnAllFilesFinished();
  void OnExportClicked();
  void OnHashEditChanged();
  void OnListDoubleClick(int item, int subitem);
  void OnListRightClick(bool dblclick = false);

public:
  PageDialog(HWND hwnd, void* prop_page);
  ~PageDialog();

  INT_PTR DlgProc(UINT msg, WPARAM wparam, LPARAM lparam);
};
