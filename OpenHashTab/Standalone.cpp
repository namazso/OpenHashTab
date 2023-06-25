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
#include "path.h"
#include "utl.h"

// rundll32 OpenHashTab.dll,StandaloneEntry <args>
extern "C" __declspec(dllexport) int APIENTRY StandaloneEntryW(
  _In_opt_ HWND hWnd,
  _In_ HINSTANCE hRunDLLInstance,
  _In_ LPCWSTR lpCmdLine,
  _In_ int nShowCmd
) {
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hRunDLLInstance);

  auto argc = 0;
  const auto argv = CommandLineToArgvW(lpCmdLine, &argc);
  const std::list<std::wstring> files{argv, argv + argc};
  LocalFree(argv);

  if (files.empty())
    return 0;

  // TODO: Support per monitor / v2 DPI awareness too
  SetProcessDPIAware();

  INITCOMMONCONTROLSEX iccex{
    sizeof(INITCOMMONCONTROLSEX),
    ICC_WIN95_CLASSES | ICC_LINK_CLASS};
  InitCommonControlsEx(&iccex);

  const auto coordinator = new StandaloneCoordinator(files);

  const auto dialog = wnd::CreateDialogFromChildDialogResourceParam(
    utl::GetInstance(),
    MAKEINTRESOURCEW(IDD_OPENHASHTAB_PROPPAGE),
    hWnd,
    &wnd::DlgProcClassBinder<MainDialog>,
    reinterpret_cast<LPARAM>(coordinator)
  );
  ShowWindow(dialog, nShowCmd);

  MSG msg;

  // Main message loop:
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    if (!IsDialogMessageW(dialog, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  // this will wait for hash tasks to gracefully terminate.
  // should we maybe just kill ourselves with ExitProcess
  // and let the OS do the cleanup?
  delete coordinator;

  return (int)msg.wParam;
}
