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

[CmdletBinding()] param ()

function Get-Environment {
    <#
    .SYNOPSIS
        Captures the current environment variables.
    .DESCRIPTION
        Returns a hashtable of all current environment variables.
    .EXAMPLE
        $SavedEnv = Get-Environment
    .OUTPUTS
        System.Collections.Hashtable
    #>
    $env = @{}
    Get-ChildItem Env: | ForEach-Object { $env[$_.Name] = $_.Value }
    return $env
}

function Set-Environment {
    <#
    .SYNOPSIS
        Restores environment variables from a saved state.
    .DESCRIPTION
        Clears current environment variables and sets them to the provided saved state.
    .PARAMETER Environment
        Hashtable of environment variables previously captured by Get-Environment.
    .EXAMPLE
        Set-Environment -Environment $SavedEnv
    #>
    param(
        [Parameter(Mandatory=$true)]
        [System.Collections.Hashtable]$Environment
    )
    
    # Clear current environment variables
    Remove-Item -Path Env:* -ErrorAction SilentlyContinue
    
    # Restore saved environment variables
    foreach ($key in $Environment.Keys) {
        Set-Item "Env:$key" $Environment[$key]
    }
}

function Reset-Environment {
    <#
    .SYNOPSIS
        Resets the environment by keeping only default Windows environment variables.
    .DESCRIPTION
        Filters an environment hashtable to keep only default Windows environment variables.
        Returns a reset environment hashtable.
    .PARAMETER Environment
        Hashtable of environment variables to reset, as provided by Get-Environment.
    .PARAMETER AdditionalVariables
        Optional array of additional environment variable names to preserve.
    .EXAMPLE
        $ResetEnv = Reset-Environment -Environment $SavedEnv
    .EXAMPLE
        $ResetEnv = Reset-Environment -Environment (Get-Environment) -AdditionalVariables @("JAVA_HOME")
    .OUTPUTS
        System.Collections.Hashtable
    #>
    param(
        [Parameter(Mandatory=$true)]
        [System.Collections.Hashtable]$Environment,
        
        [Parameter(Mandatory=$false)]
        [string[]]$AdditionalVariables = @()
    )
    
    # List of default Windows environment variables to preserve
    $defaultVars = @(
        "ALLUSERSPROFILE", "APPDATA", "CommonProgramFiles", "CommonProgramFiles(x86)",
        "CommonProgramW6432", "COMPUTERNAME", "ComSpec", "HOMEDRIVE", "HOMEPATH",
        "LOCALAPPDATA", "LOGONSERVER", "NUMBER_OF_PROCESSORS", "OS", "Path", "PATHEXT",
        "PROCESSOR_ARCHITECTURE", "PROCESSOR_IDENTIFIER", "PROCESSOR_LEVEL", "PROCESSOR_REVISION",
        "ProgramData", "ProgramFiles", "ProgramFiles(x86)", "ProgramW6432", "PROMPT",
        "PSModulePath", "PUBLIC", "SystemDrive", "SystemRoot", "TEMP", "TMP",
        "USERDOMAIN", "USERDOMAIN_ROAMINGPROFILE", "USERNAME", "USERPROFILE", "windir"
    )
    
    # Combine default variables with additional ones
    $varsToKeep = $defaultVars + $AdditionalVariables
    
    # Create a new hashtable with only the variables to keep
    $resetEnvironment = @{}
    foreach ($key in $Environment.Keys) {
        if ($varsToKeep -contains $key -or $varsToKeep -contains $key.ToUpper()) {
            $resetEnvironment[$key] = $Environment[$key]
        }
    }

    $SystemRoot = $resetEnvironment["SystemRoot"]
    $resetEnvironment["Path"] = "$SystemRoot\system32;$SystemRoot;$SystemRoot\System32\Wbem;$SystemRoot\System32\WindowsPowerShell\v1.0;"
    
    return $resetEnvironment
}
function Get-CmdEnvironment {
    <#
    .SYNOPSIS
        Captures environment variables from a cmd.exe script execution.
    .DESCRIPTION
        Executes a specified script/batch file using cmd.exe and returns all environment 
        variables after execution as a hashtable.
    .PARAMETER ScriptPath
        The path to the command, batch file, or script to execute.
    .PARAMETER Arguments
        Optional arguments to pass to the script.
    .EXAMPLE
        $VsEnv = Get-CmdEnvironment -ScriptPath "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    .OUTPUTS
        System.Collections.Hashtable
    #>
    param(
        [Parameter(Mandatory=$true)]
        [String]$ScriptPath,
        
        [Parameter(Mandatory=$false)]
        [String]$Arguments = ""
    )
    
    if (-not (Test-Path $ScriptPath)) {
        Write-Error "Script not found: $ScriptPath"
        return $null
    }
    
    # Use full path and proper quoting
    $scriptPathFull = (Resolve-Path $ScriptPath).Path
    $cmdLine = "`"$scriptPathFull`" $Arguments && set"
    
    Write-Verbose "Executing: cmd.exe /c $cmdLine"
    
    $env = @{}
    
    & cmd.exe /c "$cmdLine" | 
    Select-String '^([^=]*)=(.*)$' | ForEach-Object {
        $varName = $_.Matches[0].Groups[1].Value
        $varValue = $_.Matches[0].Groups[2].Value
        $env[$varName] = $varValue
    }
    
    return $env
}

$OriginalEnvironment = Get-Environment;

$VSRoot = If ($Env:VSINSTALLDIR) {$Env:VSINSTALLDIR} Else { & vswhere -property installationPath };

If (-not $VSRoot) {
    Throw "Visual Studio not found."
}

$WinSdkVer = If ($Env:WindowsTargetPlatformVersion) { $Env:WindowsTargetPlatformVersion } Else { $(Get-Item "hklm:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows").GetValue("CurrentVersion") };
$WinSdkDir = If ($Env:WindowsSdkDir) { $Env:WindowsSdkDir } Else { $(Get-Item "hklm:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\Windows").GetValue("CurrentInstallFolder") };

$Cmake = Get-Command "cmake";
$Dotnet = Get-Command "dotnet";
$Ninja = (Get-Command "ninja").Path;
$LlvmPath = (Get-Item (Get-Command "clang").Path).Directory.FullName;

If (Test-Path "Env:CI_VERSION") {
    $OhtVersion = $Env:CI_VERSION;
    $OhtVersionMajor = $Env:CI_VERSION_MAJOR;
    $OhtVersionMinor = $Env:CI_VERSION_MINOR;
    $OhtVersionPatch = $Env:CI_VERSION_PATCH;
} Else {
    $OhtVersion = "(unknown)";
    $OhtVersionMajor = 0;
    $OhtVersionMinor = 0;
    $OhtVersionPatch = 0;
}

$CleanEnv = Reset-Environment $OriginalEnvironment -AdditionalVariables @("AUTHENTICODE_SIGN");
Set-Environment -Environment $CleanEnv;

Write-Host "Using Visual Studio at: $VSRoot"

$x86_Environment = Get-CmdEnvironment -ScriptPath "$VSRoot\VC\Auxiliary\Build\vcvarsamd64_x86.bat";
$x86_Environment["CFLAGS"] = "--target=i686-pc-windows-msvc";
$x86_Environment["CXXFLAGS"] = "--target=i686-pc-windows-msv";
$x86_Environment["LDFLAGS"] = "--target=i686-pc-windows-msv";
$x64_Environment = Get-CmdEnvironment -ScriptPath "$VSRoot\VC\Auxiliary\Build\vcvars64.bat";
$x64_Environment["CFLAGS"] = "--target=x86_64-pc-windows-msvc";
$x64_Environment["CXXFLAGS"] = "--target=x86_64-pc-windows-msvc";
$x64_Environment["LDFLAGS"] = "--target=x86_64-pc-windows-msvc";
$ARM64_Environment = Get-CmdEnvironment -ScriptPath "$VSRoot\VC\Auxiliary\Build\vcvarsamd64_arm64.bat";
$ARM64_Environment["CFLAGS"] = "--target=arm64-pc-windows-msvc";
$ARM64_Environment["CXXFLAGS"] = "--target=arm64-pc-windows-msvc";
$ARM64_Environment["LDFLAGS"] = "--target=arm64-pc-windows-msvc";

foreach ($env in @($x86_Environment, $x64_Environment, $ARM64_Environment)) {
    $Platform = $env["Platform"];
    $env["CC"] = "$LlvmPath\clang-cl.exe";
    $env["CXX"] = "$LlvmPath\clang-cl.exe";
    $env["INCLUDE"] += ";$WinSdkDir\Include\$WinSdkVer\shared;$WinSdkDir\Include\$WinSdkVer\ucrt;$WinSdkDir\Include\$WinSdkVer\um";
    $env["LIB"] += ";$WinSdkDir\Lib\$WinSdkVer\ucrt\$Platform;$WinSdkDir\Lib\$WinSdkVer\um\$Platform";
    $env["LIBPATH"] += ";$WinSdkDir\Lib\$WinSdkVer\ucrt\$Platform;$WinSdkDir\Lib\$WinSdkVer\um\$Platform";
    $env["RC"] = "$LlvmPath\llvm-rc.exe";
    $env["Path"] += ";$WinSdkDir\bin\$WinSdkVer\x64";
}

Write-Host "bleh"

New-Item -Path "install" -ItemType Directory -Force
New-Item -Path "install\algorithms" -ItemType Directory -Force
$AlgorithmsInstallDir = (Get-Item "install\algorithms").FullName;

"x86", "SSE2", "AVX2", "AVX512", "ARM64" | ForEach-Object {
    If ($_ -eq "x86") {
        $Environment = $x86_Environment;
    } Elseif ($_ -eq "ARM64") {
        $Environment = $ARM64_Environment;
    } Else {
        $Environment = $x64_Environment;
    }

    $BuildDir = "build\algorithms-$_";

    Set-Environment $Environment;
    $Mt = (Get-Command "mt").Path;
    & $Cmake `
        -G Ninja `
        -S Algorithms `
        -B $BuildDir `
        --install-prefix $AlgorithmsInstallDir `
        "-DOHT_FLAVOR=$_" `
        "-DCMAKE_BUILD_TYPE=Release" `
        "-DCMAKE_MAKE_PROGRAM=$Ninja" `
        "-DCMAKE_MT=$Mt"
    & $Cmake --build $BuildDir
    & $Cmake --install $BuildDir

    If ($Env:AUTHENTICODE_SIGN) {
        & $Env:AUTHENTICODE_SIGN "$((Get-Item "$AlgorithmsInstallDir\AlgorithmsDll_$_.dll").FullName)"
    }
}

$OhtVersionNumeric = "$OhtVersionMajor.$OhtVersionMinor.$OhtVersionPatch";

"x86", "x64", "arm64" | ForEach-Object {
    If ($_ -eq "x86") {
        $Environment = $x86_Environment;
    } Elseif ($_ -eq "arm64") {
        $Environment = $ARM64_Environment;
    } Else {
        $Environment = $x64_Environment;
    }

    $Platform = $Environment["Platform"];

    $BuildDir = "build\oht-$_";

    Set-Environment $Environment;
    $Mt = (Get-Command "mt").Path;
    & $Cmake `
        -G Ninja `
        -S OpenHashTab `
        -B $BuildDir `
        "-DCMAKE_BUILD_TYPE=Release" `
        "-DCMAKE_MAKE_PROGRAM=$Ninja" `
        "-DCMAKE_MT=$Mt" `
        "-DOHT_ALGORITHMS_INSTALL_DIR=$AlgorithmsInstallDir" `
        "-DOHT_ALGORITHMS_INCLUDE_DIR=$((Get-Item .\Algorithms).FullName)" `
        "-DOHT_LOCALES_DIR=$((Get-Item .\Localization).FullName)" `
        "-DCI_VERSION=$OhtVersion" `
        "-DCI_VERSION_MAJOR=$OhtVersionMajor" `
        "-DCI_VERSION_MINOR=$OhtVersionMinor" `
        "-DCI_VERSION_PATCH=$OhtVersionPatch"
    & $Cmake --build $BuildDir

    If ($Env:AUTHENTICODE_SIGN) {
        & $Env:AUTHENTICODE_SIGN "$((Get-Item "$BuildDir\OpenHashTab*.dll").FullName)"
        & $Env:AUTHENTICODE_SIGN "$((Get-Item "$BuildDir\StandaloneStub.exe").FullName)"
        & $Env:AUTHENTICODE_SIGN "$((Get-Item "$BuildDir\Benchmark.exe").FullName)"
    }
    
    $BuildDir32 = "build\oht-x86";

    "User", "Machine" | ForEach-Object {
        & $Dotnet build `
            -c $_ `
            "-property:ProductVersion=$OhtVersionNumeric" `
            "-property:BuildDirectory32=$((Get-Item $BuildDir32).FullName)" `
            "-property:BuildDirectory=$((Get-Item $BuildDir).FullName)" `
            "-property:AlgorithmsDllsDirectory=$AlgorithmsInstallDir" `
    }
}

Set-Environment -Environment $OriginalEnvironment;
