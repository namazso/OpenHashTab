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

#include "MainDialog.h"
#include "Coordinator.h"
#include "Settings.h"
#include "utl.h"
#include "SettingsDialog.h"
#include "FileHashTask.h"
#include "wnd.h"

enum class HashColorType
{
  Error,
  Match,
  Insecure,
  Mismatch,
  Unknown
};

static bool ColorLine(LPNMLVCUSTOMDRAW plvcd, HashColorType type)
{
  // No hash to compare to  - system colors
  // Error processing file  - system bg, red text
  // Hash mismatch          - red bg, white text for all algos
  // Secure hash matches    - green bg, white text for algo matching
  // Insecure hash matches  - orange bg, white text for algo matching
  
  switch(type)
  {
  case HashColorType::Unknown:
    return false;
  case HashColorType::Error:
    plvcd->clrText = RGB(255, 55, 23);
    break;
  case HashColorType::Match:
    plvcd->clrText = RGB(255, 255, 255);
    plvcd->clrTextBk = RGB(45, 170, 23);
    break;
  case HashColorType::Insecure:
    plvcd->clrText = RGB(255, 255, 255);
    plvcd->clrTextBk = RGB(170, 82, 23);
    break;
  case HashColorType::Mismatch:
    plvcd->clrText = RGB(255, 255, 255);
    plvcd->clrTextBk = RGB(230, 55, 23);
    break;
  }
  return true;
}

static HashColorType HashColorTypeForFile(FileHashTask* file, size_t hasher)
{
  if (file->GetError() != ERROR_SUCCESS)
    return HashColorType::Error;

  const auto match = file->GetMatchState();
  if (match == FileHashTask::MatchState_Mismatch)
    return HashColorType::Mismatch;

  if (match != FileHashTask::MatchState_None && (size_t)match == hasher)
  {
    if (HashAlgorithm::g_hashers[(size_t)match].IsSecure())
      return HashColorType::Match;
    
    return HashColorType::Insecure;
  }
  return HashColorType::Unknown;
}

static void SetTextFromTable(HWND hwnd, UINT string_id)
{
  SetWindowTextW(hwnd, utl::GetString(string_id).c_str());
}

static int ComboBoxGetSelectedAlgorithmIdx(HWND combo)
{
  const auto sel = ComboBox_GetCurSel(combo);
  const auto len = ComboBox_GetLBTextLen(combo, sel);
  std::wstring name;
  name.resize(len);
  ComboBox_GetLBText(combo, sel, name.data());
  const auto idx = HashAlgorithm::IdxByName(utl::TStringToUTF8(name.c_str()));
  return idx;
}

MainDialog::MainDialog(HWND hwnd, void* prop_page)
  : _hwnd(hwnd)
  , _prop_page((Coordinator*)prop_page)
{
  _prop_page->RegisterWindow(hwnd);
}

MainDialog::~MainDialog()
{
  _prop_page->Cancel(false);
  _prop_page->UnregisterWindow();
}

INT_PTR MainDialog::CustomDrawListView(LPARAM lparam, HWND list) const
{
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
      LVITEMW lvitem
      {
        LVIF_PARAM,
        (int)lplvcd->nmcd.dwItemSpec
      };
      ListView_GetItem(list, &lvitem);
      const auto file_hash = FileHashTask::FromLparam(lvitem.lParam);
      const auto file = file_hash.first;
      const auto hasher = file_hash.second;
      const auto color_type = HashColorTypeForFile(file, hasher);
      if(ColorLine(lplvcd, color_type))
        return CDRF_NEWFONT;

      // fall through for normal color
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

INT_PTR MainDialog::DlgProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
  auto ret = FALSE;
  DebugMsg("WndProc uMsg: %04X wParam: %08X lParam: %016X\n", msg, wparam, lparam);
  switch (msg)
  {
  case WM_INITDIALOG:
    InitDialog();
    break;

  case wnd::WM_USER_FILE_FINISHED:
    if (wparam == wnd::k_user_magic_wparam)
      OnFileFinished((FileHashTask*)lparam);
    break;

  case wnd::WM_USER_ALL_FILES_FINISHED:
    if (wparam == wnd::k_user_magic_wparam)
      OnAllFilesFinished();
    break;

  case wnd::WM_USER_FILE_PROGRESS:
    if (wparam == wnd::k_user_magic_wparam)
      SendMessageW(_hwnd_PROGRESS, PBM_SETPOS, lparam, 0);
    break;

  case WM_TIMER:
    if (wparam == k_status_update_timer_id)
      UpdateDefaultStatus(true);
    break;

  case WM_NOTIFY:
  {
    const auto phdr = (LPNMHDR)lparam;
    const auto list = phdr->hwndFrom;
    switch (phdr->idFrom)
    {
    case IDC_HASH_LIST:
      switch (phdr->code)
      {
      case NM_CUSTOMDRAW:
        SetWindowLongPtrW(_hwnd, DWLP_MSGRESULT, CustomDrawListView(lparam, list));
        return TRUE;

      case NM_DBLCLK:
      {
        const auto lpnmia = (LPNMITEMACTIVATE)lparam;
        LVHITTESTINFO lvhtinfo = { lpnmia->ptAction };
        ListView_SubItemHitTest(list, &lvhtinfo);
        OnListDoubleClick(lvhtinfo.iItem, lvhtinfo.iSubItem);
        break;
      }
      case NM_RCLICK:
        OnListRightClick(false);
        break;

      case NM_RDBLCLK:
        OnListRightClick(true);
        break;

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
    switch (ctlid)
    {
    case IDC_EDIT_HASH:
      if (code == EN_CHANGE)
        OnHashEditChanged();
      break;

    case IDC_BUTTON_CLIPBOARD:
      if (code == BN_CLICKED)
      {
        const auto idx = ComboBoxGetSelectedAlgorithmIdx(_hwnd_COMBO_EXPORT);
        if (idx >= 0)
        {
          const auto wstr = utl::UTF8ToTString(GetSumfileAsString((size_t)idx).c_str());
          utl::SetClipboardText(_hwnd, wstr.c_str());
        }
      }
      break;

    case IDC_BUTTON_SETTINGS:
    {
      if (code == BN_CLICKED)
        DialogBoxParamW(
          ATL::_AtlBaseModule.GetResourceInstance(),
          MAKEINTRESOURCEW(IDD_SETTINGS),
          _hwnd,
          &utl::DlgProcClassBinder<SettingsDialog>,
          0
        );
      break;
    }

    case IDC_BUTTON_EXPORT:
    {
      if (code == BN_CLICKED)
        OnExportClicked();
      break;
    }

    default:
      break;
    }
    break;
  }

  // we should never ever receive WM_CLOSE when running as a property sheet, so DestroyWindow is safe here
  case WM_CLOSE:
    DestroyWindow(_hwnd);
    break;

  default:
    break;
  }

  return ret;
}

void MainDialog::InitDialog()
{
  // TODO: figure out a way to dynamically resize this thing
  //RECT rect;
  //GetClientRect(_hwnd, &rect);
  //SetWindowPos(_hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  SetTextFromTable(_hwnd_STATIC_CHECK_AGAINST, IDS_CHECK_AGAINST);
  SetTextFromTable(_hwnd_STATIC_EXPORT_TO, IDS_EXPORT_TO);
  SetTextFromTable(_hwnd_BUTTON_EXPORT, IDS_EXPORT_BTN);
  SetTextFromTable(_hwnd_STATIC_PROCESSING, IDS_PROCESSING);
  SetTextFromTable(_hwnd_BUTTON_CLIPBOARD, IDS_CLIPBOARD);

  if (IsWindows8OrGreater())
    SetWindowTextW(_hwnd_BUTTON_SETTINGS, L"\u2699");

  SendMessageW(_hwnd_HASH_LIST, LVM_SETTEXTBKCOLOR, 0, (LPARAM)CLR_NONE);
  SendMessageW(_hwnd_HASH_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

  // we put the string table ID in the text length field, to fix it up later
  LVCOLUMN cols[] =
  {
    {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 140,   nullptr, IDS_FILENAME},
    {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 70,    nullptr, IDS_ALGORITHM},
    {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 1100,  nullptr, IDS_HASH},
  };

  for (auto i = 0u; i < std::size(cols); ++i)
  {
    auto& col = cols[i];
    const auto wstr = utl::GetString(col.cchTextMax);
    col.pszText = (LPWSTR)wstr.c_str();
    ListView_InsertColumn(_hwnd_HASH_LIST, i, &cols[i]);
  }

  const auto combobox = _hwnd_COMBO_EXPORT;
  for (const auto& algorithm : HashAlgorithm::g_hashers)
    if (Settings::instance.IsHashEnabled(&algorithm))
      ComboBox_AddString(combobox, utl::UTF8ToTString(algorithm.GetName()).c_str());

  ComboBox_SetCurSel(combobox, 0);

  SendMessageW(_hwnd_PROGRESS, PBM_SETRANGE32, 0, Coordinator::k_progress_resolution);

  _prop_page->AddFiles();

  if (_prop_page->IsSumfile())
    SetTextFromTable(_hwnd_STATIC_SUMFILE, IDS_SUMFILE);

  if (_prop_page->GetFiles().size() == 1)
    ListView_SetColumnWidth(_hwnd_HASH_LIST, ColIndex_Filename, 0);

  _prop_page->ProcessFiles();
}

void MainDialog::OnFileFinished(FileHashTask* file)
{
  const auto list = _hwnd_HASH_LIST;
  if (!list)
    return;

  const auto add_item = [list](LPCWSTR filename, LPCWSTR algorithm, LPCWSTR hash, LPARAM lparam)
  {
    LVITEMW lvitem
    {
      LVIF_PARAM,
      INT_MAX,
      0,
      0,
      0,
      (LPWSTR)L""
    };
    lvitem.lParam = lparam;
    const auto item = ListView_InsertItem(list, &lvitem);
    ListView_SetItemText(list, item, ColIndex_Filename, (LPWSTR)filename);
    ListView_SetItemText(list, item, ColIndex_Algorithm, (LPWSTR)algorithm);
    ListView_SetItemText(list, item, ColIndex_Hash, (LPWSTR)hash);
  };

  if (const auto error = file->GetError(); error == ERROR_SUCCESS)
  {
    switch (file->GetMatchState())
    {
    case FileHashTask::MatchState_None:
      ++_count_unknown;
      break;
    case FileHashTask::MatchState_Mismatch:
      ++_count_mismatch;
      break;
    default:
      ++_count_match;
      break;
    }

    const auto& results = file->GetHashResult();

    for (auto i = 0u; i < HashAlgorithm::k_count; ++i)
    {
      auto& result = results[i];
      if (!result.empty())
      {
        wchar_t hash_str[HashAlgorithm::k_max_size * 2 + 1];
        utl::HashBytesToString(hash_str, result);
        const auto tname = utl::UTF8ToTString(HashAlgorithm::g_hashers[i].GetName());
        add_item(file->GetDisplayName().c_str(), tname.c_str(), hash_str, file->ToLparam(i));
      }
    }
  }
  else
  {
    ++_count_error;
    add_item(
      file->GetDisplayName().c_str(),
      utl::GetString(IDS_ERROR).c_str(),
      utl::ErrorToString(error).c_str(),
      file->ToLparam(0)
    );
  }

  UpdateDefaultStatus();
}

void MainDialog::OnAllFilesFinished()
{
  _finished = true;

  // We only enable settings button after processing is done because changing enabled algorithms could result
  // in much more problems
  Button_Enable(_hwnd_BUTTON_SETTINGS, true);
  Button_Enable(_hwnd_BUTTON_EXPORT, true);
  Button_Enable(_hwnd_BUTTON_CLIPBOARD, true);
  Edit_Enable(_hwnd_EDIT_HASH, true);

  /*SendMessage(
    _hwnd_PROGRESS,
    PBM_SETPOS,
    Coordinator::k_progress_resolution,
    Coordinator::k_progress_resolution
  );*/

  ShowWindow(_hwnd_PROGRESS, 0);

  UpdateDefaultStatus();

  const auto clip = utl::GetClipboardText(_hwnd);
  const auto find_hash = utl::HashStringToBytes(clip.c_str());
  if (find_hash.size() >= 4) // at least 4 bytes for a valid hash
  {
    SetWindowTextW((_hwnd_EDIT_HASH), (clip.c_str()));
    OnHashEditChanged(); // fake a change as if the user pasted it
  }
}

void MainDialog::OnExportClicked()
{
  const auto idx = ComboBoxGetSelectedAlgorithmIdx(_hwnd_COMBO_EXPORT);
  if (idx >= 0 && !_prop_page->GetFiles().empty())
  {
    // TODO: relativize sumfile contents to save path.
    // This may sound trivial at first, but we can't use PathRelativeToPath because it doesn't support long paths.

    const auto exts = HashAlgorithm::g_hashers[idx].GetExtensions();
    const auto ext = *exts ? std::wstring{ L"." } +utl::UTF8ToTString(*exts) : std::wstring{};
    const auto path_and_basename = _prop_page->GetSumfileDefaultSavePathAndBaseName();
    const auto name = path_and_basename.second + ext;
    const auto content = GetSumfileAsString((size_t)idx);
    const auto sumfile_path = utl::SaveDialog(_hwnd, path_and_basename.first.c_str(), name.c_str());
    if (!sumfile_path.empty())
    {
      const auto err = utl::SaveMemoryAsFile(sumfile_path.c_str(), content.c_str(), content.size());
      if (err != ERROR_SUCCESS)
        utl::FormattedMessageBox(
          _hwnd,
          L"Error",
          MB_ICONERROR | MB_OK,
          L"utl::SaveMemoryAsFile returned with error: %08X",
          err
        );
    }
  }
}

void MainDialog::OnHashEditChanged()
{
  const auto find_hash = utl::HashStringToBytes(utl::GetWindowTextString(_hwnd_EDIT_HASH).c_str());
  auto found = false;
  for (const auto& file : _prop_page->GetFiles())
  {
    const auto& result = file->GetHashResult();
    for (auto i = 0; i < HashAlgorithm::k_count; ++i)
    {
      if (!result[i].empty() && result[i] == find_hash)
      {
        found = true;
        const auto algorithm_name = utl::UTF8ToTString(HashAlgorithm::g_hashers[i].GetName());
        const auto txt = algorithm_name + L" / " + file->GetDisplayName();
        SetWindowTextW(_hwnd_STATIC_CHECK_RESULT, txt.c_str());
        break;
      }
    }
    if (found)
      break;
  }
  if (!found)
    SetWindowTextW(_hwnd_STATIC_CHECK_RESULT, utl::GetString(IDS_NOMATCH).c_str());
}

void MainDialog::OnListDoubleClick(int item, int subitem)
{
  const auto list = _hwnd_HASH_LIST;
  wchar_t hash[4096]; // It's possible it will hold an error message
  ListView_GetItemText(list, item, ColIndex_Hash, hash, (int)std::size(hash));
  if (subitem == ColIndex_Hash)
  {
    utl::SetClipboardText(_hwnd, hash);
  }
  else
  {
    const auto name = std::make_unique<std::array<wchar_t, PATHCCH_MAX_CCH>>();
    ListView_GetItemText(list, item, ColIndex_Filename, name->data(), (int)name->size());
    utl::SetClipboardText(_hwnd, (std::wstring{ hash } +L" *" + name->data()).c_str());
  }
  SetTempStatus(utl::GetString(IDS_COPIED).c_str(), 1000);
}

void MainDialog::OnListRightClick(bool dblclick)
{
  const auto list = _hwnd_HASH_LIST;
  const auto count = ListView_GetItemCount(list);
  const auto buf = std::make_unique<std::array<wchar_t, PATHCCH_MAX_CCH>>();
  std::basic_stringstream<wchar_t> clipboard;
  const auto list_get_text = [&](int idx, int subitem)
  {
    ListView_GetItemText(list, idx, subitem, buf->data(), (int)buf->size());
    return buf->data();
  };
  for (auto i = 0; i < count; ++i)
  {
    if (!dblclick && !ListView_GetItemState(list, i, LVIS_SELECTED))
      continue;

    clipboard
      << list_get_text(i, ColIndex_Filename) << L"\t"
      << list_get_text(i, ColIndex_Algorithm) << L"\t"
      << list_get_text(i, ColIndex_Hash) << L"\r\n";
  }
  utl::SetClipboardText(_hwnd, clipboard.str().c_str());
  SetTempStatus(utl::GetString(IDS_COPIED).c_str(), 1000);
}

std::string MainDialog::GetSumfileAsString(size_t hasher)
{
  std::stringstream str;
  for (const auto& file : _prop_page->GetFiles())
  {
    const auto& hash = file->GetHashResult()[hasher];
    char hash_str[HashAlgorithm::k_max_size * 2 + 1];
    const auto size = HashAlgorithm::g_hashers[hasher].GetSize();
    if (hash.empty())
    {
      std::fill(hash_str, hash_str + size * 2, '0');
      hash_str[size * 2] = 0;
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

void MainDialog::SetTempStatus(LPCWSTR status, UINT time)
{
  _temporary_status = true;
  SetWindowTextW(_hwnd_STATIC_PROCESSING, status);
  SetTimer(_hwnd, k_status_update_timer_id, time, nullptr);
}

void MainDialog::UpdateDefaultStatus(bool force_reset)
{
  if (force_reset)
    _temporary_status = false;

  if (!_temporary_status)
  {
    const auto msg = _finished ? IDS_DONE : IDS_PROCESSING;
    wchar_t done[64];
    swprintf_s(
      done,
      L"%s (%u/%u/%u/%u)",
      utl::GetString(msg).c_str(),
      _count_match,
      _count_mismatch,
      _count_unknown,
      _count_error
    );
    SetWindowTextW(_hwnd_STATIC_PROCESSING, done);
  }
}