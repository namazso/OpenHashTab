#include "stdafx.h"


#include "Coordinator.h"
#include "MainDialog.h"
#include "utl.h"

HWND CreateDialogFromChildDialogResourceParam(
  _In_opt_  HINSTANCE hInstance,
  _In_      LPCTSTR   lpTemplateName,
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
  const auto hRsrc = FindResourceEx(hInstance, RT_DIALOG, lpTemplateName, 0);
  if (hRsrc)
  {
    const auto hGlobal = LoadResource(hInstance, hRsrc);
    if (hGlobal)
    {
      const auto pOrigTemplate = (const DLGTEMPLATEEX*)LockResource(hGlobal);
      if (pOrigTemplate)
      {
        const auto dwSize = SizeofResource(hInstance, hRsrc);
        if(dwSize)
        {
          const auto pTemplate = (DLGTEMPLATEEX*)malloc(dwSize);
          if(pTemplate)
          {
            memcpy(pTemplate, pOrigTemplate, dwSize);

            if(pTemplate->signature == 0xFFFF)
            {
              // I have NO IDEA what 0x48 mean, but apparently stuff works with this
              pTemplate->style = WS_POPUPWINDOW | WS_CAPTION | 0x00000048;
              pTemplate->exStyle = WS_EX_WINDOWEDGE;
            }
            else
            {
              ((DLGTEMPLATE*)pTemplate)->style = WS_POPUPWINDOW | WS_CAPTION | 0x00000048;
              ((DLGTEMPLATE*)pTemplate)->dwExtendedStyle = WS_EX_WINDOWEDGE;
            }

            hwnd = CreateDialogIndirectParam(hInstance, (LPDLGTEMPLATE)pTemplate, hWndParent, lpDialogFunc, dwInitParam);

            free(pTemplate);
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
//  _In_      HINSTANCE hInstance,
//  _In_opt_  HINSTANCE hPrevInstance,
//  _In_      LPCSTR    lpCmdLine,
//  _In_      int       nShowCmd
  _In_  HWND      hWnd,
  _In_  HINSTANCE hRunDLLInstance,
  _In_  LPCWSTR   lpCmdLine,
  _In_  int       nShowCmd
)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hRunDLLInstance);

  //utl::FormattedMessageBox(nullptr, _T("lpCmdLine"), MB_OK, _T("%s"), lpCmdLine);

  INITCOMMONCONTROLSEX iccex
  {
    sizeof(INITCOMMONCONTROLSEX),
    ICC_WIN95_CLASSES
  };
  InitCommonControlsEx(&iccex);

  int argc = 0;
  const auto argv = CommandLineToArgvW(lpCmdLine, &argc);
  std::list<tstring> files{argv, argv + argc};
  LocalFree(argv);

  if (files.empty())
    return;

  const auto& shortest = std::min_element(
    begin(files),
    end(files),
    [](const tstring& a, const tstring& b)
    {
      return a.size() < b.size();
    }
  );

  const auto pb = shortest->c_str();

  // if PathFindFileName it returns pb, making base path "". This is intended.
  auto base = tstring{ pb, (LPCWSTR)PathFindFileName(pb) };

  const auto coordinator = new StandaloneCoordinator(std::move(files), std::move(base));

  const auto dialog = CreateDialogFromChildDialogResourceParam(
    (HINSTANCE)&__ImageBase,
    MAKEINTRESOURCE(IDD_OPENHASHTAB_PROPPAGE),
    nullptr,
    &utl::DlgProcClassBinder<MainDialog>,
    (LPARAM)coordinator
  );
  ShowWindow(dialog, nShowCmd);

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (!IsDialogMessage(dialog, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // this will wait for hask tasks to gracefully terminate.
  // should we maybe just kill ourselves with ExitProcess
  // and let the OS do the cleanup?
  delete coordinator;
}