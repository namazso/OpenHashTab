mkdir obj
set CHERE_INVOKING=1 || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make generic32/libXKCP.a.vcxproj" || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make generic64/libXKCP.a.vcxproj" || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make AVX/libXKCP.a.vcxproj" || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make AVX2noAsm/libXKCP.a.vcxproj" || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make AVX512noAsm/libXKCP.a.vcxproj" || exit /B 1
c:\msys64\usr\bin\bash -l -e -c "cd XKCP && make ARMv8A/libXKCP.a.vcxproj" || exit /B 1

rem workaround for bug https://github.com/XKCP/XKCP/issues/77
c:\msys64\usr\bin\bash -l -e -c "sed -i 's/x64/ARM64/g' XKCP/bin/VC/ARMv8A_libXKCP.a.vcxproj" || exit /B 1

c:\msys64\usr\bin\bash -l -e -c "sed -i 's/<ClCompile>/<ClCompile><RuntimeLibrary>MultiThreaded<\/RuntimeLibrary>/g' XKCP/bin/VC/*.vcxproj" || exit /B 1

msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=Win32 -p:TargetName=libXKCP-Win32-sse2 XKCP\bin\VC\generic32_libXKCP.a.vcxproj || exit /B 1
rem "extensions on x86 build is broken, see https://github.com/XKCP/XKCP/issues/76"
rem msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=Win32 -p:TargetName=libXKCP-Win32-avx XKCP\bin\VC\AVX_libXKCP.a.vcxproj || exit /B 1
copy obj\libXKCP-Win32-sse2.lib obj\libXKCP-Win32-avx.lib || exit /B 1
rem msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=Win32 -p:TargetName=libXKCP-Win32-avx2 XKCP\bin\VC\AVX2noAsm_libXKCP.a.vcxproj || exit /B 1
copy obj\libXKCP-Win32-sse2.lib obj\libXKCP-Win32-avx2.lib || exit /B 1
rem msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=Win32 -p:TargetName=libXKCP-Win32-avx512 XKCP\bin\VC\AVX512noAsm_libXKCP.a.vcxproj || exit /B 1
copy obj\libXKCP-Win32-sse2.lib obj\libXKCP-Win32-avx512.lib || exit /B 1

msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=x64 -p:TargetName=libXKCP-x64-sse2 XKCP\bin\VC\generic64_libXKCP.a.vcxproj || exit /B 1
msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=x64 -p:TargetName=libXKCP-x64-avx XKCP\bin\VC\AVX_libXKCP.a.vcxproj || exit /B 1
msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=x64 -p:TargetName=libXKCP-x64-avx2 XKCP\bin\VC\AVX2noAsm_libXKCP.a.vcxproj || exit /B 1
msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=x64 -p:TargetName=libXKCP-x64-avx512 XKCP\bin\VC\AVX512noAsm_libXKCP.a.vcxproj || exit /B 1


msbuild -p:Configuration=Release -p:ConfigurationType=StaticLibrary -p:PlatformToolset=ClangCL -p:CharacterSet=Unicode -p:FunctionLevelLinking=false -p:WholeProgramOptimization=false -p:RuntimeLibrary=MultiThreaded -p:"OutDir=$(ProjectDir)..\..\..\obj\\" -p:Platform=ARM64 -p:TargetName=libXKCP-ARM64-neon XKCP\bin\VC\ARMv8A_libXKCP.a.vcxproj || exit /B 1