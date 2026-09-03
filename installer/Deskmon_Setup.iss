; ===================================================================
; DESKMON INNO SETUP CONFIGURATION
; Modern, Custom-Branded Windows Installer & Updater
; ===================================================================

#ifndef AppVersion
  #define AppVersion "1.0.3.4"
#endif

#define AppName "Deskmon"
#define AppPublisher "Deskmon"
#define AppExeName "Deskmon.exe"
#define AppId "{{8B412D9E-02A7-44E2-BE2F-D6DF7B1C61E9}}"

[Setup]
; Informasi Aplikasi Dasar
AppId={#AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} v{#AppVersion}
AppPublisher={#AppPublisher}

; Direktori Default & Hak Akses
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline

; Pengaturan Tampilan & Visual Modern
WizardStyle=modern
WizardImageFile=assets\wizard_large.bmp
WizardSmallImageFile=assets\wizard_small.bmp
SetupIconFile=..\resources\images\icon.ico
UninstallDisplayIcon={app}\{#AppExeName}

; Output File
OutputDir=..\dist
OutputBaseFilename=Deskmon-Setup-{#AppVersion}

; Kompresi
Compression=lzma2/ultra64
SolidCompression=yes

; Deteksi Aplikasi Berjalan
CloseApplications=yes
CloseApplicationsFilter=Deskmon.exe,DeskmonTool.exe

; Halaman & Opsi Wizard
DisableWelcomePage=no
DisableDirPage=no
DisableReadyPage=no
AlwaysShowDirOnReadyPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Jalankan otomatis saat startup Windows (Start with Windows)"; GroupDescription: "Preferensi:"; Flags: unchecked

[Files]
; Seluruh file biner dan pustaka hasil windeployqt
Source: "..\dist\Deskmon\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
; Desktop Shortcut
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
; Windows Startup (Auto-start)
Name: "{userstartup}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: autostart

[Run]
; Opsi jalankan setelah instalasi selesai
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
Type: filesandordirs; Name: "{localappdata}\Pranala\Deskmon\logs"

[Code]
// Tutup proses Deskmon yang sedang berjalan sebelum instalasi/update
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  // Coba tutup Deskmon secara halus atau paksa jika masih berjalan
  Exec('cmd.exe', '/c taskkill /IM Deskmon.exe /T /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec('cmd.exe', '/c taskkill /IM DeskmonTool.exe /T /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(500);
end;

// Hapus seluruh file log saat uninstall (database deskmon.db tetap aman)
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  AppLogsDir, LocalLogsDir: String;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // 1. Hapus folder log di folder aplikasi ({app}\logs)
    AppLogsDir := ExpandConstant('{app}\logs');
    if DirExists(AppLogsDir) then
      DelTree(AppLogsDir, True, True, True);

    // 2. Hapus folder log di %LOCALAPPDATA%\Pranala\Deskmon\logs
    LocalLogsDir := ExpandConstant('{localappdata}\Pranala\Deskmon\logs');
    if DirExists(LocalLogsDir) then
      DelTree(LocalLogsDir, True, True, True);
  end;
end;
