# Copyright 2019-2025 namazso <admin@namazso.eu>
# This file is part of OpenHashTab.
#
# OpenHashTab is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenHashTab is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.

$VSRoot = (& vswhere -property installationPath);

function Invoke-CmdScript {
  param(
    [String] $scriptName
  )
  $cmdLine = """$scriptName"" $args & set"
  & $Env:SystemRoot\system32\cmd.exe /c $cmdLine |
  Select-String '^([^=]*)=(.*)$' | ForEach-Object {
    $varName = $_.Matches[0].Groups[1].Value
    $varValue = $_.Matches[0].Groups[2].Value
    Set-Item Env:$varName $varValue
  }
}

$Environment = (Get-ChildItem Env:);

"x86", "SSE2", "AVX2", "AVX512", "ARM64" | ForEach-Object {
    $ExtraFlags = "";
    If ($_ -eq "x86") {
        Invoke-CmdScript "$VSRoot\VC\Auxiliary\Build\vcvars32.bat"
    } Elseif ($_ -eq "ARM64") {
        Invoke-CmdScript "$VSRoot\VC\Auxiliary\Build\vcvarsamd64_arm64.bat"
        $ExtraFlags = "--target=arm64-pc-windows-msvc";
    } Else {
        Invoke-CmdScript "$VSRoot\VC\Auxiliary\Build\vcvars64.bat"
    }
    Set-Item Env:CFLAGS $ExtraFlags
    Set-Item Env:CXXFLAGS $ExtraFlags

    mkdir -ErrorAction Ignore "cmake-algorithms-$_"
    cmake `
        -G Ninja `
        -S Algorithms `
        -B "cmake-algorithms-$_" `
        --install-prefix ((Get-Item AlgorithmsDlls).FullName) `
        "-DOHT_FLAVOR=$_" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER=clang-cl `
        -DCMAKE_CXX_COMPILER=clang-cl
    cmake --build "cmake-algorithms-$_"
    cmake --install "cmake-algorithms-$_"

    Remove-Item -Path Env:*
    $Environment | ForEach-Object { Set-Item "env:$($_.Name)" $_.Value }
}
