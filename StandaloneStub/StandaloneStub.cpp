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
