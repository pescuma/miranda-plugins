/* 
Copyright (C) 2006 Ricardo Pescuma Domenecci

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



// Prototypes ///////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
#ifdef UNICODE
	"History Log (Unicode)",
#else
	"History Log",
#endif
	PLUGIN_MAKE_VERSION(0,0,0,1),
	"Logs history events to disk on the fly",
	"Ricardo Pescuma Domenecci",
	"",
	"© 2008 Ricardo Pescuma Domenecci",
	"http://pescuma.org/miranda/historylog",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0x53fbd1b8, 0xb6cd, 0x46af, { 0x8a, 0x89, 0xf8, 0xd9, 0x18, 0xb3, 0xe1, 0x4c } } // {53FBD1B8-B6CD-46af-8A89-F8D918B3E14C}
#else

	{ 0xef85ed7d, 0x7696, 0x45c2, { 0x87, 0xf0, 0xff, 0x36, 0x20, 0x35, 0xf9, 0x65 } } // {EF85ED7D-7696-45c2-87F0-FF362035F965}
#endif
};


HINSTANCE hInst;
PLUGINLINK *pluginLink;
LIST_INTERFACE li;
char *metacontacts_proto = NULL;

ContactAsyncQueue *queue;

static HANDLE hHooks[3] = {0};

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);
int DbEventAdded(WPARAM wParam, LPARAM lParam);

void ProcessEvent(HANDLE hContact, void *param);


// Functions ////////////////////////////////////////////////////////////////////////////


extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) 
{
	hInst = hinstDLL;
	return TRUE;
}


extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion) 
{
	pluginInfo.cbSize = sizeof(PLUGININFO);
	return (PLUGININFO*) &pluginInfo;
}


extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	pluginInfo.cbSize = sizeof(PLUGININFOEX);
	return &pluginInfo;
}


static const MUUID interfaces[] = { MIID_HISTORYLOG, MIID_LAST };
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	if (!ServiceExists(MS_HISTORYEVENTS_GET_TEXT))
	{
		MessageBox(NULL, _T("History Log depends on History Events to work!"), _T("History Log"), MB_OK | MB_ICONASTERISK);
		return -1;
	}

	init_mir_malloc();
	mir_getLI(&li);
	
	// hooks
	hHooks[0] = HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	hHooks[1] = HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	hHooks[2] = HookEvent(ME_DB_EVENT_ADDED, DbEventAdded);

	InitOptions();

	queue = new ContactAsyncQueue(ProcessEvent, 1);

	return 0;
}

extern "C" int __declspec(dllexport) Unload(void) 
{
	return 0;
}

// Called when all the modules are loaded
int ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	// add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME, 0);

	if (ServiceExists(MS_MC_GETPROTOCOLNAME))
		metacontacts_proto = (char *) CallService(MS_MC_GETPROTOCOLNAME, 0, 0);

    // updater plugin support
    if(ServiceExists(MS_UPDATE_REGISTER))
	{
		Update upd = {0};
		char szCurrentVersion[30];

		upd.cbSize = sizeof(upd);
		upd.szComponentName = pluginInfo.shortName;

		upd.szUpdateURL = UPDATER_AUTOREGISTER;

		upd.szBetaVersionURL = "http://pescuma.org/miranda/historylog_version.txt";
		upd.szBetaChangelogURL = "http://pescuma.org/miranda/historylog#Changelog";
		upd.pbBetaVersionPrefix = (BYTE *)"HistoryEvents ";
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);
#ifdef UNICODE
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/historylogW.zip";
#else
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/historylog.zip";
#endif

		upd.pbVersion = (BYTE *)CreateVersionStringPlugin((PLUGININFO*) &pluginInfo, szCurrentVersion);
		upd.cpbVersion = strlen((char *)upd.pbVersion);

        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	}

	return 0;
}


int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	delete queue;
	queue = NULL;

	DeInitOptions();

	for(int i = 0; i < MAX_REGS(hHooks); i++)
		if (hHooks[i] != NULL)
			UnhookEvent(hHooks[i]);

	return 0;
}


HANDLE GetMetaContact(HANDLE hContact)
{
	if (!ServiceExists(MS_MC_GETMETACONTACT))
		return NULL;

	return (HANDLE) CallService(MS_MC_GETMETACONTACT, (WPARAM) hContact, 0);
}


char * TrimRight(char *str) {
	int e;
	for(e = strlen(str)-1; e >= 0 && (str[e] == ' ' || str[e] == '\t' || str[e] == '\r' || str[e] == '\n'); e--) ;
	str[e+1] = '\0';
	return str;
}

wchar_t * TrimRight(wchar_t *str) {
	int e;
	for(e = lstrlenW(str)-1; e >= 0 && (str[e] == L' ' || str[e] == L'\t' || str[e] == L'\r' || str[e] == L'\n'); e--) ;
	str[e+1] = L'\0';
	return str;
}


BOOL IsProtocolEnabled(const char *proto)
{
	if (proto == NULL)
		return FALSE;

	if ((CallProtoService(proto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IM) == 0)
		return FALSE;

	char setting[256];
	mir_snprintf(setting, sizeof(setting), "%sEnabled", proto);
	return (BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, setting, TRUE);
}


BOOL IsEventEnabled(WORD eventType)
{
	char setting[256];
	mir_snprintf(setting, sizeof(setting), "Event%dEnabled", (int) eventType);
	return (BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, setting, eventType == EVENTTYPE_MESSAGE || eventType == EVENTTYPE_URL);
}


void SetEventEnabled(WORD eventType, BOOL enable)
{
	char setting[256];
	mir_snprintf(setting, sizeof(setting), "Event%dEnabled", (int) eventType);
	DBWriteContactSettingByte(NULL, MODULE_NAME, setting, enable);
}


int DbEventAdded(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	HANDLE hDbEvent = (HANDLE) lParam;

	if (hContact == NULL)
		return 0;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (!IsProtocolEnabled(proto))
		return 0;

	queue->Add(500, hDbEvent, hContact);

	return 0;
}


void ProcessEvent(HANDLE hDbEvent, void *param)
{
	HANDLE hContact = (HANDLE) param;

	if (hContact == NULL)
		return;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (!IsProtocolEnabled(proto))
		return;

	if (metacontacts_proto != NULL)
	{
		BOOL isSub = (GetMetaContact(hContact) != NULL);
		if (isSub && IsProtocolEnabled(metacontacts_proto))
			return;
	}

	DBEVENTINFO dbe = {0};
	dbe.cbSize = sizeof(dbe);
	if (CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) &dbe) != 0)
		return;

	if (!IsEventEnabled(dbe.eventType))
		return;

	TCHAR *eventText = HistoryEvents_GetTextT(hDbEvent, NULL);
	int flags = HistoryEvents_GetFlags(dbe.eventType);

	TCHAR date[128];
	DBTIMETOSTRINGT tst = {0};
	tst.szFormat = _T("d s");
	tst.szDest = date;
	tst.cbDest = 128;
	CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM) dbe.timestamp, (LPARAM) &tst);

	Buffer<TCHAR> text;
	text.append(_T("["));
	text.append(date);
	text.append(_T("] "));

	if (flags & HISTORYEVENTS_FLAG_EXPECT_CONTACT_NAME_BEFORE)
	{
		if (dbe.flags & DBEF_SENT)
		{
			CONTACTINFO ci;
			ZeroMemory(&ci, sizeof(ci));
			ci.cbSize = sizeof(ci);
			ci.hContact = NULL;
			ci.szProto = proto;
			ci.dwFlag = CNF_DISPLAY;

#ifdef UNICODE
			ci.dwFlag |= CNF_UNICODE;
#endif

			if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) 
			{
				text.append(ci.pszVal);
				mir_free(ci.pszVal);
			}
		}
		else
		{
			text.append((TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, GCDNF_TCHAR | GCDNF_NOCACHE));
		}
		text.append(_T(": "));
	}

	text.append(eventText);
	text.pack();

	char path[1024];
	CallService(MS_DB_GETPROFILEPATH, (WPARAM) MAX_REGS(path), (LPARAM) path);

	Buffer<TCHAR> filename;
	filename.append(path);
	filename.append(_T("\\"));

	TCHAR *protocol = mir_a2t(proto);
	TCHAR *group = NULL;
	DBVARIANT db = {0};
	if (DBGetContactSettingTString(hContact, "CList", "Group", &db) == 0)
	{
		if (db.ptszVal != NULL)
			group = mir_tstrdup(db.ptszVal);
		DBFreeVariant(&db);
	}

	TCHAR *vars[] = { 
		_T("group"), group == NULL ? _T("") : group, 
		_T("protocol"), protocol
	};

	ReplaceTemplate(&filename, hContact, opts.filename_pattern, vars, MAX_REGS(vars));
	filename.pack();

	// Assert folder exists
	TCHAR *p = _tcschr(filename.str, _T('\\'));
	if (p != NULL)
		p = _tcschr(p+1, _T('\\'));
	while(p != NULL)
	{
		*p = _T('\0');
		CreateDirectory(filename.str, NULL);
		*p = _T('\\');
		p = _tcschr(p+1, _T('\\'));
	}

	FILE *out = _tfopen(filename.str, _T("a"));
	if (out != NULL)
	{
		char *utf = mir_utf8encodeT(text.str);
		fprintf(out, "%s\n", utf);
		mir_free(utf);
		fclose(out);
	}

	mir_free(protocol);
	mir_free(group);
	mir_free(eventText);
}

