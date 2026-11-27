; ===================================================================
; DESKMON INNO SETUP CONFIGURATION
; Modern, Custom-Branded Windows Installer & Updater
; ===================================================================

#ifndef AppVersion
  #define VerFile FileOpen(SourcePath + "..\version.txt")
  #define AppVersion Trim(FileRead(VerFile))
  #expr FileClose(VerFile)
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
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "autostart"; Description: "Start Deskmon automatically when Windows starts"; GroupDescription: "Preferences:"; Flags: unchecked

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
var
  ShouldDeleteDatabase: Boolean;

function TryKillDeskmonProcesses(): Boolean;
var
  ResultCode: Integer;
begin
  Exec('cmd.exe', '/c taskkill /IM Deskmon.exe /T /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec('cmd.exe', '/c taskkill /IM DeskmonTool.exe /T /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(400);
  Result := True;
end;

function IsFileLocked(const FilePath: String): Boolean;
var
  ProbePath: String;
begin
  Result := False;
  if not FileExists(FilePath) then
    Exit;
  ProbePath := FilePath + '.lockcheck';
  if RenameFile(FilePath, ProbePath) then
  begin
    RenameFile(ProbePath, FilePath);
  end
  else
  begin
    Result := True;
  end;
end;

function AskUninstallDatabaseOption(const HasDb, HasProfiles: Boolean): Boolean;
var
  Form: TSetupForm;
  InfoLabel: TNewStaticText;
  DbCheckBox: TNewCheckBox;
  OkButton: TNewButton;
  TargetDesc: String;
begin
  Result := False;
  if UninstallSilent then
    Exit;

  Form := CreateCustomForm(ScaleX(430), ScaleY(175), False, True);
  try
    Form.Caption := 'Deskmon Uninstall Options';
    Form.Position := poScreenCenter;

    TargetDesc := 'Deskmon application files and temporary caches will be removed.' + #13#10 +
                  'Choose whether to also delete your local user data:';

    InfoLabel := TNewStaticText.Create(Form);
    InfoLabel.Parent := Form;
    InfoLabel.Left := ScaleX(20);
    InfoLabel.Top := ScaleY(18);
    InfoLabel.Width := ScaleX(390);
    InfoLabel.Height := ScaleY(44);
    InfoLabel.WordWrap := True;
    InfoLabel.Caption := TargetDesc;

    DbCheckBox := TNewCheckBox.Create(Form);
    DbCheckBox.Parent := Form;
    DbCheckBox.Left := ScaleX(20);
    DbCheckBox.Top := ScaleY(72);
    DbCheckBox.Width := ScaleX(390);
    DbCheckBox.Height := ScaleY(36);
    DbCheckBox.Checked := False;
    if HasDb and HasProfiles then
      DbCheckBox.Caption := 'Delete local database (deskmon.db), profile data, and saved settings'
    else if HasDb then
      DbCheckBox.Caption := 'Delete local database (deskmon.db) and saved settings'
    else
      DbCheckBox.Caption := 'Delete saved profile data and settings';

    OkButton := TNewButton.Create(Form);
    OkButton.Parent := Form;
    OkButton.Width := ScaleX(95);
    OkButton.Height := ScaleY(26);
    OkButton.Left := Form.ClientWidth - ScaleX(20) - OkButton.Width;
    OkButton.Top := Form.ClientHeight - ScaleY(16) - OkButton.Height;
    OkButton.Caption := 'Continue';
    OkButton.ModalResult := mrOk;
    OkButton.Default := True;

    Form.ShowModal();
    Result := DbCheckBox.Checked;
  finally
    Form.Free();
  end;
end;

function DeleteFileWithRetryPrompt(const FilePath, FriendlyName: String; var Aborted: Boolean): Boolean;
var
  Choice: Integer;
begin
  Result := True;
  if Aborted or (not FileExists(FilePath)) then
    Exit;

  while FileExists(FilePath) do
  begin
    if DeleteFile(FilePath) then
      Exit;

    if UninstallSilent then
    begin
      Result := False;
      Exit;
    end;

    Choice := MsgBox(
      'Unable to delete ' + FriendlyName + ' because it is currently open or locked by another application:' + #13#10 +
      FilePath + #13#10#13#10 +
      'Please close the application using this file (e.g. SQLite viewer or Deskmon), then click Retry.' + #13#10 +
      'Click Ignore to keep the file, or Abort to stop removing remaining user data.',
      mbError, MB_ABORTRETRYIGNORE);

    if Choice = IDRETRY then
    begin
      TryKillDeskmonProcesses();
      Continue;
    end
    else if Choice = IDABORT then
    begin
      Aborted := True;
      Result := False;
      Exit;
    end
    else
    begin
      Result := False;
      Exit;
    end;
  end;
end;

function DeleteDirWithRetryPrompt(const DirPath, FriendlyName: String; var Aborted: Boolean): Boolean;
var
  Choice: Integer;
begin
  Result := True;
  if Aborted or (not DirExists(DirPath)) then
    Exit;

  while DirExists(DirPath) do
  begin
    if DelTree(DirPath, True, True, True) then
      Exit;

    if UninstallSilent then
    begin
      Result := False;
      Exit;
    end;

    Choice := MsgBox(
      'Unable to completely remove ' + FriendlyName + ' because a file inside it is locked or in use:' + #13#10 +
      DirPath + #13#10#13#10 +
      'Please close any program accessing this folder and click Retry, Ignore to skip, or Abort to stop.',
      mbError, MB_ABORTRETRYIGNORE);

    if Choice = IDRETRY then
    begin
      TryKillDeskmonProcesses();
      Continue;
    end
    else if Choice = IDABORT then
    begin
      Aborted := True;
      Result := False;
      Exit;
    end
    else
    begin
      Result := False;
      Exit;
    end;
  end;
end;

// Tutup proses Deskmon yang sedang berjalan sebelum instalasi/update dan pastikan file tidak terkunci
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ExistingExe: String;
  Choice: Integer;
begin
  Result := '';
  TryKillDeskmonProcesses();

  ExistingExe := ExpandConstant('{app}\{#AppExeName}');
  while IsFileLocked(ExistingExe) do
  begin
    if WizardSilent then
    begin
      Result := 'Deskmon.exe is still running or locked by another process: ' + ExistingExe;
      Exit;
    end;

    Choice := MsgBox(
      'Deskmon.exe is currently in use and could not be closed automatically:' + #13#10 +
      ExistingExe + #13#10#13#10 +
      'Please close Deskmon (and any tools holding its files open), then click Retry to continue installation.',
      mbError, MB_RETRYCANCEL);

    if Choice = IDRETRY then
      TryKillDeskmonProcesses()
    else
    begin
      Result := 'Installation canceled because Deskmon.exe is locked by another process.';
      Exit;
    end;
  end;
end;

// Bersihkan log, cache, icon_cache, updates saat uninstall; dan hapus database + profil hanya jika dicentang
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  AppLogsDir, LocalDataDir, RoamingDataDir, OrgLocalDir, OrgRoamingDir, DbPath: String;
  HasDb, HasProfiles, Aborted: Boolean;
begin
  if CurUninstallStep = usUninstall then
  begin
    ShouldDeleteDatabase := False;
    TryKillDeskmonProcesses();

    LocalDataDir := ExpandConstant('{localappdata}\Pranala\Deskmon');
    RoamingDataDir := ExpandConstant('{userappdata}\Pranala\Deskmon');
    DbPath := LocalDataDir + '\deskmon.db';

    HasDb := FileExists(DbPath) or FileExists(DbPath + '-wal') or FileExists(DbPath + '-shm');
    HasProfiles := DirExists(RoamingDataDir);

    if HasDb or HasProfiles then
      ShouldDeleteDatabase := AskUninstallDatabaseOption(HasDb, HasProfiles);
  end
  else if CurUninstallStep = usPostUninstall then
  begin
    Aborted := False;
    AppLogsDir := ExpandConstant('{app}\logs');
    LocalDataDir := ExpandConstant('{localappdata}\Pranala\Deskmon');
    RoamingDataDir := ExpandConstant('{userappdata}\Pranala\Deskmon');
    OrgLocalDir := ExpandConstant('{localappdata}\Pranala');
    OrgRoamingDir := ExpandConstant('{userappdata}\Pranala');

    // 1. Bersihkan seluruh jejak sementara aplikasi (logs, cache, icon_cache, updates)
    DeleteDirWithRetryPrompt(AppLogsDir, 'application logs folder', Aborted);
    DeleteDirWithRetryPrompt(LocalDataDir + '\logs', 'local logs folder', Aborted);
    DeleteDirWithRetryPrompt(LocalDataDir + '\cache', 'QML cache folder', Aborted);
    DeleteDirWithRetryPrompt(LocalDataDir + '\icon_cache', 'app icon cache folder', Aborted);
    DeleteDirWithRetryPrompt(LocalDataDir + '\updates', 'staged updates folder', Aborted);

    // 2. Hapus database lokal, file WAL/SHM, profil, dan preferensi jika dicentang oleh pengguna
    if ShouldDeleteDatabase and (not Aborted) then
    begin
      DbPath := LocalDataDir + '\deskmon.db';
      DeleteFileWithRetryPrompt(DbPath + '-wal', 'database WAL journal', Aborted);
      DeleteFileWithRetryPrompt(DbPath + '-shm', 'database SHM index', Aborted);
      DeleteFileWithRetryPrompt(DbPath, 'local database (deskmon.db)', Aborted);
      DeleteDirWithRetryPrompt(RoamingDataDir, 'saved user profile folder', Aborted);

      if not Aborted then
      begin
        RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Pranala\Deskmon');
        RegDeleteKeyIfEmpty(HKCU, 'Software\Pranala');
      end;
    end;

    // 3. Hapus folder induk jika sudah benar-benar kosong
    RemoveDir(LocalDataDir);
    RemoveDir(OrgLocalDir);
    RemoveDir(RoamingDataDir);
    RemoveDir(OrgRoamingDir);
    RemoveDir(ExpandConstant('{app}'));
  end;
end;
