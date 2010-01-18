/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_JS_MOD_EVENTS_H
#define _ZNC_JS_MOD_EVENTS_H

/* WARNING: The contents and their order of this enum have to be in
	sync with the contents of s_mod_event_names, as defined in znc_js_mod_events.inc! */

typedef enum _mod_event_t {
	ModEv_OnLoad = 0,
	ModEv_OnPreRehash,
	ModEv_OnPostRehash,
	ModEv_OnIRCDisconnected,
	ModEv_OnIRCConnected,
	ModEv_OnIRCConnecting,
	ModEv_OnIRCRegistration,
	ModEv_OnBroadcast,

	ModEv_OnChanPermission,
	ModEv_OnOp,
	ModEv_OnDeop,
	ModEv_OnVoice,
	ModEv_OnDevoice,
	ModEv_OnRawMode,
	ModEv_OnMode,

	ModEv_OnRaw,

	ModEv_OnStatusCommand,
	ModEv_OnModCommand,
	ModEv_OnModNotice,
	ModEv_OnModCTCP,

	ModEv_OnQuit,
	ModEv_OnNick,
	ModEv_OnKick,
	ModEv_OnJoin,
	ModEv_OnPart,

	ModEv_OnClientLogin,
	ModEv_OnClientDisconnect,
	ModEv_OnUserRaw,
	ModEv_OnUserCTCPReply,
	ModEv_OnUserCTCP,
	ModEv_OnUserAction,
	ModEv_OnUserMsg,
	ModEv_OnUserNotice,
	ModEv_OnUserJoin,
	ModEv_OnUserPart,
	ModEv_OnUserTopic,
	ModEv_OnUserTopicRequest,

	ModEv_OnCTCPReply,
	ModEv_OnPrivCTCP,
	ModEv_OnChanCTCP,
	ModEv_OnPrivAction,
	ModEv_OnChanAction,
	ModEv_OnPrivMsg,
	ModEv_OnChanMsg,
	ModEv_OnPrivNotice,
	ModEv_OnChanNotice,
	ModEv_OnTopic,

	_ModEV_Max
} EModEvId;

#endif /* !_ZNC_JS_MOD_EVENTS_H */
