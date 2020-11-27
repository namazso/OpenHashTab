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

            // I have NO IDEA what 0x48 mean, but apparently stuff works with this
            *pStyle = WS_POPUPWINDOW | WS_CAPTION | 0x00000048 | WS_THICKFRAME;
            *pexStyle = WS_EX_WINDOWEDGE;

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
    &utl::DlgProcClassBinder<MainDialog>,
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