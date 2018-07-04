; -- setup.iss --
; Creates the installation file for the service

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!
#define MyAppName "whatismyip"
#define MyAppExeName "whatismyip_service.exe"
#define MyAppIcoName "program.ico"

[Setup]
AppName={#MyAppName}
AppVersion=1.2.0
AppPublisher=Microsoft
UsePreviousAppDir=yes
DefaultDirName={userappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={userappdata}\{#MyAppName}\{#MyAppIcoName}
OutputBaseFilename={#MyAppName}
VersionInfoCompany=Microsoft
VersionInfoProductName={#MyAppName}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
; OutputDir=userdocs:Inno Setup Examples Output
                                                
[Files]
; Source: "{#MyAppIcoName}"; DestDir: "{app}"
; Install MyAppName only if running in 64-bit mode (x64; see above),
Source: "x64\Release\{#MyAppExeName}"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Check: Is64BitInstallMode; Flags: ignoreversion


[Run]
; add the Parameters, WorkingDir and StatusMsg as you wish, just keep here
; the conditional installation Check
Filename: "sc.exe"; Parameters: "stop {#MyAppName}"
Filename: "sc.exe"; Parameters: "delete {#MyAppName}"
Filename: "sc.exe"; Parameters: "create {#MyAppName} binpath= {app}\{#MyAppExeName}"
Filename: "sc.exe"; Parameters: "config {#MyAppName}  start= auto"
Filename: "sc.exe"; Parameters: "start {#MyAppName}"

