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

// Beware: Most of these macros and include ordering are figured out via trying until it compiles.

#ifndef STRICT
#define STRICT
#endif

#define NOMINMAX

// thanks, microsoft
#define Unknown Unknown_FROM_WINDOWS

#include "targetver.h"

#define PHNT_VERSION PHNT_WIN7

#define ISOLATION_AWARE_ENABLED 1
#define SIDEBYSIDE_COMMONCONTROLS 1

#include <ntstatus.h>

#define WIN32_NO_STATUS

// ATL
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

// PHNT
#include <phnt_windows.h>
#include <phnt.h>

// Windows
#include <WinUser.h>
#include <CommCtrl.h>
#include <VersionHelpers.h>
#include <PathCch.h>

#undef Unknown
