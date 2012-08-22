# Updates ModulesList.make and ModulesList.iss from
#  filesystem (module dirs with .cpp files)
# no beauty.

$mod_package_dirs = @("", "extra\", "extra_win32\")

Function GetMakeFileFileList([string]$Extension)
{
	[string]$result = ""
	foreach($p in $mod_package_dirs)
	{
		$list = Get-Childitem -Name "..\..\modules\$p*.cpp" |
			% { $_ -replace "cpp$", $Extension } |
			% { $_ -replace "^", "`$(MAKDIR)\$p" } |
			% { $_ -replace "$", " \`r`n" }
		$result += " " + $list
	}
	return ($result | % { $_ -replace "\s*\\\s*$", "" })
}

"DLLS=" + (GetMakeFileFileList "dll") + "`r`n" | Out-File -Encoding "UTF8" ModulesList.make

"OBJS=" + (GetMakeFileFileList "obj") + "`r`n" | Out-File -Encoding "UTF8" -Append ModulesList.make

Function GetInnoSetupFileList()
{
	[string]$result = ""
	foreach($p in $mod_package_dirs)
	{
		Get-Childitem "..\..\modules\$p*.cpp" | ForEach-Object {
			$name = ($_.name -replace "\.cpp$", "")
			if($p -eq "") { $compo = "core" } else { $compo = ($p -replace "\\$", "") }
			$result += "`r`nSource: `"{#SourceFileDir32}\modules\$name.dll`"; DestDir: `"{app}\modules`"; Flags: ignoreversion; Check: not Is64BitInstallMode; "
			if($_.name -ne "win32_service_helper.cpp") { $result += "Components: modules/$compo" }
			$result += "`r`nSource: `"{#SourceFileDir64}\modules\$name.dll`"; DestDir: `"{app}\modules`"; Flags: ignoreversion; Check: Is64BitInstallMode; "
			if($_.name -ne "win32_service_helper.cpp") { $result += "Components: modules/$compo" }

			if(Test-Path -Path ("..\..\modules\$p" + "data\$name") -PathType Container)
			{
				$result += "`r`nSource: `"{#SourceCodeDir}\modules\$p" + "data\$name\*`"; DestDir: `"{app}\modules\data\$name`"; Excludes: `".svn`"; Flags: recursesubdirs; Components: modules/$compo"
			}
		}
	}
	return $result
}

(GetInnoSetupFileList) | Out-File -Encoding "UTF8" "ModulesList.iss"
