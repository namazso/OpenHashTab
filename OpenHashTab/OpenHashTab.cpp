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
#include "MainDialog.h"
#include "utl.h"

#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error \
  "Single-threaded COM objects are not properly supported on Windows CE \
platform, such as the Windows Mobile platforms that do not include full DCOM \
support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to \
support creating single-thread COM object's and allow use of it's \
single-threaded COM object implementations. The threading model in your rgs \
file was set to 'Free' as that is the only threading model supported in non \
DCOM Windows CE platforms."
#endif

using namespace ATL;

class COpenHashTabModule : public ATL::CAtlDllModuleT<COpenHashTabModule> {
public:
  DECLARE_LIBID(LIBID_OpenHashTabLib)
};

COpenHashTabModule _AtlModule;

class ATL_NO_VTABLE COpenHashTabShlExt : public CComObjectRootEx<CComSingleThreadModel>
    , public CComCoClass<COpenHashTabShlExt, &CLSID_OpenHashTabShlExt>
    , public IShellExtInit
    , public IShellPropSheetExt
    , public IContextMenu {
protected:
  std::list<std::wstring> _files_raw;

public:
  COpenHashTabShlExt() = default;

  // IShellExtInit
  HRESULT STDMETHODCALLTYPE Initialize(
    _In_opt_ PCIDLIST_ABSOLUTE folder,
    _In_opt_ IDataObject* data,
    _In_opt_ HKEY prog_id
  ) override;

  // IShellPropSheetExt
  HRESULT STDMETHODCALLTYPE AddPages(
    _In_ LPFNSVADDPROPSHEETPAGE add_page_proc,
    _In_ LPARAM lparam
  ) override;

  HRESULT STDMETHODCALLTYPE ReplacePage(
    _In_ EXPPS page_id,
    _In_ LPFNSVADDPROPSHEETPAGE replace_with_proc,
    _In_ LPARAM lparam
  ) override;

  // IContextMenu
  HRESULT STDMETHODCALLTYPE QueryContextMenu(
    _In_ HMENU hmenu,
    _In_ UINT indexMenu,
    _In_ UINT idCmdFirst,
    _In_ UINT idCmdLast,
    _In_ UINT uFlags
  ) override;

  HRESULT STDMETHODCALLTYPE InvokeCommand(
    _In_ CMINVOKECOMMANDINFO* pici
  ) override;

  HRESULT STDMETHODCALLTYPE GetCommandString(
    _In_ UINT_PTR idCmd,
    _In_ UINT uType,
    _Reserved_ UINT* pReserved,
    _Out_writes_bytes_((uType & GCS_UNICODE) ? (cchMax * sizeof(wchar_t)) : cchMax) _When_(!(uType & (GCS_VALIDATEA | GCS_VALIDATEW)), _Null_terminated_) CHAR* pszName,
    _In_ UINT cchMax
  ) override;

  static HRESULT WINAPI UpdateRegistry(_In_ BOOL bRegister) throw() {
    UNREFERENCED_PARAMETER(bRegister);
    return S_OK;
  }

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

class DECLSPEC_UUID("23b5bdd4-7669-42b8-9cdc-beebc8a5baa9") OpenHashTabShlExt;

OBJECT_ENTRY_AUTO(__uuidof(OpenHashTabShlExt), COpenHashTabShlExt)

// COpenHashTabShlExt

HRESULT STDMETHODCALLTYPE COpenHashTabShlExt::Initialize(
  _In_opt_ PCIDLIST_ABSOLUTE folder,
  _In_opt_ IDataObject* data,
  _In_opt_ HKEY prog_id
) {
  UNREFERENCED_PARAMETER(folder);
  UNREFERENCED_PARAMETER(prog_id);

  // Init the common controls.
  INITCOMMONCONTROLSEX iccex{
    sizeof(INITCOMMONCONTROLSEX),
    ICC_WIN95_CLASSES};
  InitCommonControlsEx(&iccex);

  // Read the list of folders from the data object. They're stored in HDROP
  // form, so just get the HDROP handle and then use the drag 'n' drop APIs
  // on it.
  FORMATETC etc{
    CF_HDROP,
    nullptr,
    DVASPECT_CONTENT,
    -1,
    TYMED_HGLOBAL};
  STGMEDIUM stg;
  if (FAILED(data->GetData(&etc, &stg)))
    return E_INVALIDARG;

  // Get an HDROP handle.
  const auto drop = static_cast<HDROP>(GlobalLock(stg.hGlobal));

  if (!drop) {
    ReleaseStgMedium(&stg);
    return E_INVALIDARG;
  }

  // Determine how many files are involved in this operation.
  const auto file_count = DragQueryFileW(
    drop,
    0xFFFFFFFF,
    nullptr,
    0
  );

  const auto file_name_buf = std::make_unique<wchar_t[]>(PATHCCH_MAX_CCH);

  for (auto i = 0u; i < file_count; i++) {
    // Get the next filename.
    if (0 == DragQueryFileW(drop, i, file_name_buf.get(), static_cast<UINT>(PATHCCH_MAX_CCH)))
      continue;

    // Add the filename to our list of files to act on.
    _files_raw.emplace_back(file_name_buf.get());
  }

  // Release resources.
  GlobalUnlock(stg.hGlobal);
  ReleaseStgMedium(&stg);

  // If we found any files we can work with, return S_OK.  Otherwise,
  // return E_FAIL, so we don't get called again for this right-click
  // operation.
  return _files_raw.empty() ? E_FAIL : S_OK;
}

void COpenHashTabShlExt::FinalRelease() {
}

// IShellPropSheetExt

HRESULT STDMETHODCALLTYPE COpenHashTabShlExt::AddPages(
  _In_ LPFNSVADDPROPSHEETPAGE add_page_proc,
  _In_ LPARAM lparam
) {
  // We shouldn't ever get called with empty files, since Initialize should
  // return failure. So if we somehow do, just don't add any pages.
  assert(!_files_raw.empty());
  if (_files_raw.empty())
    return S_OK;

  const auto tab_name = utl::GetString(IDS_HASHES);

  // Set up everything but pfnDlgProc, pfnCallback, lParam which will be set by MakePropPage to call members
  // functions on the page object
  PROPSHEETPAGEW psp{};
  psp.dwSize = sizeof(PROPSHEETPAGEW);
  psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
  psp.hInstance = _AtlBaseModule.GetResourceInstance();
  psp.pszTemplate = MAKEINTRESOURCEW(IDD_OPENHASHTAB_PROPPAGE);
  psp.pszTitle = tab_name.c_str();
  psp.pcRefParent = reinterpret_cast<UINT*>(&_AtlModule.m_nLockCnt);

  const auto hpage = wnd::MakePropPage<PropPageCoordinator, MainDialog>(psp, _files_raw);

  if (hpage) {
    // Call the shell's callback function, so it adds the page to
    // the property sheet.
    if (!add_page_proc(hpage, lparam))
      DestroyPropertySheetPage(hpage);
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE COpenHashTabShlExt::ReplacePage(
  _In_ EXPPS page_id,
  _In_ LPFNSVADDPROPSHEETPAGE replace_with_proc,
  _In_ LPARAM lparam
) {
  UNREFERENCED_PARAMETER(page_id);
  UNREFERENCED_PARAMETER(replace_with_proc);
  UNREFERENCED_PARAMETER(lparam);

  return E_NOTIMPL;
}

// IContextMenu

HRESULT COpenHashTabShlExt::QueryContextMenu(
  _In_ HMENU hmenu,
  _In_ UINT indexMenu,
  _In_ UINT idCmdFirst,
  _In_ UINT idCmdLast,
  _In_ UINT uFlags
) {
  UNREFERENCED_PARAMETER(indexMenu);
  UNREFERENCED_PARAMETER(idCmdLast);

  // If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
  if (uFlags & CMF_DEFAULTONLY)
    return S_OK;

  // If invoked on a shortcut, don't add options - the user probably doesn't want to hash the shortcut
  if (uFlags & CMF_VERBSONLY)
    return S_OK;

  InsertMenuW(
    hmenu,
    indexMenu,
    MF_BYPOSITION,
    idCmdFirst,
    utl::GetString(IDS_HASHES).c_str()
  );

  return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1);
}

HRESULT COpenHashTabShlExt::InvokeCommand(
  _In_ CMINVOKECOMMANDINFO* pici
) {
  if (IS_INTRESOURCE(pici->lpVerb) && (UINT)(UINT_PTR)pici->lpVerb == 0) {
    const auto coordinator = new WindowedCoordinator(_files_raw);

    const auto dialog = wnd::CreateDialogFromChildDialogResourceParam(
      utl::GetInstance(),
      MAKEINTRESOURCEW(IDD_OPENHASHTAB_PROPPAGE),
      pici->hwnd,
      &wnd::DlgProcClassBinder<MainDialog>,
      reinterpret_cast<LPARAM>(coordinator)
    );
    ShowWindow(dialog, SW_SHOW);

    return S_OK;
  }

  return E_INVALIDARG;
}

HRESULT COpenHashTabShlExt::GetCommandString(
  _In_ UINT_PTR idCmd,
  _In_ UINT uType,
  _Reserved_ UINT* pReserved,
  _Out_writes_bytes_((uType & GCS_UNICODE) ? (cchMax * sizeof(wchar_t)) : cchMax) _When_(!(uType & (GCS_VALIDATEA | GCS_VALIDATEW)), _Null_terminated_) CHAR* pszName,
  _In_ UINT cchMax
) {
  UNREFERENCED_PARAMETER(pReserved);

  // Check idCmd, it must be 0 since we have only one menu item.
  if (0 != idCmd)
    return E_INVALIDARG;

  if (uType == GCS_HELPTEXTW) {
    wcscpy_s((LPWSTR)pszName, cchMax, L"Is this even displayed anywhere??");
    return S_OK;
  }

  return E_INVALIDARG;
}

// Used to determine whether the DLL can be unloaded by OLE.
_Use_decl_annotations_ STDAPI DllCanUnloadNow() {
  return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
_Use_decl_annotations_ STDAPI DllGetClassObject(
  _In_ REFCLSID rclsid,
  _In_ REFIID riid,
  _Outptr_ LPVOID* ppv
) {
  return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

EXTERN_C constexpr IID LIBID_OpenHashTabLib = {
  0x715dae2b,
  0x0063,
  0x4d88,
  {0x8f, 0x94, 0x3d, 0xc0, 0x72, 0xfc, 0x3f, 0xb0}
};

EXTERN_C constexpr IID IID_IOpenHashTabShlExt = {
  0x50b2e14f,
  0x21c8,
  0x4a3c,
  {0x9a, 0x25, 0x3d, 0x33, 0x56, 0x17, 0x85, 0x49}
};

EXTERN_C constexpr CLSID CLSID_OpenHashTabShlExt = {
  0x23b5bdd4,
  0x7669,
  0x42b8,
  {0x9c, 0xdc, 0xbe, 0xeb, 0xc8, 0xa5, 0xba, 0xa9}
};

// DLL Entry Point
EXTERN_C BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

EXTERN_C BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  UNREFERENCED_PARAMETER(instance);
  return _AtlModule.DllMain(reason, reserved);
}
