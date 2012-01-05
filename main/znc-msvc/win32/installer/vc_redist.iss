; based on http://code.google.com/p/infekt/source/browse/trunk/project/setup-script.iss
; GPLv2

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


procedure VC_Redist_InitializeWizard();
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
