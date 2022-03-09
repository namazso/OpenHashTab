//    Copyright 2019-2022 namazso <admin@namazso.eu>
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
#include "resource.h"
#include "OpenHashTab_i.h"

#include <list>
#include <string>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE \
platform, such as the Windows Mobile platforms that do not include full DCOM \
support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to \
support creating single-thread COM object's and allow use of it's \
single-threaded COM object implementations. The threading model in your rgs \
file was set to 'Free' as that is the only threading model supported in non \
DCOM Windows CE platforms."
#endif

using namespace ATL;

class ATL_NO_VTABLE COpenHashTabShlExt :
  public CComObjectRootEx<CComSingleThreadModel>,
  public CComCoClass<COpenHashTabShlExt, &CLSID_OpenHashTabShlExt>,
  public IShellExtInit,
  public IShellPropSheetExt,
  public IContextMenu
{
protected:
  std::list<std::wstring> _files_raw;

public:
  COpenHashTabShlExt() = default;

  // IShellExtInit
  HRESULT STDMETHODCALLTYPE Initialize(
    _In_opt_  PCIDLIST_ABSOLUTE folder,
    _In_opt_  IDataObject*      data,
    _In_opt_  HKEY              prog_id
  ) override;

  // IShellPropSheetExt
  HRESULT STDMETHODCALLTYPE AddPages(
    _In_  LPFNSVADDPROPSHEETPAGE  add_page_proc,
    _In_  LPARAM                  lparam
  ) override;

  HRESULT STDMETHODCALLTYPE ReplacePage(
    _In_ EXPPS                  page_id,
    _In_ LPFNSVADDPROPSHEETPAGE replace_with_proc,
    _In_ LPARAM                 lparam
  ) override;
  
  // IContextMenu
  HRESULT STDMETHODCALLTYPE QueryContextMenu(
    _In_  HMENU hmenu,
    _In_  UINT indexMenu,
    _In_  UINT idCmdFirst,
    _In_  UINT idCmdLast,
    _In_  UINT uFlags
  ) override;

  HRESULT STDMETHODCALLTYPE InvokeCommand(
    _In_  CMINVOKECOMMANDINFO* pici
  ) override;

  HRESULT STDMETHODCALLTYPE GetCommandString(
    _In_  UINT_PTR idCmd,
    _In_  UINT uType,
    _Reserved_  UINT* pReserved,
    _Out_writes_bytes_((uType& GCS_UNICODE) ? (cchMax * sizeof(wchar_t)) : cchMax) _When_(!(uType& (GCS_VALIDATEA | GCS_VALIDATEW)), _Null_terminated_)  CHAR* pszName,
    _In_  UINT cchMax
  ) override;

DECLARE_REGISTRY_RESOURCEID(IDR_OPENHASHTABSHLEXT)

DECLARE_NOT_AGGREGATABLE(COpenHashTabShlExt)

BEGIN_COM_MAP(COpenHashTabShlExt)
  COM_INTERFACE_ENTRY(IShellExtInit)
  COM_INTERFACE_ENTRY(IShellPropSheetExt)
  COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct() { return S_OK; }

  void FinalRelease();
};

OBJECT_ENTRY_AUTO(__uuidof(OpenHashTabShlExt), COpenHashTabShlExt)
