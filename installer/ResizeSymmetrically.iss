#define MyAppName "Resize Symmetrically"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Resize Symmetrically"
#define MyAppExeName "ResizeSymmetrically.exe"

[Setup]
AppId={{63C07190-650C-4C17-8484-CB47A213EF09}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\Resize Symmetrically
DefaultGroupName=Resize Symmetrically
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\artifacts
OutputBaseFilename=ResizeSymmetrically-Setup-x64
SetupIconFile=..\assets\ResizeSymmetrically.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#MyAppVersion}
VersionInfoProductName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "..\bin\x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion restartreplace

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--uninstall-cleanup"; Flags: runhidden; RunOnceId: "CleanupUserSettings"

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
