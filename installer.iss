; Copyright 2019-2023 namazso <admin@namazso.eu>
; This file is part of OpenHashTab.
;
; OpenHashTab is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; OpenHashTab is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.
#ifndef CI_VERSION
#define CI_VERSION "(unknown)"
#define CI_VERSION_NUMERIC "0.0.0"
#endif

#define MyAppName       "OpenHashTab"
#define MyAppVersion    CI_VERSION
#define MyAppPublisher  "namazso"
#define MyAppURL        "https://github.com/namazso/OpenHashTab"
#define MyCopyright     "(c) namazso. Licensed under GNU GPLv3 or (at your option) later."
#define DLLName         "OpenHashTab.dll"
#define StandaloneName  "StandaloneStub.exe"
#define DLLCLSID        "{{23b5bdd4-7669-42b8-9cdc-beebc8a5baa9}"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{C0EEE3CD-665D-4E4E-B3BC-ADCD0FE73C0F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
AppCopyright={#MyCopyright}
CreateAppDir=yes
DefaultDirName={autopf}\OpenHashTab
LicenseFile=license.installer.txt
PrivilegesRequiredOverridesAllowed=commandline dialog
OutputBaseFilename=OpenHashTab_setup
OutputDir=.
Compression=lzma
SolidCompression=yes
WizardStyle=modern
VersionInfoProductTextVersion={#CI_VERSION}
VersionInfoVersion={#CI_VERSION_NUMERIC}
ChangesAssociations=yes
UninstallDisplayIcon={app}\OpenHashTab.dll,0
ArchitecturesAllowed=win64
ArchitecturesInstallIn64BitMode=win64
#ifdef SIGN
SignTool=signtool $f
SignedUninstaller=yes
#endif

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
// "English" seems to always break the default language detection with unknown reasons.
Name: "armenian"; MessagesFile: "compiler:Languages\Armenian.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "icelandic"; MessagesFile: "compiler:Languages\Icelandic.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "slovak"; MessagesFile: "compiler:Languages\Slovak.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "ChineseSimplified"; MessagesFile: "Localization\isl\ChineseSimplified.isl"
Name: "ChineseTraditional"; MessagesFile: "Localization\isl\ChineseTraditional.isl"

[Files]
Source: "AlgorithmsDlls\*.dll";                     DestDir: "{app}"; Flags: ignoreversion restartreplace;
Source: "AlgorithmsDlls\*.pdb";                     DestDir: "{app}"; Flags: ignoreversion restartreplace;

Source: "cmake-openhashtab-x64\OpenHashTab.dll";    DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('x64')
Source: "cmake-openhashtab-x64\*.exe";              DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('x64')
Source: "cmake-openhashtab-x64\*.pdb";              DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('x64')

Source: "cmake-openhashtab-ARM64\OpenHashTab.dll";  DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('arm64')
Source: "cmake-openhashtab-ARM64\*.exe";            DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('arm64')
Source: "cmake-openhashtab-ARM64\*.pdb";            DestDir: "{app}"; Flags: ignoreversion restartreplace 64bit; Check: InstallArch('arm64')

[CustomMessages]
GroupDescription=Optional features:
myAssociation=Associate with known sumfile formats
myContextMenu=Add to context menu

ChineseSimplified.GroupDescription=可选操作：
ChineseSimplified.myAssociation=关联已知的校验和文件（sumfile）格式
ChineseSimplified.myContextMenu=添加右键菜单

[Tasks]
Name: myAssociation; Description: "{cm:myAssociation}"; GroupDescription: "{cm:GroupDescription}"
Name: myContextMenu; Description: "{cm:myContextMenu}"; GroupDescription: "{cm:GroupDescription}"

[Registry]
Root: HKLM32; Subkey: "Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved";  ValueName: "{#DLLCLSID}"; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Check: InstallArch('x86'); Flags: noerror
Root: HKLM64; Subkey: "Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved";  ValueName: "{#DLLCLSID}"; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Check: InstallArch('x64'); Flags: noerror
Root: HKLM64; Subkey: "Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved";  ValueName: "{#DLLCLSID}"; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Check: InstallArch('arm64'); Flags: noerror

Root: HKA32; Subkey: "Software\Classes\CLSID\{#DLLCLSID}";  ValueName: ""; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Flags: uninsdeletekey; Check: InstallArch('x86')
Root: HKA32; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: ""; ValueData: "{app}\{#DLLName}"; ValueType: string; Check: InstallArch('x86')
Root: HKA32; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: "ThreadingModel"; ValueData: "Apartment"; ValueType: string; Check: InstallArch('x86')

Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}";  ValueName: ""; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Flags: uninsdeletekey; Check: InstallArch('x64')
Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: ""; ValueData: "{app}\{#DLLName}"; ValueType: string; Check: InstallArch('x64')
Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: "ThreadingModel"; ValueData: "Apartment"; ValueType: string; Check: InstallArch('x64')

Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}";  ValueName: ""; ValueData: "{#MyAppName} Shell Extension";  ValueType: string; Flags: uninsdeletekey; Check: InstallArch('arm64')
Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: ""; ValueData: "{app}\{#DLLName}"; ValueType: string; Check: InstallArch('arm64')
Root: HKA64; Subkey: "Software\Classes\CLSID\{#DLLCLSID}\InprocServer32";  ValueName: "ThreadingModel"; ValueData: "Apartment"; ValueType: string; Check: InstallArch('arm64')

Root: HKA; Subkey: "Software\Classes\AllFilesystemObjects\shellex\PropertySheetHandlers\{#DLLCLSID}";  ValueName: ""; Flags: uninsdeletekey; ValueType: none;
Root: HKA; Subkey: "Software\Classes\AllFilesystemObjects\shellex\ContextMenuHandlers\{#DLLCLSID}";    ValueName: ""; Flags: uninsdeletekey; ValueType: none; Tasks: myContextMenu

Root: HKA; Subkey: "Software\Classes\{#MyAppName}";                     ValueData: "Checksum file";      ValueType: string; ValueName: ""; Flags: uninsdeletekey;
Root: HKA; Subkey: "Software\Classes\{#MyAppName}\shell\open\command";  ValueData: """{app}\{#StandaloneName}"" ""%1""";  ValueType: string; ValueName: ""
Root: HKA; Subkey: "Software\Classes\{#MyAppName}\DefaultIcon";         ValueData: "{app}\{#DLLName},0"; ValueType: string; ValueName: ""

Root: HKA; Subkey: "Software\Classes\.md5";        ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.md5sum";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.md5sums";    ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.ripemd160";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha1";       ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha1sum";    ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha1sums";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha224";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha224sum";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha256";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha256sum";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha256sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha384";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha512";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha512sum";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha512sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha3";       ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha3-512";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha3-224";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha3-256";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.sha3-384";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.k12-264";    ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.ph128-264";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.ph256-528";  ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.blake3";     ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.blake2sp";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.xxh32";      ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.xxh64";      ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.xxh3-64";    ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.xxh3-128";   ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKA; Subkey: "Software\Classes\.md4";        ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

; corz checksum that we accidentally support
Root: HKA; Subkey: "Software\Classes\.hash"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

; sfv
Root: HKA; Subkey: "Software\Classes\.sfv"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

; our default export extension when there is no known for a given algorithm
Root: HKA; Subkey: "Software\Classes\.sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

[Code]
function InstallArch(Arch: String): Boolean;
begin
    Result := False;
    case ProcessorArchitecture of
        paX86:    Result := Arch = 'x86';
        paX64:    Result := (Arch = 'x64') or (Arch = 'wow');
        paARM64:  Result := (Arch = 'arm64') or (Arch = 'wow');
    end;
end;
