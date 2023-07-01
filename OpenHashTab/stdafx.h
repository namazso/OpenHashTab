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
#pragma once

// Beware: Most of these macros and include ordering are figured out via trying until it compiles.

#ifndef STRICT
#define STRICT
#endif

#define NOMINMAX

// thanks, microsoft
#define Unknown Unknown_FROM_WINDOWS

#include <WinSDKVer.h>

// Windows 7
#define _WIN32_WINNT 0x0601

#include <SDKDDKVer.h>

#define ISOLATION_AWARE_ENABLED   1
#define SIDEBYSIDE_COMMONCONTROLS 1

#include <ntstatus.h>

#define WIN32_NO_STATUS

// ATL
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"

// Windows
#include <Windows.h>

#include <winternl.h>

#include <windowsx.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <CommCtrl.h>
#include <oaidl.h>
#include <ocidl.h>
#include <PathCch.h>
#include <shobjidl.h>
#include <VersionHelpers.h>
#include <winhttp.h>
#include <WinUser.h>

#undef Unknown

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <xutility>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#include <ctre-unicode.hpp>
#pragma clang diagnostic pop

EXTERN_C const IID LIBID_OpenHashTabLib;
EXTERN_C const IID IID_IOpenHashTabShlExt;
EXTERN_C const IID CLSID_OpenHashTabShlExt;

#undef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
  ((std::add_pointer_t<type>)((ULONG_PTR)(address)-offsetof(type, field)))
