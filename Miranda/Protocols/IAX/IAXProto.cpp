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


static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static int static_iaxc_callback(iaxc_event e, void *param);


IAXProto::IAXProto(const char *aProtoName, const TCHAR *aUserName)
{
	InitializeCriticalSection(&cs);

	reg_id = 0;
	m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;

	m_tszUserName = mir_tstrdup(aUserName);
	m_szProtoName = mir_strdup(aProtoName);
	m_szModuleName = mir_strdup(aProtoName);

	TCHAR buffer[MAX_PATH]; 
	mir_sntprintf(buffer, MAX_REGS(buffer), TranslateT("%s plugin connections"), m_tszUserName);
		
	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.flags = NUF_OUTGOING | NUF_TCHAR;
	nl_user.szSettingsModule = m_szModuleName;
	nl_user.ptszDescriptiveName = buffer;
	hNetlibUser = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);

	CreateProtoService(PS_CREATEACCMGRUI, &IAXProto::CreateAccMgrUI);

	static OptPageControl ctrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_HOST,			"Host", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_PORT,			"Port", 4569, 0, 1 },
		{ NULL,	CONTROL_TEXT,		IDC_USERNAME,		"Username", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
	};

	memmove(accountManagerCtrls, ctrls, sizeof(accountManagerCtrls));
	accountManagerCtrls[0].var = &opts.host;
	accountManagerCtrls[1].var = &opts.port;
	accountManagerCtrls[2].var = &opts.username;
	accountManagerCtrls[3].var = &opts.password;
	accountManagerCtrls[4].var = &opts.savePassword;

	LoadOpts(accountManagerCtrls, MAX_REGS(accountManagerCtrls), m_szModuleName);

	iaxc_set_event_callback(&static_iaxc_callback, this); 
	
	// TODO Handle network out port
	iaxc_set_preferred_source_udp_port(-1);
	if (iaxc_initialize(3))
		throw "!!";

	iaxc_set_formats(IAXC_FORMAT_SPEEX,
					 IAXC_FORMAT_ULAW|IAXC_FORMAT_ALAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX|IAXC_FORMAT_ILBC);
	iaxc_set_speex_settings(1,-1,-1,0,8000,3);
	iaxc_set_silence_threshold(-99);

	if (iaxc_start_processing_thread())
		throw "!!";
}


IAXProto::~IAXProto()
{
	iaxc_stop_processing_thread();

	iaxc_shutdown();

	Netlib_CloseHandle(hNetlibUser);

	mir_free(m_tszUserName);
	mir_free(m_szProtoName);
	mir_free(m_szModuleName);

	DeleteCriticalSection(&cs);
}


DWORD_PTR __cdecl IAXProto::GetCaps( int type, HANDLE hContact ) 
{
	switch(type) 
	{
		case PFLAGNUM_1:
			return 0;

		case PFLAGNUM_2:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND 
					 | PF2_FREECHAT | PF2_OUTTOLUNCH | PF2_ONTHEPHONE | PF2_INVISIBLE | PF2_IDLE;

		case PFLAGNUM_3:
			return 0;

		case PFLAGNUM_4:
			return PF4_NOCUSTOMAUTH;

		case PFLAG_UNIQUEIDTEXT:
			return NULL;

		case PFLAG_UNIQUEIDSETTING:
			return NULL;

		case PFLAG_MAXLENOFMESSAGE:
			return 100;
	}

	return 0;
}


int __cdecl IAXProto::SetStatus( int iNewStatus ) 
{
	if (m_iStatus == iNewStatus) 
		return 0;
	m_iDesiredStatus = iNewStatus;

	if (m_iDesiredStatus == ID_STATUS_OFFLINE)
	{
		Disconnect();
	}
	else if (m_iStatus == ID_STATUS_OFFLINE)
	{
		BroadcastStatus(ID_STATUS_CONNECTING);

		TCHAR server_port[1024];
		mir_sntprintf(server_port, MAX_REGS(server_port), _T("%s:%d"), opts.host, opts.port);

		reg_id = iaxc_register(TcharToUtf8(opts.username), opts.password, TcharToUtf8(server_port));
		if (reg_id <= 0)
		{
			BroadcastStatus(ID_STATUS_OFFLINE);
			return -1;
		}
	}
	else if (m_iStatus > ID_STATUS_OFFLINE)
	{
		BroadcastStatus(m_iDesiredStatus);
	}

	return 0; 
}


INT_PTR  __cdecl IAXProto::CreateAccMgrUI(WPARAM wParam, LPARAM lParam)
{
	return (INT_PTR) CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ACCMGRUI), 
										(HWND) lParam, DlgProcAccMgrUI, (LPARAM)this);
}


void IAXProto::BroadcastStatus(int newStatus)
{
	int oldStatus = m_iStatus;
	m_iStatus = newStatus;
	SendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
}


void IAXProto::CreateProtoService(const char* szService, IAXServiceFunc serviceProc)
{
	char str[MAXMODULELABELLENGTH];

	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
}


void IAXProto::CreateProtoService(const char* szService, IAXServiceFuncParam serviceProc, LPARAM lParam)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObjParam(str, (MIRANDASERVICEOBJPARAM)*(void**)&serviceProc, this, lParam);
}


HANDLE IAXProto::CreateProtoEvent(const char* szService)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	return ::CreateHookableEvent(str);
}


int IAXProto::SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
{
	ACKDATA ack = {0};
	ack.cbSize = sizeof(ACKDATA);
	ack.szModule = m_szModuleName;
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;
	return CallService(MS_PROTO_BROADCASTACK, 0, (LPARAM)&ack);
}


void IAXProto::ShowMessage(bool error, TCHAR *fmt, ...) 
{
	va_list args;
	va_start(args, fmt);

	TCHAR buff[1024];
	mir_sntprintf(buff, MAX_REGS(buff), fmt, args);

	OutputDebugString(buff);
	CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) (const char *) TcharToChar(buff));

	va_end(args);
}


void IAXProto::Disconnect()
{
	iaxc_dump_all_calls();

	if (reg_id > 0)
	{
		iaxc_unregister(reg_id);
		reg_id = 0;
	}

	BroadcastStatus(ID_STATUS_OFFLINE);
}


int IAXProto::levels_callback(iaxc_ev_levels &levels)
{
	return 0;
}


int IAXProto::text_callback(iaxc_ev_text &text)
{
	switch(text.type) 
	{
		case IAXC_TEXT_TYPE_STATUS:
		{
			ShowMessage(false, TranslateT("Status: %s"), Utf8ToTchar(text.message));
			return 1;
		}
		case IAXC_TEXT_TYPE_NOTICE:
		{
			ShowMessage(false, TranslateT("Notice: %s"), Utf8ToTchar(text.message));
			return 1;
		}
		case IAXC_TEXT_TYPE_ERROR:
		{
			ShowMessage(true, Utf8ToTchar(text.message));
			return 1;
		}
		case IAXC_TEXT_TYPE_FATALERROR:
		{
			ShowMessage(true, TranslateT("Fatal: %s"), Utf8ToTchar(text.message));
			Disconnect();
			return 1;
		}
		case IAXC_TEXT_TYPE_IAX:
		{
			ShowMessage(false, TranslateT("IAX: %s"), Utf8ToTchar(text.message));
			return 1;
		}
		default:
			return 0;
	}

	return 0;
}


int IAXProto::state_callback(iaxc_ev_call_state &call)
{
	return 0;
}


int IAXProto::netstats_callback(iaxc_ev_netstats &netstats)
{
	return 0;
}


int IAXProto::url_callback(iaxc_ev_url &url)
{
	return 0;
}


int IAXProto::registration_callback(iaxc_ev_registration &reg)
{
	if (reg.id != reg_id)
		return 0;

	switch(reg.reply)
	{
		case IAXC_REGISTRATION_REPLY_ACK:
		{
			BroadcastStatus(m_iDesiredStatus > ID_STATUS_OFFLINE ? m_iDesiredStatus : ID_STATUS_ONLINE);

			if (reg.msgcount > 0) 
				ShowMessage(false, TranslateT("You have %d voicemail message(s)."), reg.msgcount);

			return 1;
		}
		case IAXC_REGISTRATION_REPLY_REJ:
		{
			ShowMessage(true, TranslateT("Registration rejected"));
			Disconnect();

			return 1;
		}
		case IAXC_REGISTRATION_REPLY_TIMEOUT:
		{
			ShowMessage(true, TranslateT("Registration timeout"));
			Disconnect();

			return 1;
		}
		default:
			return 0;
	}
}


int IAXProto::iaxc_callback(iaxc_event &e)
{
	switch(e.type) 
	{
		case IAXC_EVENT_LEVELS:			return levels_callback(e.ev.levels);
		case IAXC_EVENT_TEXT:			return text_callback(e.ev.text);
		case IAXC_EVENT_STATE:			return state_callback(e.ev.call);
		case IAXC_EVENT_NETSTAT:		return netstats_callback(e.ev.netstats);
		case IAXC_EVENT_URL:			return url_callback(e.ev.url);
		case IAXC_EVENT_REGISTRATION:	return registration_callback(e.ev.reg);
		default:						return 0;  // not handled
	}
}


static void CALLBACK ProcessIAXEvents(void *param)
{
	for(int i = 0; i < instances.getCount(); ++i)
	{
		IAXProto *proto = &instances[i];

		EnterCriticalSection(&proto->cs);

		for(std::vector<iaxc_event>::iterator it = proto->events.begin(); it != proto->events.end(); ++it)
			proto->iaxc_callback(*it);
		
		proto->events.clear();

		LeaveCriticalSection(&proto->cs);
	}
}


static int static_iaxc_callback(iaxc_event e, void *param)
{
	IAXProto *proto = (IAXProto *) param;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(e);
	LeaveCriticalSection(&proto->cs);

	CallFunctionAsync(&ProcessIAXEvents, NULL);

	return 0;
}



static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	IAXProto *proto = (IAXProto *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg) 
	{
		case WM_INITDIALOG: 
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			proto = (IAXProto *) lParam;
			break;
		}

		case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->code == (UINT)PSN_APPLY) 
			{
			}
			break;
		}
	}

	if (proto == NULL)
		return FALSE;

	return SaveOptsDlgProc(proto->accountManagerCtrls, MAX_REGS(proto->accountManagerCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);
}

