#pragma once
namespace wnd
{
  // apparently you can get random WM_USER messages from malfunctioning other apps
  static constexpr auto k_user_magic_wparam = (WPARAM)0x1c725fcfdcbf5843;

  enum UserWindowMessages : UINT
  {
    WM_USER_FILE_FINISHED = WM_USER,
    WM_USER_ALL_FILES_FINISHED,
    WM_USER_FILE_PROGRESS
  };
}