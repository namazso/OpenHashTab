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
#include "dllmain.h"

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

COpenHashTabModule _AtlModule;

// DLL Entry Point
EXTERN_C BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
  UNREFERENCED_PARAMETER(instance);
  return _AtlModule.DllMain(reason, reserved);
}
