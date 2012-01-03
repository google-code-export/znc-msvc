; based on http://code.google.com/p/infekt/source/browse/trunk/project/setup-script.iss
; GPLv2

#define SourceFileDir32 "..\..\..\build-out\Win32-Release"
#define SourceFileDir64 "..\..\..\build-out\x64-Release"
#define COMServiceControlCLSID "{{DC2BF05E-2451-435E-A24C-1B9BA804B5F0}"

#include "it_download.iss"

[Setup]
AppId={{819933F3-A105-4B40-8920-6D74139B5464}
AppName=ZNC IRC Bouncer
AppVerName=ZNC 0.202plus
AppVersion=0.202plus
AppPublisher=Ingmar Runge
AppPublisherURL=http://znc-msvc.googlecode.com/
DefaultDirName={pf}\ZNC
DefaultGroupName=ZNC IRC Bouncer
AllowNoIcons=yes
OutputDir=..\..\..\build-out
OutputBaseFilename=znc-setup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
MinVersion=0,5.1sp2
UninstallDisplayIcon={app}\ZNC_Tray.exe
UninstallDisplayName=ZNC IRC Bouncer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "core"; Description: "Core Files (required)"; Types: full compact custom; Flags: fixed
Name: "service"; Description: "ZNC Service"; Types: full compact
Name: "service/autorun"; Description: "Set Service to Run on Startup"; Types: full
Name: "service/tray"; Description: "Install Tray Control"; Types: full compact
Name: "service/tray/desktop"; Description: "Create a Desktop Icon"; Types: full
Name: "service/tray/autorun"; Description: "Launch Tray Control on Startup"; Types: full
Name: "modules"; Description: "Modules"; Types: full compact
Name: "modules/core"; Description: "Core Modules"; Types: full compact
Name: "modules/extra"; Description: "Extra Modules"; Types: full
Name: "modules/extra_win32"; Description: "Useful third-party Modules"; Types: full
Name: "webskins"; Description: "Web Interface Themes"; Types: full compact custom; Flags: fixed
Name: "webskins\default"; Description: "Default Theme"; Types: full compact custom; Flags: fixed
Name: "webskins\darkclouds"; Description: "Dark Clouds Theme"; Types: full
Name: "webskins\forest"; Description: "Forest Theme"; Types: full
Name: "webskins\ice"; Description: "Ice Theme"; Types: full
Name: "vcredist"; Description: "Microsoft Visual C++ 2010 Runtime (if not installed)"; Types: full compact custom; Flags: fixed

[Files]
Source: "{#SourceFileDir32}\ZNC.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ZNC.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\ZNC.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ZNC.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#SourceFileDir32}\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\cares.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\cares.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#SourceFileDir32}\ZNC_Service.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service
Source: "{#SourceFileDir64}\ZNC_Service.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service
Source: "{#SourceFileDir32}\service_provider.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service
Source: "{#SourceFileDir64}\service_provider.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service
Source: "{#SourceFileDir32}\ZNC_Tray.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service/tray
Source: "{#SourceFileDir64}\ZNC_Tray.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service/tray

Source: "{#SourceFileDir32}\COMServiceControl.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service/tray
Source: "{#SourceFileDir64}\COMServiceControl.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service/tray

[Icons]
Name: "{group}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Components: service/tray
Name: "{group}\ZNC (CLI)"; Filename: "{app}\ZNC.exe"
Name: "{group}\More\{cm:ProgramOnTheWeb,ZNC-MSVC}"; Filename: "http://znc-msvc.googlecode.com/"; Flags: excludefromshowinnewinstall
Name: "{group}\More\{cm:ProgramOnTheWeb,ZNC}"; Filename: "http://znc.in/"; Flags: excludefromshowinnewinstall
Name: "{group}\More\{cm:UninstallProgram,ZNC}"; Filename: "{uninstallexe}"; Flags: excludefromshowinnewinstall
Name: "{userdesktop}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Components: service/tray/desktop
Name: "{userstartup}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Parameters: "--autorun"; Components: service/tray/autorun

[Run]
Filename: "{app}\ZNC_Service.exe"; Parameters: "--install"; Flags: runhidden; Components: service and service/autorun
Filename: "{app}\ZNC_Service.exe"; Parameters: "--install --manual"; Flags: runhidden; Components: service and not service/autorun
Filename: "{app}\COMServiceControl.exe"; Parameters: "/RegServer"; Flags: runhidden; Components: service/tray
; can't use [Registry] because this needs to be done *after* /RegServer (same during uninstall where it needs to be done before /UnRegServer):
Filename: "{cmd}"; Parameters: "/c reg add HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID} /v LocalizedString /t REG_EXPAND_SZ /d ""@{app}\COMServiceControl.exe,-101"" /f"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; Parameters: "/c reg add HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID}\Elevation /v Enabled /t REG_DWORD /d 1 /f"; Flags: runhidden; Components: service/tray

[UninstallRun]
; should use something less brutal than taskkill in the future:
Filename: "{cmd}"; parameters: "/c taskkill /f /im ZNC_Tray.exe"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; parameters: "/c net stop ZNC"; Flags: runhidden; Components: service
Filename: "{app}\ZNC_Service.exe"; Flags: runhidden; Parameters: "--uninstall"; Components: service
Filename: "{cmd}"; Parameters: "/c reg delete HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID} /v LocalizedString /f"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; Parameters: "/c reg delete HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID}\Elevation /f"; Flags: runhidden; Components: service/tray
Filename: "{app}\COMServiceControl.exe"; Parameters: "/UnRegServer"; Flags: runhidden; Components: service/tray

[Code]
function MsiQueryProductState(ProductCode: String): Integer; external 'MsiQueryProductStateW@msi.dll stdcall';

var
	cppRuntimeInstalled: Boolean;

const
	INSTALLSTATE_DEFAULT = 5;

	MSVC_X64_URL = 'http://download.microsoft.com/download/A/8/0/A80747C3-41BD-45DF-B505-E9710D2744E0/vcredist_x64.exe';
	MSVC_X86_URL = 'http://download.microsoft.com/download/C/6/D/C6D0FD4E-9E53-4897-9B91-836EBA2AACD3/vcredist_x86.exe';


function InstallCppRuntime(): Boolean;
begin
  Result := not cppRuntimeInstalled;
end;


procedure InitializeWizard();
begin
	ITD_Init();

	if Is64BitInstallMode() then
	begin
		ITD_AddFile(MSVC_X64_URL, expandconstant('{tmp}\vcredist_x64.exe'));

		cppRuntimeInstalled := (MsiQueryProductState('{1D8E6291-B0D5-35EC-8441-6616F567A0F7}') = INSTALLSTATE_DEFAULT) // with SP1
		  or (MsiQueryProductState('{DA5E371C-6333-3D8A-93A4-6FD5B20BCC6E}') = INSTALLSTATE_DEFAULT); // without SP1
	end
	else
	begin
		ITD_AddFile(MSVC_X86_URL, expandconstant('{tmp}\vcredist_x86.exe'));

		cppRuntimeInstalled := (MsiQueryProductState('{F0C3E5D1-1ADE-321E-8167-68EF0DE699A5}') = INSTALLSTATE_DEFAULT) // with SP1
		  or (MsiQueryProductState('{196BB40D-1578-3D01-B289-BEFC77A11A1E}') = INSTALLSTATE_DEFAULT); // without SP1
	end;

	if InstallCppRuntime() then
	begin
		ITD_DownloadAfter(wpReady);
	end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
	exepath: String;
	exitcode: Integer;
begin
	if (CurStep = ssInstall) and InstallCppRuntime() then
	begin
		// Install C++ Runtime. This MUST NOT be done in [Run] because
		// regsvr32'ing the shell extension requires the runtimes to be installed
		// already whereas [Run] would only install them afterwards.

		if Is64BitInstallMode() then
			exepath := ExpandConstant('{tmp}\vcredist_x64.exe')
		else
			exepath := ExpandConstant('{tmp}\vcredist_x86.exe');

		if FileExists(exepath) then
		begin
			if Is64BitInstallMode() then
				WizardForm.StatusLabel.Caption := 'Installing Microsoft Visual C++ 2010 (x64) runtime...'
			else
				WizardForm.StatusLabel.Caption := 'Installing Microsoft Visual C++ 2010 (x86) runtime...';

			Exec(exepath, '/q /norestart', '', SW_HIDE, ewWaitUntilTerminated, exitcode);
		end;
	end;
end;
