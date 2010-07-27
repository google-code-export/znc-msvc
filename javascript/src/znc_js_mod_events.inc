/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

/* WARNING: The contents and their order of this array have to be in
	sync with the contents of the EModEvId enum, as defined in znc_js_mod_events.h! */

static const char* s_mod_event_names[] = {
	"OnLoad",
	"OnPreRehash",
	"OnPostRehash",
	"OnIRCDisconnected",
	"OnIRCConnected",
	"OnIRCConnecting",
	"OnIRCRegistration",
	"OnBroadcast",

	"OnChanPermission",
	"OnOp",
	"OnDeop",
	"OnVoice",
	"OnDevoice",
	"OnRawMode",
	"OnMode",

	"OnRaw",

	"OnStatusCommand",
	"OnModCommand",
	"OnModNotice",
	"OnModCTCP",

	"OnQuit",
	"OnNick",
	"OnKick",
	"OnJoin",
	"OnPart",

	"OnClientLogin",
	"OnClientDisconnect",
	"OnUserRaw",
	"OnUserCTCPReply",
	"OnUserCTCP",
	"OnUserAction",
	"OnUserMsg",
	"OnUserNotice",
	"OnUserJoin",
	"OnUserPart",
	"OnUserTopic",
	"OnUserTopicRequest",

	"OnCTCPReply",
	"OnPrivCTCP",
	"OnChanCTCP",
	"OnPrivAction",
	"OnChanAction",
	"OnPrivMsg",
	"OnChanMsg",
	"OnPrivNotice",
	"OnChanNotice",
	"OnTopic",

	NULL
};
