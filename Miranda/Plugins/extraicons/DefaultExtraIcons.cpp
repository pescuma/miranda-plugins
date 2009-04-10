/*
 Copyright (C) 2009 Ricardo Pescuma Domenecci

 This is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this file; see the file license.txt.  If
 not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 */

#include "commons.h"

/*
 0, // EXTRA_ICON_VISMODE
 1, // EXTRA_ICON_EMAIL
 2, // EXTRA_ICON_PROTO
 3, // EXTRA_ICON_SMS
 4, // EXTRA_ICON_ADV1
 5, // EXTRA_ICON_ADV2
 6, // EXTRA_ICON_WEB
 7, // EXTRA_ICON_CLIENT
 8, // EXTRA_ICON_ADV3
 9, // EXTRA_ICON_ADV4
 */

static void ProtocolInit();
static void DBExtraIconsInit();
static void VisibilityInit();

void DefaultExtraIcons_Load()
{
	ProtocolInit();
	DBExtraIconsInit();
	VisibilityInit();
}

void DefaultExtraIcons_Unload()
{
}

// DB extra icons ///////////////////////////////////////////////////////////////////////

HANDLE hExtraVisibility = NULL;

static void SetVisibility(HANDLE hContact, int apparentMode, BOOL clear)
{
	if (hContact == NULL)
		return;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return;

	if (apparentMode < 0)
		apparentMode = DBGetContactSettingWord(hContact, proto, "ApparentMode", 0);

	const char *ico = NULL;

	if (DBGetContactSettingByte(hContact, proto, "ChatRoom", 0))
	{
		// Is chat
		if (apparentMode == ID_STATUS_OFFLINE)
			ico = "ChatActivity";
	}
	else
	{
		// Not chat
		if (apparentMode == ID_STATUS_OFFLINE)
			ico = "NeverVis";

		else if (apparentMode == ID_STATUS_ONLINE)
			ico = "AlwaysVis";
	}

	if (ico == NULL && !clear)
		return;

	ExtraIcon_SetIcon(hExtraVisibility, hContact, ico);
}

static void VisibilityInit()
{
	hExtraVisibility = ExtraIcon_Register("visibility", "Visibility/Chat activity", "AlwaysVis");

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		SetVisibility(hContact, -1, FALSE);

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

static int EmailOnClick(WPARAM wParam, LPARAM lParam);
static int HomepageOnClick(WPARAM wParam, LPARAM lParam);

struct Info
{
	const char *name;
	const char *desc;
	const char *icon;
	const char *db[4];
	int (*OnClick)(WPARAM wParam, LPARAM lParam);
	HANDLE hExtraIcon;

} infos[] = {

{ "email", "E-mail", "core_main_14", { NULL, "e-mail", "UserInfo", "Mye-mail0" }, &EmailOnClick, NULL },

{ "sms", "Phone/SMS", "core_main_17", { NULL, "Cellular", "UserInfo", "MyPhone0" }, NULL, NULL },

{ "homepage", "Homepage", "core_main_2", { NULL, "Homepage", "UserInfo", "Homepage" }, &HomepageOnClick, NULL },

};

static void SetExtraIcons(HANDLE hContact)
{
	if (hContact == NULL)
		return;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return;

	for (unsigned int i = 0; i < MAX_REGS(infos); ++i)
	{
		Info &info = infos[i];

		bool show = false;
		for (unsigned int j = 0; !show && j < MAX_REGS(info.db); j += 2)
		{
			if (info.db[j + 1] == NULL)
				break;

			DBVARIANT dbv = { 0 };
			if (!DBGetContactSettingString(hContact, info.db[j] == NULL ? proto : info.db[j], info.db[j+1], &dbv))
			{
				if (!IsEmpty(dbv.pszVal))
					show = true;
				DBFreeVariant(&dbv);
			}
		}

		if (show)
			ExtraIcon_SetIcon(info.hExtraIcon, hContact, info.icon);
	}
}

static int SettingChanged(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*) lParam;

	if (hContact == NULL)
		return 0;

	char *proto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return 0;

	bool isProto = (strcmp(cws->szModule, proto) == 0);

	if (isProto && strcmp(cws->szSetting, "ApparentMode") == 0)
	{
		SetVisibility(hContact, cws->value.type == DBVT_DELETED ? 0 : cws->value.wVal, TRUE);
		return 0;
	}

	for (unsigned int i = 0; i < MAX_REGS(infos); ++i)
	{
		Info &info = infos[i];

		for (unsigned int j = 0; j < MAX_REGS(info.db); j += 2)
		{
			if (info.db[j + 1] == NULL)
				break;
			if (info.db[j] == NULL && !isProto)
				continue;
			if (info.db[j] != NULL && strcmp(cws->szModule, info.db[j]))
				continue;
			if (strcmp(cws->szSetting, info.db[j + 1]))
				continue;

			bool show = (cws->value.type != DBVT_DELETED && !IsEmpty(cws->value.pszVal));
			ExtraIcon_SetIcon(info.hExtraIcon, hContact, show ? info.icon : NULL);

			break;
		}
	}

	return 0;
}

static int EmailOnClick(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	if (hContact == NULL)
		return 0;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return 0;

	Info &info = infos[0];

	bool found = false;
	for (unsigned int j = 0; !found && j < MAX_REGS(info.db); j += 2)
	{
		if (info.db[j + 1] == NULL)
			break;

		DBVARIANT dbv = { 0 };
		if (!DBGetContactSettingString(hContact, info.db[j] == NULL ? proto : info.db[j], info.db[j+1], &dbv))
		{
			if (!IsEmpty(dbv.ptszVal))
			{
				char cmd[1024];
				mir_snprintf(cmd, MAX_REGS(cmd), "mailto:%s", dbv.ptszVal);
				ShellExecute(NULL, "open", cmd, NULL, NULL, SW_SHOW);
				found = true;
			}

			DBFreeVariant(&dbv);
		}
	}

	return 0;
}

static int HomepageOnClick(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	if (hContact == NULL)
		return 0;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return 0;

	Info &info = infos[2];

	bool found = false;
	for (unsigned int j = 0; !found && j < MAX_REGS(info.db); j += 2)
	{
		if (info.db[j + 1] == NULL)
			break;

		DBVARIANT dbv = { 0 };
		if (!DBGetContactSettingString(hContact, info.db[j] == NULL ? proto : info.db[j], info.db[j+1], &dbv))
		{
			if (!IsEmpty(dbv.ptszVal))
			{
				ShellExecute(NULL, "open", dbv.ptszVal, NULL, NULL, SW_SHOW);
				found = true;
			}

			DBFreeVariant(&dbv);
		}
	}

	return 0;
}

static void DBExtraIconsInit()
{
	for (unsigned int i = 0; i < MAX_REGS(infos); ++i)
	{
		Info &info = infos[i];
		info.hExtraIcon = ExtraIcon_Register(info.name, info.desc, info.icon, info.OnClick);
	}

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		SetExtraIcons(hContact);

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	hHooks.push_back(HookEvent(ME_DB_CONTACT_SETTINGCHANGED, SettingChanged));
}

// Protocol /////////////////////////////////////////////////////////////////////////////

struct ProtoInfo
{
	string proto;
	HANDLE hImage;
};

vector<ProtoInfo> protos;

HANDLE hExtraProto = NULL;

static int ProtocolRebuildIcons(WPARAM wParam, LPARAM lParam)
{
	protos.clear();
	return 0;
}

static ProtoInfo *FindProto(const char * proto)
{
	for (unsigned int i = 0; i < protos.size(); ++i)
	{
		ProtoInfo *pi = &protos[i];
		if (strcmp(pi->proto.c_str(), proto) == 0)
			return pi;
	}

	HICON hIcon = LoadSkinnedProtoIcon(proto, ID_STATUS_ONLINE);
	if (hIcon == NULL)
		return NULL;

	HANDLE hImage = (HANDLE) CallService(MS_CLIST_EXTRA_ADD_ICON, (WPARAM) hIcon, 0);
	if (hImage == NULL)
		return NULL;

	ProtoInfo tmp;
	tmp.proto = proto;
	tmp.hImage = hImage;
	protos.push_back(tmp);

	return &protos[protos.size() - 1];
}

static int ProtocolApplyIcon(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	int slot = (int) lParam;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (IsEmpty(proto))
		return 0;

	HANDLE hImage = NULL;

	ProtoInfo *pi = FindProto(proto);
	if (pi != NULL)
		hImage = pi->hImage;

	IconExtraColumn iec = { 0 };
	iec.cbSize = sizeof(iec);
	iec.ColumnType = slot;
	iec.hImage = (hImage == NULL ? (HANDLE) -1 : hImage);
	CallService(MS_CLIST_EXTRA_SET_ICON, (WPARAM) hContact, (LPARAM) &iec);

	return 0;
}

static int ProtocolOnClick(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	if (hContact == NULL)
		return 0;

	CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) hContact, 0);
	return 0;
}

static void ProtocolInit()
{
	hExtraProto = ExtraIcon_Register("protocol", "Account", "core_main_34", &ProtocolRebuildIcons, &ProtocolApplyIcon,
			&ProtocolOnClick);
}
