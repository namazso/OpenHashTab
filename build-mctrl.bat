mkdir obj
set CFLAGS="/DISOLATION_AWARE_ENABLED=1" || exit /B 1

cmake -G "Visual Studio 16 2019" -DMCTRL_BUILD_STATIC=ON -DMCTRL_BUILD_EXAMPLES=OFF -DMCTRL_BUILD_TESTS=OFF -S mctrl -A "Win32" -B obj\mctrl\Win32 || exit /B 1
cmake -G "Visual Studio 16 2019" -DMCTRL_BUILD_STATIC=ON -DMCTRL_BUILD_EXAMPLES=OFF -DMCTRL_BUILD_TESTS=OFF -S mctrl -A "x64" -B obj\mctrl\x64 || exit /B 1
cmake -G "Visual Studio 16 2019" -DMCTRL_BUILD_STATIC=ON -DMCTRL_BUILD_EXAMPLES=OFF -DMCTRL_BUILD_TESTS=OFF -S mctrl -A "ARM64" -B obj\mctrl\ARM64 || exit /B 1

msbuild -p:Configuration=Release -p:PlatformToolset=ClangCL obj\mctrl\Win32\mCtrl.sln || exit /B 1
msbuild -p:Configuration=Release -p:PlatformToolset=ClangCL obj\mctrl\x64\mCtrl.sln || exit /B 1
msbuild -p:Configuration=Release -p:PlatformToolset=ClangCL obj\mctrl\ARM64\mCtrl.sln || exit /B 1
