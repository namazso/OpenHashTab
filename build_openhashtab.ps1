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

$Env:RC = "llvm-rc";

If (Test-Path "Env:CI_VERSION") {
    $CI_VERSION = $Env:CI_VERSION;
    $CI_VERSION_MAJOR = $Env:CI_VERSION_MAJOR;
    $CI_VERSION_MINOR = $Env:CI_VERSION_MINOR;
    $CI_VERSION_PATCH = $Env:CI_VERSION_PATCH;
} Else {
    $CI_VERSION = "(unknown)";
    $CI_VERSION_MAJOR = 0;
    $CI_VERSION_MINOR = 0;
    $CI_VERSION_PATCH = 0;
}

$Environment = (Get-ChildItem Env:);

Invoke-CmdScript "$VSRoot\VC\Auxiliary\Build\vcvars64.bat"

cmake `
    -G Ninja `
    -S . `
    -B "cmake-openhashtab-x64" `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_C_COMPILER=clang-cl `
    -DCMAKE_CXX_COMPILER=clang-cl `
    -DCMAKE_MT=mt `
    "-DCI_VERSION=$CI_VERSION" `
    "-DCI_VERSION_MINOR=$CI_VERSION_MINOR" `
    "-DCI_VERSION_MAJOR=$CI_VERSION_MAJOR" `
    "-DCI_VERSION_PATCH=$CI_VERSION_PATCH"
cmake --build "cmake-openhashtab-x64"

Remove-Item -Path Env:*
$Environment | ForEach-Object { Set-Item "env:$($_.Name)" $_.Value }

Invoke-CmdScript "$VSRoot\VC\Auxiliary\Build\vcvarsamd64_arm64.bat"
$ExtraFlags = "--target=arm64-pc-windows-msvc";

Set-Item Env:CFLAGS $ExtraFlags
Set-Item Env:CXXFLAGS $ExtraFlags

cmake `
    -G Ninja `
    -S . `
    -B "cmake-openhashtab-ARM64" `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_C_COMPILER=clang-cl `
    -DCMAKE_CXX_COMPILER=clang-cl `
    -DCMAKE_MT=mt `
    "-DCI_VERSION=$CI_VERSION" `
    "-DCI_VERSION_MINOR=$CI_VERSION_MINOR" `
    "-DCI_VERSION_MAJOR=$CI_VERSION_MAJOR" `
    "-DCI_VERSION_PATCH=$CI_VERSION_PATCH"
cmake --build "cmake-openhashtab-ARM64"

Remove-Item -Path Env:*
$Environment | ForEach-Object { Set-Item "env:$($_.Name)" $_.Value }
