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

#include "OpenHashTabPropPage.h"
#include "SumFileParser.h"
#include "FileHashTask.h"
#include "utl.h"

static constexpr auto k_color_error_fg = RGB(255, 55, 23);

static constexpr auto k_color_match_fg = RGB(255, 255, 255);
static constexpr auto k_color_match_bg = RGB(45, 170, 23);

static constexpr auto k_color_mismatch_fg = RGB(255, 255, 255);
static constexpr auto k_color_mismatch_bg = RGB(230, 55, 23);

inline void SetTextFromTable(HWND hwnd, int control, UINT string_id)
{
  SetDlgItemText(hwnd, control, utl::GetString(string_id).c_str());
};

OpenHashTabPropPage::OpenHashTabPropPage(std::list<tstring> files, tstring base)
  : _files(std::move(files))
  , _base(std::move(base))
{
}

OpenHashTabPropPage::~OpenHashTabPropPage()
{
  Cancel();

  for (const auto p : _file_tasks)
    delete p;
}

INT_PTR OpenHashTabPropPage::CustomDrawListView(LPARAM lparam, HWND list) const
{
  // No hash to compare to  - system colors
  // Error processing file  - system bg, red text
  // Hash mismatch          - red bg, white text for all
  // Hash matches           - green bg, white text for algo matching

  const auto lplvcd = (LPNMLVCUSTOMDRAW)lparam;

  switch (lplvcd->nmcd.dwDrawStage)
  {
  case CDDS_PREPAINT:
    return CDRF_NOTIFYITEMDRAW;

  case CDDS_ITEMPREPAINT:
    return CDRF_NOTIFYSUBITEMDRAW;

  case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
  {
    switch (lplvcd->iSubItem)
    {
    case ColIndex_Hash:
    {
      LVITEM lvitem
      {
        LVIF_PARAM,
        (int)lplvcd->nmcd.dwItemSpec
      };
      ListView_GetItem(list, &lvitem);
      const auto file_hash = FileHashTask::FromLparam(lvitem.lParam);
      const auto file = file_hash.first;
      const auto match = file->GetMatchState();
      if(file->GetError() != ERROR_SUCCESS)
      {
        lplvcd->clrText = k_color_error_fg;
        return CDRF_NEWFONT;
      }
      else if(match != FileHashTask::MatchState_None)
      {
        if(match == FileHashTask::MatchState_Mismatch)
        {
          lplvcd->clrText = k_color_mismatch_fg;
          lplvcd->clrTextBk = k_color_mismatch_bg;
        }
        else // Match
        {
          if((size_t)match == file_hash.second)
          {
            lplvcd->clrText = k_color_match_fg;
            lplvcd->clrTextBk = k_color_match_bg;
          }
        }
        return CDRF_NEWFONT;
      }
      // fall through
    }
    default:
      break;
    }

  }
  default:
    break;
  }
  return CDRF_DODEFAULT;
}

INT_PTR OpenHashTabPropPage::DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  auto ret = FALSE;
  DebugMsg("WndProc uMsg: %04X wParam: %08X lParam: %016X\n", msg, wparam, lparam);
  switch (msg)
  {
  case WM_INITDIALOG:
  {
    _hwnd = hwnd;

    // TODO: figure out a way to dynamically resize this thing
    //RECT rect;
    //GetClientRect(hwnd, &rect);
    //SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    SetTextFromTable(hwnd, IDC_STATIC_CHECK_AGAINST,  IDS_CHECK_AGAINST);
    SetTextFromTable(hwnd, IDC_STATIC_EXPORT_TO,      IDS_EXPORT_TO);
    SetTextFromTable(hwnd, IDC_BUTTON_EXPORT,         IDS_EXPORT_BTN);
    SetTextFromTable(hwnd, IDC_STATIC_PROCESSING,     IDS_PROCESSING);
    SetTextFromTable(hwnd, IDC_BUTTON_CLIPBOARD,      IDS_CLIPBOARD);

    const auto list = GetDlgItem(hwnd, IDC_HASH_LIST);

    SendMessage(list, LVM_SETTEXTBKCOLOR, 0, (LPARAM)CLR_NONE);
    SendMessage(list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    // we put the string table ID in the text length field, to fix it up later
    LVCOLUMN cols[] =
    {
      {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 140,   nullptr, IDS_FILENAME},
      {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 70,    nullptr, IDS_ALGORITHM},
      {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 1100,  nullptr, IDS_HASH},
    };

    for(auto i = 0u; i < std::size(cols); ++i)
    {
      auto& col = cols[i];
      const auto tstr = utl::GetString(col.cchTextMax);
      col.pszText = (LPTSTR)tstr.c_str();
      ListView_InsertColumn(list, i, &cols[i]);
    }

    const auto combobox = GetDlgItem(hwnd, IDC_COMBO_EXPORT);
    for(auto name : k_hashers_name)
      ComboBox_AddString(combobox, name);

    ComboBox_SetCurSel(combobox, 0);

    AddFiles();

    if(_is_sumfile)
      SetTextFromTable(hwnd, IDC_STATIC_SUMFILE, IDS_SUMFILE);

    if (_file_tasks.size() == 1)
      ListView_SetColumnWidth(list, ColIndex_Filename, 0);

    ProcessFiles();
    break;
  }
  case WM_DESTROY: //WM_NCDESTROY:
  {
    {
      std::lock_guard<std::mutex> guard(_list_view_lock);
      _hwnd_deleted = true;
    }
    break;
  }
  case WM_NOTIFY:
  {
    const auto phdr = (LPNMHDR)lparam;
    switch(phdr->idFrom)
    {
    case IDC_HASH_LIST:
      switch (phdr->code)
      {
      case NM_CUSTOMDRAW:
      {
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CustomDrawListView(lparam, phdr->hwndFrom));
        return TRUE;
      }
      case NM_DBLCLK:
      {
        const auto lpnmia = (LPNMITEMACTIVATE)lparam;
        LVHITTESTINFO lvhtinfo;
        lvhtinfo.pt = lpnmia->ptAction;
        ListView_SubItemHitTest(phdr->hwndFrom, &lvhtinfo);
        const auto subitem = lvhtinfo.iSubItem;
        TCHAR hash[MBEDTLS_MD_MAX_SIZE * 2 + 1];
        ListView_GetItemText(phdr->hwndFrom, lvhtinfo.iItem, ColIndex_Hash, hash, (int)std::size(hash));
        if(subitem == ColIndex_Hash)
        {
          utl::SetClipboardText(hwnd, hash);
        }
        else
        {
          TCHAR name[PATHCCH_MAX_CCH];
          ListView_GetItemText(phdr->hwndFrom, lvhtinfo.iItem, ColIndex_Filename, name, (int)std::size(name));
          utl::SetClipboardText(hwnd, (tstring{ hash } +_T(" *") + name).c_str());
        }
        break;
      }
      default:
        break;
      }
    default:
      break;
    }
    break;
  }
  case WM_COMMAND:
  {
    const auto code = HIWORD(wparam);
    const auto ctlid = LOWORD(wparam);
    const auto ctl = HWND(lparam);
    switch (ctlid)
    {
    case IDC_EDIT_HASH:
    {
      switch (code)
      {
      case EN_CHANGE:
      {
        TCHAR text[256];
        GetDlgItemText(hwnd, IDC_EDIT_HASH, text, (int)std::size(text));
        const auto find_hash = utl::HashStringToBytes(text);
        auto found = false;
        for (const auto file : _file_tasks)
        {
          const auto& result = file->GetHashResult();
          for (auto i = 0; i < k_hashers_count; ++i)
          {
            if (result[i] == find_hash)
            {
              found = true;
              const auto txt = tstring{ k_hashers_name[i] } +_T(" / ") + file->GetDisplayName();
              SetDlgItemText(hwnd, IDC_STATIC_CHECK_RESULT, txt.c_str());
              break;
            }
          }
          if (found)
            break;
        }
        if (!found)
          SetDlgItemText(hwnd, IDC_STATIC_CHECK_RESULT, utl::GetString(IDS_NOMATCH).c_str());

        break;
      }
      default:
        break;
      }
      break;
    }

    case IDC_BUTTON_CLIPBOARD:
    {
      switch (code)
      {
      case BN_CLICKED:
      {
        const auto sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_EXPORT));
        if (sel >= 0 && sel < k_hashers_count)
        {
          const auto tstr = utl::UTF8ToTString(GetSumfileAsString((size_t)sel).c_str());
          utl::SetClipboardText(hwnd, tstr.c_str());
        }
        break;
      }
      default:
        break;
      }
      break;
    }

    case IDC_BUTTON_EXPORT:
    {
      switch (code)
      {
      case BN_CLICKED:
      {
        const auto sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_EXPORT));
        if (sel >= 0 && sel < k_hashers_count)
        {
          // TODO: relativize sumfile contents to save path.
          // This may sound trivial at first, but we can't use PathRelativeToPath because it doesn't support long paths.
          
          const auto ext = tstring{ _T(".") } + hasher_get_extension_search_string(k_hashers[sel]);
          const auto& file = *_files.begin();
          const auto file_path = file.c_str();
          const auto file_name = (LPCTSTR)PathFindFileName(file_path);
          const auto dir = tstring{ file_path, file_name };
          const auto name = _files.size() == 1 ? (tstring{ file_name } + ext) : ext;
          const auto content = GetSumfileAsString((size_t)sel);
          const auto sumfile_path = utl::SaveDialog(hwnd, _base.c_str(), name.c_str());
          if(!sumfile_path.empty())
          {
            const auto err = utl::SaveMemoryAsFile(sumfile_path.c_str(), content.c_str(), content.size());
            if(err != ERROR_SUCCESS)
              utl::FormattedMessageBox(
                hwnd,
                _T("Error"),
                MB_ICONERROR | MB_OK,
                _T("utl::SaveMemoryAsFile returned with error: %08X"),
                err
              );
          }
        }
        break;
      }
      default:
        break;
      }
      break;
    }

    default:
      break;
    }
    break;
  }
  default:
    break;
  }

  return ret;
}

void OpenHashTabPropPage::AddFiles()
{
  if(_files.size() == 1)
  {
    auto& file = *_files.begin();
    const auto handle = utl::OpenForRead(file);
    if (handle != INVALID_HANDLE_VALUE)
    {
      FileSumList fsl;
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      if(!fsl.empty())
      {
        _is_sumfile = true;

        const auto sumfile_path = file.c_str();
        const auto sumfile_name = (LPCTSTR)PathFindFileName(sumfile_path);
        const auto sumfile_base_path = tstring{ sumfile_path, sumfile_name };
        for(auto& filesum : fsl)
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

/*void OpenHashTabPropPage::AddFile(const tstring& path, const std::vector<std::uint8_t>& expected_hash, tstring display_name)
{
  if (display_name.empty())
    display_name = tstring{ PathFindFileName(path.c_str()) };

  const auto task = new FileHashTask(path, this, std::move(display_name), expected_hash);
  _file_tasks.push_back(task);
  ++_files_not_finished;
}*/

void OpenHashTabPropPage::AddFile(const tstring& path, const std::vector<std::uint8_t>& expected_hash)
{
  auto dispname = utl::CanonicalizePath(path);

  // If path looks like _base + filename use filename as displayname, else the canonical name.
  // Optimally you'd use PathRelativePathTo for this, however that function not only doesn't support long paths, it also
  // doesn't have a PathCch alternative on Win8+. Additionally, seeing ".." and similar in the Name part could be confusing
  // to users, so printing full path instead is probably a better idea anyways.
  if(dispname.size() >= _base.size())
    if(std::equal(begin(_base), end(_base), begin(dispname)))
      dispname = dispname.substr(_base.size());

  const auto task = new FileHashTask(path, this, std::move(dispname), expected_hash);
  _file_tasks.push_back(task);
  ++_files_not_finished;
}

std::vector<std::uint8_t> OpenHashTabPropPage::TryGetExpectedSumForFile(const tstring& path)
{
  std::vector<std::uint8_t> hash{};

  const auto file = utl::OpenForRead(path);
  if (file == INVALID_HANDLE_VALUE)
    return hash;

  const auto file_path = path.c_str();
  const auto file_name = (LPCTSTR)PathFindFileName(file_path);
  const auto base_path = tstring{ file_path, file_name };

  for(auto hasher : k_hashers)
  {
    auto sumfile_path = path + _T(".") + hasher_get_extension_search_string(hasher);
    auto handle = utl::OpenForRead(sumfile_path);
    if(handle == INVALID_HANDLE_VALUE)
    {
      sumfile_path += _T("sum");
      handle = utl::OpenForRead(sumfile_path);
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
      FileSumList fsl;
      TryParseSumFile(handle, fsl);
      CloseHandle(handle);
      if(fsl.size() == 1)
      {
        const auto& file_sum = *fsl.begin();

        auto valid = false;

        if(file_sum.first.empty())
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

        if(valid)
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

void OpenHashTabPropPage::ProcessFiles()
{
  for(const auto task : _file_tasks)
    task->StartProcessing();
}

void OpenHashTabPropPage::Cancel()
{
  for (auto file : _file_tasks)
    file->SetCancelled();

  while (_files_not_finished > 0)
    _mm_pause();
}

void OpenHashTabPropPage::FileCompletionCallback(FileHashTask* file)
{
  std::lock_guard<std::mutex> guard(_list_view_lock);

  const auto files_being_processed = --_files_not_finished;

  if (_hwnd_deleted)
    return;

  const auto list = GetDlgItem(_hwnd, IDC_HASH_LIST);
  if (!list)
    return;

  const auto add_item = [list](LPCTSTR filename, LPCTSTR algorithm, LPCTSTR hash, LPARAM lparam)
  {
    LVITEM lvitem
    {
      LVIF_PARAM,
      INT_MAX,
      0,
      0,
      0,
      (LPTSTR)_T("")
    };
    lvitem.lParam = lparam;
    const auto item = ListView_InsertItem(list, &lvitem);
    ListView_SetItemText(list, item, ColIndex_Filename, (LPTSTR)filename);
    ListView_SetItemText(list, item, ColIndex_Algorithm, (PTSTR)algorithm);
    ListView_SetItemText(list, item, ColIndex_Hash, (LPTSTR)hash);
  };

  if(const auto error = file->GetError(); error == ERROR_SUCCESS)
  {
    const auto& results = file->GetHashResult();

    for (auto i = 0u; i < k_hashers_count; ++i)
    {
      auto& result = results[i];
      TCHAR hash_str[MBEDTLS_MD_MAX_SIZE * 2 + 1];
      utl::HashBytesToString(hash_str, result);
      add_item(file->GetDisplayName().c_str(), k_hashers_name[i], hash_str, file->ToLparam(i));
    }
  }
  else
  {
    TCHAR buf[1024];
    FormatMessage(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buf,
      _ARRAYSIZE(buf),
      nullptr
    );
    add_item(file->GetDisplayName().c_str(), utl::GetString(IDS_ERROR).c_str(), buf, file->ToLparam(0));
  }

  if(files_being_processed == 0)
  {
    Button_Enable(GetDlgItem(_hwnd, IDC_BUTTON_EXPORT), true);
    Button_Enable(GetDlgItem(_hwnd, IDC_BUTTON_CLIPBOARD), true);
    //SetTextFromTable(_hwnd, IDC_STATIC_PROCESSING, IDS_DONE);
    TCHAR done[64];
    size_t match = 0, mismatch = 0, none = 0, error = 0;
    for(auto file_task : _file_tasks)
    {
      if (file_task->GetError() != ERROR_SUCCESS)
        error++;
      else
        switch (file_task->GetMatchState())
        {
        case FileHashTask::MatchState_None:
          ++none;
          break;
        case FileHashTask::MatchState_Mismatch:
          ++mismatch;
          break;
        default:
          ++match;
          break;
        }
    }
    _stprintf_s(done, _T("%s (%zd/%zd/%zd/%zd)"), utl::GetString(IDS_DONE).c_str(), match, mismatch, none, error);
    SetDlgItemText(_hwnd, IDC_STATIC_PROCESSING, done);
  }
}

std::string OpenHashTabPropPage::GetSumfileAsString(size_t hasher)
{
  std::stringstream str;
  for(auto file : _file_tasks)
  {
    const auto& hash = file->GetHashResult()[hasher];
    char hash_str[MBEDTLS_MD_MAX_SIZE * 2 + 1];
    const auto size = (size_t)mbedtls_md_get_size(k_hashers[hasher]);
    if (hash.empty())
    {
      std::fill(hash_str, hash_str + size * 2, '0');
      hash_str[MBEDTLS_MD_MAX_SIZE * 2] = 0;
    }
    else
    {
      utl::HashBytesToString(hash_str, hash);
    }
    // force \r\n because clipboard expects that
    str << hash_str << " *" << utl::TStringToUTF8(file->GetDisplayName().c_str()) << "\r\n";
  }
  return str.str();
}