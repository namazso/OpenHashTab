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

#include "OpenHashTab_i.h"
#include "dllmain.h"

#include <xutility>

static constexpr auto k_approved_reg_path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved";
static constexpr auto k_extension_name = L"OpenHashTab Shell Extension";

#define GUID_FORMAT "%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx"
#define GUID_ARG(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

void GetExtensionUUID(LPWSTR str, size_t len)
{
  const auto uuid = __uuidof(OpenHashTabShlExt);  // NOLINT(clang-diagnostic-language-extension-token)
  swprintf_s(str, len, L"{" _T(GUID_FORMAT) L"}", GUID_ARG(uuid));
}

// Used to determine whether the DLL can be unloaded by OLE.
_Use_decl_annotations_
STDAPI DllCanUnloadNow()
{
  return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
_Use_decl_annotations_
STDAPI DllGetClassObject(
  _In_      REFCLSID  rclsid,
  _In_      REFIID    riid,
  _Outptr_  LPVOID*   ppv
)
{
  return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - Adds entries to the system registry.
_Use_decl_annotations_
STDAPI DllRegisterServer()
{
  ATL::CRegKey reg;

  auto ret = reg.Open(
    HKEY_LOCAL_MACHINE,
    k_approved_reg_path,
    KEY_SET_VALUE
  );

  if (ERROR_SUCCESS != ret)
    return ret;

  wchar_t uuid[128];
  GetExtensionUUID(uuid, std::size(uuid));
  ret = reg.SetStringValue(k_extension_name, uuid);

  reg.Close();

  if (ERROR_SUCCESS != ret)
    return ret;

  // registers object, typelib and all interfaces in typelib
  return _AtlModule.RegisterServer(false);
}

// DllUnregisterServer - Removes entries from the system registry.
_Use_decl_annotations_
STDAPI DllUnregisterServer()
{
  ATL::CRegKey reg;

  const auto ret = reg.Open(
    HKEY_LOCAL_MACHINE,
    k_approved_reg_path,
    KEY_SET_VALUE
  );

  if (ERROR_SUCCESS == ret)
  {
    wchar_t uuid[128];
    GetExtensionUUID(uuid, std::size(uuid));
    reg.DeleteValue(uuid);
    reg.Close();
  }

  return _AtlModule.UnregisterServer(false);
}

// DllInstall - Adds/Removes entries to the system registry per user per machine.
STDAPI DllInstall(
  _In_      BOOL    install,
  _In_opt_  LPCWSTR cmd_line
)
{
  static constexpr wchar_t k_user_switch[] = L"user";

  if (cmd_line && _wcsnicmp(cmd_line, k_user_switch, std::size(k_user_switch)) == 0)
    ATL::AtlSetPerUserRegistration(true);

  auto hr = E_FAIL;

  if (install)
  {
    hr = DllRegisterServer();
    if (FAILED(hr))
      DllUnregisterServer();
  }
  else
  {
    hr = DllUnregisterServer();
  }

  return hr;
}


