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
#include "stdafx.h"

#include "OpenHashTab_i.h"
#include "dllmain.h"

#include <xutility>

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
