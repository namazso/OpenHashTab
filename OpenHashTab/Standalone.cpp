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

#include "Coordinator.h"
#include "MainDialog.h"
#include "path.h"
#include "utl.h"

HWND CreateDialogFromChildDialogResourceParam(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCWSTR   lpTemplateName,
  _In_opt_  HWND      hWndParent,
  _In_opt_  DLGPROC   lpDialogFunc,
  _In_      LPARAM    dwInitParam
)
{
  typedef struct {
    WORD      dlgVer;
    WORD      signature;
    DWORD     helpID;
    DWORD     exStyle;
    DWORD     style;
    WORD      cDlgItems;
    short     x;
    short     y;
    short     cx;
    short     cy;
    //sz_Or_Ord menu;
    //sz_Or_Ord windowClass;
    //WCHAR     title[titleLen];
    //WORD      pointsize;
    //WORD      weight;
    //BYTE      italic;
    //BYTE      charset;
    //WCHAR     typeface[stringLen];
  } DLGTEMPLATEEX;

  HWND hwnd = nullptr;
  const auto hRsrc = FindResourceExW(hInstance, RT_DIALOG, lpTemplateName, 0);
  if (hRsrc)
  {
    const auto hGlobal = LoadResource(hInstance, hRsrc);
    if (hGlobal)
    {
      const auto pOrigTemplate = static_cast<const DLGTEMPLATEEX*>(LockResource(hGlobal));
      if (pOrigTemplate)
      {
        const auto dwSize = SizeofResource(hInstance, hRsrc);
        if(dwSize)
        {
          const auto pTemplate = static_cast<DLGTEMPLATE*>(malloc(dwSize));
          const auto pTemplateEx = reinterpret_cast<DLGTEMPLATEEX*>(pTemplate);
          if(pTemplateEx)
          {
            memcpy(pTemplateEx, pOrigTemplate, dwSize);

            PDWORD pStyle, pexStyle;

            if(pTemplateEx->signature == 0xFFFF)
            {
              pStyle = &pTemplateEx->style;
              pexStyle = &pTemplateEx->exStyle;
            }
            else
            {
              pStyle = &pTemplate->style;
              pexStyle = &pTemplate->dwExtendedStyle;
            }

            // I have NO IDEA what 0x48 means, but without at least 0x40 the window just doesn't show.
            *pStyle = WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | 0x40;// | 0x8;
            *pexStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;

            hwnd = CreateDialogIndirectParamW(hInstance, pTemplate, hWndParent, lpDialogFunc, dwInitParam);

            free(pTemplateEx);
          }
        }
      }
      FreeResource(hGlobal);
    }
  }
  return hwnd;
}

// rundll32 OpenHashTab.dll,StandaloneEntry <args>
extern "C" __declspec(dllexport) void CALLBACK StandaloneEntryW(
  _In_  HWND      hWnd,
  _In_  HINSTANCE hRunDLLInstance,
  _In_  LPCWSTR   lpCmdLine,
  _In_  int       nShowCmd
)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hRunDLLInstance);

  //utl::FormattedMessageBox(nullptr, L"lpCmdLine", MB_OK, L"%s", lpCmdLine);

  INITCOMMONCONTROLSEX iccex
  {
    sizeof(INITCOMMONCONTROLSEX),
    ICC_WIN95_CLASSES
  };
  InitCommonControlsEx(&iccex);

  auto argc = 0;
  const auto argv = CommandLineToArgvW(lpCmdLine, &argc);
  const std::list<std::wstring> files{argv, argv + argc};
  LocalFree(argv);

  if (files.empty())
    return;

  const auto coordinator = new StandaloneCoordinator(files);

  const auto dialog = CreateDialogFromChildDialogResourceParam(
    utl::GetInstance(),
    MAKEINTRESOURCEW(IDD_OPENHASHTAB_PROPPAGE),
    hWnd,
    &wnd::DlgProcClassBinder<MainDialog>,
    reinterpret_cast<LPARAM>(coordinator)
  );
  ShowWindow(dialog, nShowCmd);

  MSG msg;

  // Main message loop:
  while (GetMessageW(&msg, nullptr, 0, 0))
  {
    if (!IsDialogMessageW(dialog, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  // this will wait for hask tasks to gracefully terminate.
  // should we maybe just kill ourselves with ExitProcess
  // and let the OS do the cleanup?
  delete coordinator;
}