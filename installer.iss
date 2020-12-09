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
CreateAppDir=no
LicenseFile=license.installer.txt
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
OutputDir=.
OutputBaseFilename=OpenHashTab_setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
VersionInfoProductTextVersion={#CI_VERSION}
VersionInfoVersion={#CI_VERSION_NUMERIC}
ChangesAssociations = yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
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

[Files]
Source: "bin\Release\x64\{#DLLName}";   DestDir: "{sys}"; Flags: ignoreversion restartreplace regserver solidbreak 64bit; Check: InstallArch('x64')
Source: "bin\Release\ARM64\{#DLLName}"; DestDir: "{sys}"; Flags: ignoreversion restartreplace regserver solidbreak 64bit; Check: InstallArch('arm64') 
Source: "bin\Release\Win32\{#DLLName}"; DestDir: "{sys}"; Flags: ignoreversion restartreplace regserver solidbreak 32bit

Source: "bin\Release\x64\AlgorithmsDll*.dll";   DestDir: "{sys}"; Flags: ignoreversion restartreplace solidbreak 64bit; Check: InstallArch('x64')
Source: "bin\Release\ARM64\AlgorithmsDll*.dll"; DestDir: "{sys}"; Flags: ignoreversion restartreplace solidbreak 64bit; Check: InstallArch('arm64') 
Source: "bin\Release\Win32\AlgorithmsDll*.dll"; DestDir: "{sys}"; Flags: ignoreversion restartreplace solidbreak 32bit

[Tasks]
Name: myAssociation; Description: "Associate with known sumfile formats"; GroupDescription: File extensions:


[Registry]
Root: HKCR; Subkey: "{#MyAppName}";                     ValueData: "Checksum file";                               ValueType: string; ValueName: ""; Flags: uninsdeletekey;
Root: HKCR; Subkey: "{#MyAppName}\DefaultIcon";         ValueData: "{sys}\{#DLLName},0";                          ValueType: string; ValueName: ""
Root: HKCR; Subkey: "{#MyAppName}\shell\open\command";  ValueData: "rundll32 {#DLLName},StandaloneEntry ""%1""";  ValueType: string; ValueName: ""

Root: HKCR; Subkey: ".md5"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".md5sum"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".md5sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".ripemd160"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha1"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha1sum"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha1sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha224"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha224sum"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha256"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha256sum"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha256sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha384"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha512"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha512sum"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha512sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation
Root: HKCR; Subkey: ".sha3"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

; corz checksum that we accidentally support
Root: HKCR; Subkey: ".hash"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

; our default export extension when there is no known for a given algorithm
Root: HKCR; Subkey: ".sums"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: myAssociation

[Code]
function InstallArch(Arch: String): Boolean;
begin
    Result := False;
    case ProcessorArchitecture of
        paX86:    Result := Arch = 'x86';
        paX64:    Result := (Arch = 'x64') or (Arch = 'wow');
        paIA64:   Result := (Arch = 'ia64') or (Arch = 'wow');
        paARM64:  Result := (Arch = 'arm64') or (Arch = 'wow');
    end;
end; 