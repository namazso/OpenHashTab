//    Copyright 2019-2025 namazso <admin@namazso.eu>
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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

extern "C" __declspec(dllimport) int APIENTRY StandaloneEntryW(
  _In_opt_ HWND hWnd,
  _In_ HINSTANCE hRunDLLInstance,
  _In_ LPCWSTR lpCmdLine,
  _In_ int nShowCmd
);

int APIENTRY wWinMain(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR lpCmdLine,
  _In_ int nCmdShow
) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  return StandaloneEntryW(nullptr, hInstance, lpCmdLine, nCmdShow);
}
