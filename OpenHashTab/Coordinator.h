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

class Coordinator
{
public:
  constexpr static auto k_progress_resolution = 256u;

private:
  std::list<tstring> _files;
  tstring _base;
  HWND _window{};
  uint64_t _size_total{};
  std::atomic<uint64_t> _size_progressed{};
  std::list<std::unique_ptr<FileHashTask>> _file_tasks;
  std::mutex _window_mutex{};
  std::atomic<unsigned> _references{};
  std::atomic<unsigned> _files_not_finished{};
  bool _is_sumfile{};

  void AddFile(const tstring& path, const std::vector<std::uint8_t>& expected_hash);

public:
  Coordinator(std::list<tstring> files, tstring base);
  virtual ~Coordinator();

  virtual void RegisterWindow(HWND window);
  virtual void UnregisterWindow();

  unsigned Reference();
  unsigned Dereference();

  void AddFiles();
  void ProcessFiles();
  void Cancel(bool wait = true);
  void FileCompletionCallback(FileHashTask* file);
  void FileProgressCallback(uint64_t size_progress);

  // The window should probably only inspect files before processing or after all are done
  const std::list<std::unique_ptr<FileHashTask>>& GetFiles() const { return _file_tasks; };
  bool IsSumfile() const { return _is_sumfile; }
  std::pair<tstring, tstring> GetSumfileDefaultSavePathAndBaseName();
};

class PropPageCoordinator : public Coordinator
{
public:
  using Coordinator::Coordinator;
  ~PropPageCoordinator() = default;

  void AddRef(HWND, LPPROPSHEETPAGE) { }
  UINT Create(HWND, LPPROPSHEETPAGE) { return 1; }
  void Release(HWND, LPPROPSHEETPAGE) { delete this; }
};

class StandaloneCoordinator : public Coordinator
{
public:
  using Coordinator::Coordinator;
  ~StandaloneCoordinator() = default;

  void UnregisterWindow() override
  {
    Coordinator::UnregisterWindow();
    PostQuitMessage(0);
  }
};