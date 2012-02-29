// Utility functions for Inno Setup
//   used to add/remove programs from the windows firewall rules
// Code originally from http://news.jrsoftware.org/news/innosetup/msg43799.html

// could use some error handling+feedback...

const
  NET_FW_SCOPE_ALL = 0;
  NET_FW_IP_VERSION_ANY = 2;

procedure AddFirewallException(AppName, FileName: string);
var
	FirewallObject: Variant;
	FirewallManager: Variant;
	FirewallProfile: Variant;
begin
	try
		FirewallObject := CreateOleObject('HNetCfg.FwAuthorizedApplication');
		FirewallObject.ProcessImageFileName := FileName;
		FirewallObject.Name := AppName;
		FirewallObject.Scope := NET_FW_SCOPE_ALL;
		FirewallObject.IpVersion := NET_FW_IP_VERSION_ANY;
		FirewallObject.Enabled := True;
		FirewallManager := CreateOleObject('HNetCfg.FwMgr');
		FirewallProfile := FirewallManager.LocalPolicy.CurrentProfile;
		FirewallProfile.AuthorizedApplications.Add(FirewallObject);
	except
	end;
end;

procedure RemoveFirewallException(FileName: string);
var
	FirewallManager: Variant;
	FirewallProfile: Variant;
begin
	try
		FirewallManager := CreateOleObject('HNetCfg.FwMgr');
		FirewallProfile := FirewallManager.LocalPolicy.CurrentProfile;
		FireWallProfile.AuthorizedApplications.Remove(FileName);
	except
	end;
end;
