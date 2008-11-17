/* 
Copyright (C) 2008 Ricardo Pescuma Domenecci

Parts copied from tabSRMM code: Copyright (C) 2003 Jörgen Persson and Nightwish

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
	PLUGIN_MAKE_VERSION(0,0,0,4),
	"Logs history events to disk on the fly and speaks then",
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

BOOL shutingDown = FALSE;

ContactAsyncQueue *queue = NULL;
ContactAsyncQueue *chatQueue = NULL;

static HANDLE hHooks[3] = {0};

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);
int DbEventAdded(WPARAM wParam, LPARAM lParam);

void ProcessEvent(HANDLE hContact, void *param);
void ProcessChat(HANDLE hDummy, void *param);

void InitChat();
void FreeChat();


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

	queue = new ContactAsyncQueue(ProcessEvent);
	chatQueue = new ContactAsyncQueue(ProcessChat);

	InitChat();

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

	{
		Buffer<char> tmps[MAX_REGS(CHAT_EVENTS)];
		char *templates[MAX_REGS(CHAT_EVENTS)];
		for(int i = 0; i < MAX_REGS(CHAT_EVENTS); i++) 
		{
			tmps[i].append(CHAT_EVENTS[i].name);
			tmps[i].append('\n');
			tmps[i].append("%session%: ");
			tmps[i].append(&(CHAT_EVENTS[i].templ[2]));
			tmps[i].append('\n');
			tmps[i].append("%nick%\tNickname\n%status%\tStatus message\n%text%\tTest\n%text_sl%\tText as single line\n" 
						   "%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay\n%session%\tSession");
			tmps[i].pack();
			templates[i] = tmps[i].str;
		}
		Speak_RegisterWT(MODULE_NAME, "groupchat", "Group chat", "core_main_1", templates, MAX_REGS(templates));
	}
	{
		char *templates[] = { 
			"Sent\nto %contact%: %text%\n%text%\tMessage text\n%nick%\tNickname\n%msg_date%\tDate of message\n%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay",
			"Received\nfrom %contact%: %text%\n%text%\tMessage text\n%nick%\tNickname\n%msg_date%\tDate of message%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay" 
		};
		Speak_RegisterWT(MODULE_NAME, "message", "Message", "core_main_1", templates, MAX_REGS(templates));
	}
	{
		char *templates[] = { 
			"Sent\nto %contact%: %text%\n%text%\tURL\n%msg_date%\tDate of message\n%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay", 
			"Received\nfrom %contact%: %text%\n%text%\tURL\n%msg_date%\tDate of message\n%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay" 
		};
		Speak_RegisterWT(MODULE_NAME, "url", "URL", "core_main_2", templates, MAX_REGS(templates));
	}
	{
		char *templates[] = { 
			"Sent\nSent file to %contact%: %text%\n%text%\tFilename\n%description%\tDescription\n%msg_date%\tDate of message\n%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay",
			"Received\nReceived file from %contact%: %text%\n%text%\tFilename\n%description%\tDescription\n%msg_date%\tDate of message\n%protocol%\tProtocol\n%year%\tYear\n%month%\tMonth\n%month_name%\tMonth name\n%day%\tDay" 
		};
		Speak_RegisterWT(MODULE_NAME, "file", "File Transfer", "core_main_3", templates, MAX_REGS(templates));
	}

	return 0;
}


int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	shutingDown = TRUE;

	FreeChat();

	delete queue;
	queue = NULL;

	delete chatQueue;
	chatQueue = NULL;

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
	mir_snprintf(setting, sizeof(setting), "Enable%s", proto);
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


BOOL IsChatProtocolEnabled(const char *proto)
{
	if (proto == NULL)
		return FALSE;

	if ((CallProtoService(proto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_CHAT) == 0)
		return FALSE;

	char setting[256];
	mir_snprintf(setting, sizeof(setting), "ChatEnable%s", proto);
	return (BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, setting, TRUE);
}


BOOL IsChatEventEnabled(WORD type)
{
	char setting[256];
	mir_snprintf(setting, sizeof(setting), "Chat%dEnabled", (int) type);
	return (BOOL) DBGetContactSettingByte(NULL, MODULE_NAME, setting, type == GC_EVENT_MESSAGE 
																	|| type == GC_EVENT_TOPIC 
																	|| type == GC_EVENT_ACTION);
}


void SetChatEventEnabled(WORD type, BOOL enable)
{
	char setting[256];
	mir_snprintf(setting, sizeof(setting), "Chat%dEnabled", (int) type);
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


int GetUIDFromHContact(HANDLE contact, TCHAR* uinout, size_t uinout_len)
{
	CONTACTINFO cinfo;

	ZeroMemory(&cinfo,sizeof(CONTACTINFO));
	cinfo.cbSize = sizeof(CONTACTINFO);
	cinfo.hContact = contact;
	cinfo.dwFlag = CNF_UNIQUEID;
#ifdef UNICODE
	cinfo.dwFlag |= CNF_UNICODE;
#endif

	BOOL found = TRUE;
	if(CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&cinfo)==0)
	{
		if(cinfo.type == CNFT_ASCIIZ)
		{
			lstrcpyn(uinout, cinfo.pszVal, uinout_len);
			mir_free(cinfo.pszVal);
		}
		else if(cinfo.type == CNFT_DWORD)
		{
			_itot(cinfo.dVal,uinout,10);
		}
		else if(cinfo.type == CNFT_WORD)
		{
			_itot(cinfo.wVal,uinout,10);
		}
		else found = FALSE;
	}
	else found = FALSE;

	if (!found)
	{
#ifdef UNICODE
		// Try non unicode ver
		cinfo.dwFlag = CNF_UNIQUEID;

		found = TRUE;
		if(CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&cinfo)==0)
		{
			if(cinfo.type == CNFT_ASCIIZ)
			{
				MultiByteToWideChar(CP_ACP, 0, (char *) cinfo.pszVal, -1, uinout, uinout_len);
				mir_free(cinfo.pszVal);
			}
			else if(cinfo.type == CNFT_DWORD)
			{
				_itot(cinfo.dVal,uinout,10);
			}
			else if(cinfo.type == CNFT_WORD)
			{
				_itot(cinfo.wVal,uinout,10);
			}
			else found = FALSE;
		}
		else found = FALSE;

		if (!found)
#endif
			lstrcpy(uinout, TranslateT("Unknown UIN"));
	}
	return 0;
}


void GetDateTexts(DWORD timestamp, TCHAR year[16], TCHAR month[16], TCHAR month_name[32], TCHAR day[16])
{
	LARGE_INTEGER liFiletime;
	FILETIME filetime;
	SYSTEMTIME st;
	liFiletime.QuadPart=((__int64)11644473600+(__int64)(DWORD)CallService(MS_DB_TIME_TIMESTAMPTOLOCAL, timestamp, 0))*10000000;
	filetime.dwHighDateTime=liFiletime.HighPart;
	filetime.dwLowDateTime=liFiletime.LowPart;
	FileTimeToSystemTime(&filetime,&st);

	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, _T("yyyy"), year, 16);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, _T("MM"), month, 16);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, _T("MMMM"), month_name, 32);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, _T("dd"), day, 16);
}


TCHAR * GetContactGroup(HANDLE hContact)
{
	TCHAR *group = NULL;

	DBVARIANT db = {0};
	if (DBGetContactSettingTString(hContact, "CList", "Group", &db) == 0)
	{
		if (db.ptszVal != NULL)
			group = mir_tstrdup(db.ptszVal);
		DBFreeVariant(&db);
	}

	return group;
}


void AppendToFile(TCHAR *filename_pattern, TCHAR **vars, int numVars, HANDLE hContact, TCHAR *text)
{
	char path[1024];
	CallService(MS_DB_GETPROFILEPATH, (WPARAM) MAX_REGS(path), (LPARAM) path);

	Buffer<TCHAR> filename;
	filename.append(path);
	filename.append(_T("\\"));

	ReplaceTemplate(&filename, hContact, filename_pattern, vars, numVars);

	// Replace invalid chars
	filename.replaceAll(_T('\\'), _T('_'));
	filename.replaceAll(_T('/'), _T('_'));
	filename.replaceAll(_T(':'), _T('_'));
	filename.replaceAll(_T('*'), _T('_'));
	filename.replaceAll(_T('?'), _T('_'));
	filename.replaceAll(_T('"'), _T('_'));
	filename.replaceAll(_T('<'), _T('_'));
	filename.replaceAll(_T('>'), _T('_'));
	filename.replaceAll(_T('|'), _T('_'));

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

	BOOL writeBOM = (GetFileAttributes(filename.str) == INVALID_FILE_ATTRIBUTES);
	
	FILE *out = _tfopen(filename.str, _T("ab"));
	if (out != NULL)
	{
		if (writeBOM)
			fprintf(out, "\xEF\xBB\xBF");
		
		char *utf = mir_utf8encodeT(text);
		fprintf(out, "%s\r\n", utf);
		mir_free(utf);
		fclose(out);
	}
}


void AppendKeepingIndentationRemovingFormatting(Buffer<TCHAR> &text, TCHAR *eventText, BOOL ident)
{
	size_t spaces = text.len;
	for(TCHAR *c = eventText; *c != NULL; c++)
	{
		if (*c == _T('\r'))
		{
			if (*(c+1) == _T('\n'))
				continue;
			else
				*c = _T('\n');
		}
		
		if (*c == _T('\n'))
		{
			text.append(_T("\r\n"));
			if (ident)
				text.appendn(spaces, _T(' '));
		}
		else if (*c == _T('%'))
		{
			switch (*(c+1)) 
			{
				case _T('%'):
					text.append(_T('%'));
					c++;
					break;

				case 'b':
				case 'u':
				case 'i':
				case 'B':
				case 'U':
				case 'I':
				case 'r':
				case 'C':
				case 'F':
					c++;
					break;

				case 'c':
				case 'f':
					c += 3;
					break;

				default:
					text.append(_T('%'));
					break;
			}
		}
		else
		{
			text.append(*c);
		}
	}
}


void AppendKeepingIndentation(Buffer<TCHAR> &text, TCHAR *eventText, BOOL ident)
{
	size_t spaces = text.len;
	for(TCHAR *c = eventText; *c != NULL; c++)
	{
		if (*c == _T('\r'))
		{
			if (*(c+1) == _T('\n'))
				continue;
			else
				*c = _T('\n');
		}
		if (*c == _T('\n'))
		{
			text.append(_T("\r\n"));
			if (ident)
				text.appendn(spaces, _T(' '));
		}
		else
		{
			text.append(*c);
		}
	}
}

void GetNick(Buffer<TCHAR> &nick, HANDLE hContact, char *proto)
{
	if (hContact == NULL)
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
			nick.append(ci.pszVal);
			mir_free(ci.pszVal);
		}
	}
	else
	{
		nick.append((TCHAR *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, GCDNF_TCHAR | GCDNF_NOCACHE));
	}
}


void ProcessEvent(HANDLE hDbEvent, void *param)
{
	HANDLE hContact = (HANDLE) param;

	if (hContact == NULL)
		return;

	DBEVENTINFO dbe = {0};
	dbe.cbSize = sizeof(dbe);
	if (CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) &dbe) != 0)
		return;

	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (proto == NULL)
		return;

	scope<TCHAR *> eventText = HistoryEvents_GetTextT(hDbEvent, NULL);
	int flags = HistoryEvents_GetFlags(dbe.eventType);

	TCHAR date[128];
	DBTIMETOSTRINGT tst = {0};
	tst.szFormat = _T("d s");
	tst.szDest = date;
	tst.cbDest = 128;
	CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM) dbe.timestamp, (LPARAM) &tst);

	Buffer<TCHAR> nick;
	GetNick(nick, dbe.flags & DBEF_SENT ? NULL : hContact, proto);

	TCHAR year[16];
	TCHAR month[16];
	TCHAR month_name[32];
	TCHAR day[16];
	GetDateTexts(dbe.timestamp, year, month, month_name, day);

	scope<TCHAR *> protocol = mir_a2t(proto);
	scope<TCHAR *> group = GetContactGroup(hContact);

	TCHAR cid[512];
	GetUIDFromHContact(hContact, cid, MAX_REGS(cid));

	TCHAR *vars[] = { 
		_T("text"), eventText, 
		_T("nick"), nick.str, 
		_T("group"), group == NULL ? _T("") : group, 
		_T("protocol"), protocol,
		_T("msg_date"), date, 
		_T("year"), year,
		_T("month"), month,
		_T("month_name"), month_name, 
		_T("day"), day,
		_T("contact_id"), cid
	};

	if (dbe.eventType == EVENTTYPE_MESSAGE)
		Speak_SayExWT("message", hContact, dbe.flags & DBEF_SENT ? 0 : 1, vars, MAX_REGS(vars));
	else if (dbe.eventType == EVENTTYPE_URL)
		Speak_SayExWT("url", hContact, dbe.flags & DBEF_SENT ? 0 : 1, vars, MAX_REGS(vars));
	else if (dbe.eventType == EVENTTYPE_FILE)
		Speak_SayExWT("file", hContact, dbe.flags & DBEF_SENT ? 0 : 1, vars, MAX_REGS(vars));

	if (!IsProtocolEnabled(proto))
		return;

	if (metacontacts_proto != NULL)
	{
		BOOL isSub = (GetMetaContact(hContact) != NULL);
		if (isSub && IsProtocolEnabled(metacontacts_proto))
			return;
	}

	if (!IsEventEnabled(dbe.eventType))
		return;

	Buffer<TCHAR> text;
	text.append(_T("["));
	text.append(date);
	text.append(_T("] "));

	if (flags & HISTORYEVENTS_FLAG_EXPECT_CONTACT_NAME_BEFORE)
	{
		text.append(nick);
		text.append(_T(": "));
	}

	AppendKeepingIndentation(text, eventText, opts.ident_multiline_msgs);

	AppendToFile(opts.filename_pattern, vars, MAX_REGS(vars), hContact, text.str);
}


typedef HANDLE (*CREATESERVICEFUNCTION)(const char *,MIRANDASERVICE);
CREATESERVICEFUNCTION originalCreateServiceFunction = NULL;
MIRANDASERVICE originalChatGetEventPtrService = NULL;
MIRANDASERVICE originalChatAddEventService = NULL;
GETEVENTFUNC originalAddEvent = NULL;

struct ChatMsg
{
	int type;
	Buffer<char> proto;
	Buffer<TCHAR> name;
	DWORD time;
	Buffer<TCHAR> nick;
	Buffer<TCHAR> status;
	Buffer<TCHAR> text;
	Buffer<TCHAR> userInfo;
};

int ChatAddEvent(WPARAM wParam, LPARAM lParam)
{
	int ret;
	if (originalAddEvent != NULL)
		ret = originalAddEvent(wParam, lParam);

	else if (originalChatAddEventService != NULL)
		ret = originalChatAddEventService(wParam, lParam);

	else
		return GC_EVENT_ERROR;

	if (shutingDown || chatQueue == NULL)
		return ret;


	GCEVENT *gce = (GCEVENT*) lParam;
	if (gce == NULL)
		return ret;

	if (!(gce->dwFlags & GCEF_ADDTOLOG))
		return ret;

	GCDEST *gcd = gce->pDest;
	if (gcd == NULL || gcd->pszModule == NULL || gcd->pszModule[0] == '\0'
					|| gcd->pszID == NULL || gcd->pszID[0] == '\0') // No all/active window messages right now
		return ret;

	if (gce->dwFlags & GC_UNICODE)
	{
		if (lstrcmpW((WCHAR *) gcd->pszID, L"Network log") == 0)
			return ret;
	}
	else
	{
		if (strcmp(gcd->pszID, "Network log") == 0)
			return ret;
	}

	ChatMsg *msg = new ChatMsg();

	msg->type = gcd->iType;
	msg->time = gce->time;
	msg->proto = gcd->pszModule;

	if (gce->dwFlags & GC_UNICODE)
	{
		msg->name = (WCHAR *) gcd->pszID;
		msg->nick = (WCHAR *) gce->pszNick;
		msg->status = (WCHAR *) gce->pszStatus;
		msg->text = (WCHAR *) gce->pszText;
		msg->userInfo = (WCHAR *) gce->pszUserInfo;
	}
	else
	{
		msg->name = gce->pszUID;
		msg->nick = gce->pszNick;
		msg->status = gce->pszStatus;
		msg->text = gce->pszText;
		msg->userInfo = gce->pszUserInfo;
	}

	chatQueue->Add(500, 0, msg);

	return ret;
}


void ProcessChat(HANDLE hDummy, void *param)
{
	if (param == NULL)
		return;

	ChatMsg *msg = (ChatMsg *) param;
	msg->type = msg->type & ~GC_EVENT_HIGHLIGHT;

	msg->text.trim();
	msg->name.trim();

	TCHAR date[128];
	DBTIMETOSTRINGT tst = {0};
	tst.szFormat = _T("d s");
	tst.szDest = date;
	tst.cbDest = 128;
	CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM) msg->time, (LPARAM) &tst);

	Buffer<TCHAR> nick;
	if (msg->nick.len > 0)
	{
		nick = msg->nick;
		if (msg->userInfo.len > 0)
		{
			nick += _T(" (");
			nick += msg->userInfo;
			nick += _T(")");
		}
	}

	Buffer<TCHAR> plain;
	if (msg->text.len > 0)
	{
		AppendKeepingIndentationRemovingFormatting(plain, msg->text.str, FALSE);
		plain.replaceAll('\r', ' ');
		plain.replaceAll('\n', ' ');
		plain.trim();
	}

	TCHAR year[16];
	TCHAR month[16];
	TCHAR month_name[32];
	TCHAR day[16];
	GetDateTexts(msg->time, year, month, month_name, day);

	scope<TCHAR *> protocol = mir_a2t(msg->proto.str);

	TCHAR *vars[] = { 
		_T("nick"), nick.str,
		_T("status"), msg->status.str,
		_T("text"), msg->text.str,
		_T("text_sl"), plain.str,
		_T("protocol"), protocol,
		_T("msg_date"), date, 
		_T("year"), year,
		_T("month"), month,
		_T("month_name"), month_name, 
		_T("day"), day,
		_T("session"), msg->name.str
	};

	int i;
	for(i = 0; i < MAX_REGS(CHAT_EVENTS); i++) 
		if (msg->type == CHAT_EVENTS[i].type)
			break;
	if (i < MAX_REGS(CHAT_EVENTS))
		Speak_SayExWT("groupchat", NULL, i, vars, MAX_REGS(vars));

	if (IsChatProtocolEnabled(msg->proto.str) && IsChatEventEnabled(msg->type))
	{
		Buffer<TCHAR> text;
		text.append(_T("["));
		text.append(date);
		text.append(_T("] "));

		switch (msg->type) 
		{
			case GC_EVENT_MESSAGE:
				text.append(_T("* "));
				text.append(nick);
				text.append(_T(": "));
				AppendKeepingIndentationRemovingFormatting(text, msg->text.str, opts.chat_ident_multiline_msgs);
				break;
			case GC_EVENT_ACTION:
				text.append(_T("* "));
				text.append(nick);
				text.append(_T(" "));
				AppendKeepingIndentationRemovingFormatting(text, msg->text.str, opts.chat_ident_multiline_msgs);
				break;
			case GC_EVENT_JOIN:
				text.append(_T("> "));
				text.appendPrintf(TranslateT("%s has joined"), nick.str);
				break;
			case GC_EVENT_PART:
				text.append(_T("< "));
				if (plain.len <= 0)
					text.appendPrintf(TranslateT("%s has left"), nick.str);
				else
					text.appendPrintf(TranslateT("%s has left (%s)"), nick.str, plain.str);
				break;
			case GC_EVENT_QUIT:
				text.append(_T("< "));
				if (plain.len <= 0)
					text.appendPrintf(TranslateT("%s has disconnected"), nick.str);
				else
					text.appendPrintf(TranslateT("%s has disconnected (%s)"), nick.str, plain.str);
				break;
			case GC_EVENT_NICK:
				text.append(_T("^ "));
				text.appendPrintf(TranslateT("%s is now known as %s"), nick.str, msg->text.str);
				break;
			case GC_EVENT_KICK:
				text.append(_T("~ "));
				if (plain.len <= 0)
					text.appendPrintf(TranslateT("%s kicked %s"), msg->status.str, nick.str);
				else
					text.appendPrintf(TranslateT("%s kicked %s (%s)"), msg->status.str, nick.str, plain.str);
				break;
			case GC_EVENT_NOTICE:
				text.append(_T("¤ "));
				text.appendPrintf(TranslateT("Notice from %s: %s"), nick.str, plain.str);
				break;
			case GC_EVENT_TOPIC:
				text.append(_T("# "));
				if (nick.len <= 0)
					text.appendPrintf(TranslateT("The topic is \'%s\'"), plain.str);
				else
					text.appendPrintf(TranslateT("The topic is \'%s\' (set by %s)"), plain.str, nick.str);
				break;
			case GC_EVENT_INFORMATION:
				text.append(_T("! "));
				text.append(plain);
				break;
			case GC_EVENT_ADDSTATUS:
				text.append(_T("+ "));
				text.appendPrintf(TranslateT("%s enables \'%s\' status for %s"), msg->text.str, msg->status.str, nick.str);
				break;
			case GC_EVENT_REMOVESTATUS:
				text.append(_T("- "));
				text.appendPrintf(TranslateT("%s disables \'%s\' status for %s"), msg->text.str, msg->status.str, nick.str);
				break;
		}

		AppendToFile(opts.chat_filename_pattern, vars, MAX_REGS(vars), NULL, text.str);

	}

	delete msg;
}


int ChatGetEventPtr(WPARAM wParam, LPARAM lParam) 
{ 
	if (originalChatGetEventPtrService == NULL)
		return -1;

	int ret = originalChatGetEventPtrService(wParam, lParam);
	if (ret != 0 || lParam == NULL || shutingDown)
		return ret;

	GCPTRS * gp = (GCPTRS *) lParam;
	originalAddEvent = gp->pfnAddEvent;
	gp->pfnAddEvent = &ChatAddEvent;

	return ret; 
}


HANDLE HackCreateServiceFunction(const char *name, MIRANDASERVICE service)
{
	if (name == NULL || service == NULL)
		return originalCreateServiceFunction(name, service);

	if (strcmp(name, MS_GC_GETEVENTPTR) == 0)
	{
		if (originalChatGetEventPtrService == NULL)
			originalChatGetEventPtrService = service;

		return originalCreateServiceFunction(name, &ChatGetEventPtr);
	}
	else if (strcmp(name, MS_GC_EVENT) == 0)
	{
		if (originalChatAddEventService == NULL)
			originalChatAddEventService = service;

		return originalCreateServiceFunction(name, &ChatAddEvent);
	}

	return originalCreateServiceFunction(name, service);
}


void InitChat()
{
	originalCreateServiceFunction = pluginLink->CreateServiceFunction;
	pluginLink->CreateServiceFunction = HackCreateServiceFunction;
}


void FreeChat()
{
	if (originalCreateServiceFunction)
		pluginLink->CreateServiceFunction = originalCreateServiceFunction;
}
