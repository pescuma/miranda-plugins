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
static INT_PTR CALLBACK DlgOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


class SipToTchar
{
public:
	SipToTchar(const pj_str_t &str) : tchar(NULL)
	{
		if (str.ptr == NULL)
			return;

		int size = MultiByteToWideChar(CP_UTF8, 0, str.ptr, str.slen, NULL, 0);
		if (size < 0)
			throw _T("Could not convert string to WCHAR");
		size++;

		WCHAR *tmp = (WCHAR *) mir_alloc(size * sizeof(WCHAR));
		if (tmp == NULL)
			throw _T("mir_alloc returned NULL");

		MultiByteToWideChar(CP_UTF8, 0, str.ptr, str.slen, tmp, size);
		tmp[size-1] = 0;

#ifdef UNICODE

		tchar = tmp;

#else

		size = WideCharToMultiByte(CP_ACP, 0, tmp, -1, NULL, 0, NULL, NULL);
		if (size <= 0)
		{
			mir_free(tmp);
			throw _T("Could not convert string to ACP");
		}

		tchar = (TCHAR *) mir_alloc(size * sizeof(char));
		if (tchar == NULL)
		{
			mir_free(tmp);
			throw _T("mir_alloc returned NULL");
		}

		WideCharToMultiByte(CP_ACP, 0, tmp, -1, tchar, size, NULL, NULL);

		mir_free(tmp);

#endif
	}

	~SipToTchar()
	{
		if (tchar != NULL)
			mir_free(tchar);
	}

	TCHAR *detach()
	{
		TCHAR *ret = tchar;
		tchar = NULL;
		return ret;
	}

	TCHAR * get() const
	{
		return tchar;
	}

	operator TCHAR *() const
	{
		return tchar;
	}

	TCHAR operator[](int pos) const
	{
		return tchar[pos];
	}

	TCHAR & operator[](int pos)
	{
		return tchar[pos];
	}

private:
	TCHAR *tchar;
};


SIPProto::SIPProto(const char *aProtoName, const TCHAR *aUserName)
{
	//hasToDestroy = false;
	transport_id = -1;
	acc_id = -1;
	hNetlibUser = 0;
	hCallStateEvent = 0;
	m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;

	m_tszUserName = mir_tstrdup(aUserName);
	m_szProtoName = mir_strdup(aProtoName);
	m_szModuleName = mir_strdup(aProtoName);

	static OptPageControl amCtrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_HOST,			"Host", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_PORT,			"Port", 5060, 0, 1 },
		{ NULL,	CONTROL_TEXT,		IDC_USERNAME,		"Username", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, IDC_SAVEPASSWORD, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
	};

	memmove(accountManagerCtrls, amCtrls, sizeof(accountManagerCtrls));
	accountManagerCtrls[0].var = &opts.host;
	accountManagerCtrls[1].var = &opts.port;
	accountManagerCtrls[2].var = &opts.username;
	accountManagerCtrls[3].var = &opts.password;
	accountManagerCtrls[4].var = &opts.savePassword;

	static OptPageControl oCtrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_HOST,			"Host", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_PORT,			"Port", 5060, 0, 1 },
		{ NULL,	CONTROL_TEXT,		IDC_USERNAME,		"Username", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, IDC_SAVEPASSWORD, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
		{ NULL,	CONTROL_TEXT,		IDC_STUN_HOST,		"StunHost", _T("stun01.sipphone.com"), 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_STUN_PORT,		"StunPort", 0, 0, 1 },
	};

	memmove(optionsCtrls, oCtrls, sizeof(optionsCtrls));
	optionsCtrls[0].var = &opts.host;
	optionsCtrls[1].var = &opts.port;
	optionsCtrls[2].var = &opts.username;
	optionsCtrls[3].var = &opts.password;
	optionsCtrls[4].var = &opts.savePassword;
	optionsCtrls[5].var = &opts.stun.host;
	optionsCtrls[6].var = &opts.stun.port;

	LoadOpts(optionsCtrls, MAX_REGS(optionsCtrls), m_szModuleName);

	CreateProtoService(PS_CREATEACCMGRUI, &SIPProto::CreateAccMgrUI);

	CreateProtoService(PS_VOICE_CAPS, &SIPProto::VoiceCaps);
	CreateProtoService(PS_VOICE_CALL, &SIPProto::VoiceCall);
	CreateProtoService(PS_VOICE_ANSWERCALL, &SIPProto::VoiceAnswerCall);
	CreateProtoService(PS_VOICE_DROPCALL, &SIPProto::VoiceDropCall);
	CreateProtoService(PS_VOICE_HOLDCALL, &SIPProto::VoiceHoldCall);
	CreateProtoService(PS_VOICE_SEND_DTMF, &SIPProto::VoiceSendDTMF);
	CreateProtoService(PS_VOICE_CALL_STRING_VALID, &SIPProto::VoiceCallStringValid);

	hCallStateEvent = CreateProtoEvent(PE_VOICE_CALL_STATE);

	HookProtoEvent(ME_OPT_INITIALISE, &SIPProto::OnOptionsInit);
}


SIPProto::~SIPProto()
{
	Netlib_CloseHandle(hNetlibUser);

	mir_free(m_tszUserName);
	mir_free(m_szProtoName);
	mir_free(m_szModuleName);
}


DWORD_PTR __cdecl SIPProto::GetCaps( int type, HANDLE hContact ) 
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
			return (UINT_PTR) Translate("User");

		case PFLAG_UNIQUEIDSETTING:
			return (UINT_PTR) "Username";

		case PFLAG_MAXLENOFMESSAGE:
			return 100;
	}

	return 0;
}


int SIPProto::Connect()
{
	Trace(_T("Connecting..."));

	BroadcastStatus(ID_STATUS_CONNECTING);
/*
	{
		pj_status_t status = pjsua_create();
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error creating pjsua!"));
			return -1;
		}
		hasToDestroy = true;
	}

	{
		pjsua_config cfg;
		pjsua_config_default(&cfg);
		cfg.cb.on_incoming_call = &static_on_incoming_call;
		cfg.cb.on_call_media_state = &static_on_call_media_state;
		cfg.cb.on_call_state = &static_on_call_state;
		cfg.cb.on_reg_state = &static_on_reg_state;

		pjsua_logging_config log_cfg;
		pjsua_logging_config_default(&log_cfg);
		log_cfg.console_level = 4;
		log_cfg.cb = &static_on_log;

		pj_status_t status = pjsua_init(&cfg, &log_cfg, NULL);
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error initializing pjsua"));
			Disconnect();
			return 1;
		}
	}
*/
	{
		pjsua_transport_config cfg;
		pjsua_transport_config_default(&cfg);
		pj_status_t status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_id);
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error creating transport"));
			Disconnect();
			return 2;
		}
	}
/*
	{
		pj_status_t status = pjsua_start();
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error starting pjsua"));
			Disconnect();
			return 3;
		}
	}
*/
	{
		TCHAR tmp[1024];

		pjsua_acc_config cfg;
		pjsua_acc_config_default(&cfg);
		cfg.user_data = this;
		cfg.transport_id = transport_id;

		mir_sntprintf(tmp, MAX_REGS(tmp), _T("sip:%s@%s"), opts.username, opts.host);
		TcharToUtf8 id(tmp);
		cfg.id = pj_str(id);

		mir_sntprintf(tmp, MAX_REGS(tmp), _T("sip:%s:%d"), opts.host, opts.port);
		TcharToUtf8 reg_uri(tmp);
		cfg.reg_uri = pj_str(reg_uri);

		cfg.publish_enabled = PJ_TRUE;

		cfg.cred_count = 1;
		TcharToUtf8 host(opts.host);
		cfg.cred_info[0].realm = pj_str(host);
		cfg.cred_info[0].scheme = pj_str("digest");
		TcharToUtf8 username(opts.username);
		cfg.cred_info[0].username = pj_str(username);
		cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD; // TODO
		cfg.cred_info[0].data = pj_str(opts.password);

		pj_status_t status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error adding account"));
			Disconnect();
			return 4;
		}
	}

	return 0;
}


int __cdecl SIPProto::SetStatus( int iNewStatus ) 
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
		return Connect();
	}
	else if (m_iStatus > ID_STATUS_OFFLINE)
	{
		SendPresence(m_iDesiredStatus);
		BroadcastStatus(m_iDesiredStatus);
	}

	return 0; 
}


INT_PTR  __cdecl SIPProto::CreateAccMgrUI(WPARAM wParam, LPARAM lParam)
{
	return (INT_PTR) CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ACCMGRUI), 
										(HWND) lParam, DlgProcAccMgrUI, (LPARAM)this);
}


void SIPProto::BroadcastStatus(int newStatus)
{
	Trace(_T("BroadcastStatus %d"), newStatus);

	int oldStatus = m_iStatus;
	m_iStatus = newStatus;
	SendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
}


void SIPProto::CreateProtoService(const char* szService, SIPServiceFunc serviceProc)
{
	char str[MAXMODULELABELLENGTH];

	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
}


void SIPProto::CreateProtoService(const char* szService, SIPServiceFuncParam serviceProc, LPARAM lParam)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObjParam(str, (MIRANDASERVICEOBJPARAM)*(void**)&serviceProc, this, lParam);
}


HANDLE SIPProto::CreateProtoEvent(const char* szService)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	return ::CreateHookableEvent(str);
}


void SIPProto::HookProtoEvent(const char* szEvent, SIPEventFunc pFunc)
{
	::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*(void**)&pFunc, this);
}



int SIPProto::SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
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


#define MESSAGE_TYPE_TRACE 0
#define MESSAGE_TYPE_INFO 1
#define MESSAGE_TYPE_ERROR 2


void SIPProto::Trace(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_TRACE, fmt, args);

	va_end(args);
}


void SIPProto::Info(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_INFO, TranslateTS(fmt), args);

	va_end(args);
}


void SIPProto::Error(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_ERROR, TranslateTS(fmt), args);

	va_end(args);
}


void SIPProto::Error(pj_status_t status, TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	TCHAR buff[1024];
	mir_vsntprintf(buff, MAX_REGS(buff), TranslateTS(fmt), args);

	va_end(args);

    char errmsg[PJ_ERR_MSG_SIZE];
    pj_strerror(status, errmsg, sizeof(errmsg));

	Error(_T("%s: %s"), buff, Utf8ToTchar(errmsg));
}


void SIPProto::ShowMessage(int type, TCHAR *fmt, va_list args) 
{
	TCHAR buff[1024];
	if (type == MESSAGE_TYPE_TRACE)
		lstrcpy(buff, _T("TRACE: "));
	else if (type == MESSAGE_TYPE_INFO)
		lstrcpy(buff, _T("INFO : "));
	else
		lstrcpy(buff, _T("ERROR: "));

	mir_vsntprintf(&buff[7], MAX_REGS(buff)-7, fmt, args);

	OutputDebugString(buff);
	OutputDebugString(_T("\n"));
	CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) TcharToChar(buff).get());
}


void SIPProto::Disconnect()
{
	Trace(_T("Disconnecting..."));

	if (acc_id >= 0)
	{
		SendPresence(ID_STATUS_OFFLINE);
		pjsua_acc_del(acc_id);
		acc_id = -1;
	}

	if (transport_id >= 0)
	{
		pjsua_transport_close(transport_id, PJ_FALSE);
		transport_id = -1;
	}
/*
	if (hasToDestroy)
	{
		pjsua_destroy();
		hasToDestroy = false;
	}
*/
	BroadcastStatus(ID_STATUS_OFFLINE);
}


int __cdecl SIPProto::OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam ) 
{
	switch(iEventType) 
	{
		case EV_PROTO_ONLOAD:    
			return OnModulesLoaded(0, 0);

		case EV_PROTO_ONEXIT:    
			return OnPreShutdown(0, 0);

		case EV_PROTO_ONOPTIONS: 
			return OnOptionsInit(wParam, lParam);

		case EV_PROTO_ONRENAME:
		{	
			break;
		}
	}
	return 1;
}


int __cdecl SIPProto::OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	TCHAR buffer[MAX_PATH]; 
	mir_sntprintf(buffer, MAX_REGS(buffer), TranslateT("%s plugin connections"), m_tszUserName);
		
	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.flags = /* NUF_INCOMING | NUF_OUTGOING | */ NUF_NOOPTIONS | NUF_TCHAR;
	nl_user.szSettingsModule = m_szModuleName;
	nl_user.ptszDescriptiveName = buffer;
	hNetlibUser = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);

	if (!ServiceExists(MS_VOICESERVICE_REGISTER))
	{
		Error(_T("SIP needs Voice Service plugin to work!"));
		return 1;
	}

	return 0;
}


int __cdecl SIPProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.ptszGroup = LPGENT("Network");
	odp.ptszTitle = m_tszUserName;
	odp.pfnDlgProc = DlgOptions;
	odp.dwInitParam = (LPARAM) this;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);

	return 0;
}


int __cdecl SIPProto::OnPreShutdown(WPARAM wParam, LPARAM lParam)
{
	return 0;
}


void SIPProto::NotifyCall(pjsua_call_id call_id, int state, HANDLE hContact, TCHAR *name, TCHAR *number)
{
	Trace(_T("NotifyCall %d -> %d"), call_id, state);

	char tmp[16];

	VOICE_CALL vc = {0};
	vc.cbSize = sizeof(vc);
	vc.szModule = m_szModuleName;
	vc.id = itoa((int) call_id, tmp, 10);
	vc.flags = VOICE_TCHAR;
	vc.hContact = hContact;
	vc.ptszName = name;
	vc.ptszNumber = number;
	vc.state = state;

	NotifyEventHooks(hCallStateEvent, (WPARAM) &vc, 0);
}


bool SIPProto::SendPresence(int proto_status)
{
	pjsua_acc_info info;
	pj_status_t status = pjsua_acc_get_info(acc_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return false;
	}

	if (info.status != PJSIP_SC_OK)
		return false;
	
	pjrpid_element elem;
	pj_bzero(&elem, sizeof(elem));
	elem.type = PJRPID_ELEMENT_TYPE_PERSON;

	switch(proto_status)
	{
		case ID_STATUS_AWAY:
			elem.activity = PJRPID_ACTIVITY_AWAY;
			elem.note = pj_str("Away");
			break;
		case ID_STATUS_NA:
			elem.activity = PJRPID_ACTIVITY_AWAY;
			elem.note = pj_str("Not avaible");
			break;
		case ID_STATUS_DND:
			elem.activity = PJRPID_ACTIVITY_BUSY;
			elem.note = pj_str("Do not disturb");
			break;
		case ID_STATUS_OCCUPIED:
			elem.activity = PJRPID_ACTIVITY_BUSY;
			elem.note = pj_str("Occupied");
			break;
		case ID_STATUS_FREECHAT:
			elem.activity = PJRPID_ACTIVITY_UNKNOWN;
			elem.note = pj_str("Free for chat");
			break;
		case ID_STATUS_ONTHEPHONE:
			elem.activity = PJRPID_ACTIVITY_BUSY;
			elem.note = pj_str("On the phone");
			break;
		case ID_STATUS_OUTTOLUNCH:
			elem.activity = PJRPID_ACTIVITY_AWAY;
			elem.note = pj_str("Out to lunch");
			break;
		default:
			break;
	}

	pj_bool_t online = (proto_status == ID_STATUS_INVISIBLE ? PJ_FALSE : PJ_TRUE);

	if (online == info.online_status && pj_strcmp(&info.online_status_text, &elem.note) == 0)
		return false;
	
	pjsua_acc_set_online_status2(acc_id, online, &elem);
	return true;
}


void CALLBACK SIPProto::DisconnectProto(void *param)
{
	SIPProto *proto = (SIPProto *) param;
	proto->Disconnect();
}


void SIPProto::on_reg_state()
{
	Trace(_T("on_reg_state"));

	pjsua_acc_info info;
	pj_status_t status = pjsua_acc_get_info(acc_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return;
	}

	if (info.status == PJSIP_SC_OK)
	{
		int status = (m_iDesiredStatus > ID_STATUS_OFFLINE ? m_iDesiredStatus : ID_STATUS_ONLINE);
		SendPresence(status);
		BroadcastStatus(status);
	}
	else
	{
		CallFunctionAsync(&SIPProto::DisconnectProto, this);
	}
}

static void RemoveLtGt(TCHAR *str)
{
	int len = lstrlen(str);
	if (str[0] == _T('<') && str[len-1] == _T('>'))
	{
		str[len-1] = 0;
		memmove(str, &str[1], len * sizeof(TCHAR));
	}
}

void SIPProto::on_incoming_call(pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	Trace(_T("on_incoming_call: %d"), call_id);

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return;
	}

	TCHAR numberBuff[1024];
	lstrcpyn(numberBuff, SipToTchar(info.remote_contact), MAX_REGS(numberBuff));
	RemoveLtGt(numberBuff);

	TCHAR name[256];
	name[0] = 0;
	SipToTchar remote_info(info.remote_info);
	if (remote_info[0] == _T('"'))
	{
		TCHAR *other = _tcsstr(&remote_info[1], _T("\" <"));
		if (other != NULL)
		{
			lstrcpyn(name, &remote_info[1], min(MAX_REGS(name), other - remote_info));
			if (IsEmpty(numberBuff))
			{
				lstrcpyn(numberBuff, other+2, MAX_REGS(numberBuff));
				RemoveLtGt(numberBuff);
			}
		}
	}

	if (IsEmpty(name) && IsEmpty(numberBuff))
		lstrcpyn(numberBuff, remote_info, MAX_REGS(numberBuff));

	TCHAR *number = numberBuff;

	if (_tcsnicmp(_T("sip:"), number, 4) == 0)
	{
		number += 4;

		TCHAR *host = _tcschr(number, _T('@'));
		if (host != NULL && _tcsicmp(opts.host, host+1) == 0)
			*host = 0;
	}

	NotifyCall(call_id, VOICE_STATE_RINGING, NULL, name, number);
}


void SIPProto::on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	Trace(_T("on_call_state: %d"), call_id);

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return;
	}

	Trace(_T("Call info: %d / last: %d %s"), info.state, info.last_status, info.last_status_text);

	switch(info.state)
	{
		case PJSIP_INV_STATE_NULL:
		case PJSIP_INV_STATE_DISCONNECTED:
		{
			NotifyCall(call_id, VOICE_STATE_ENDED);
			break;
		}
		case PJSIP_INV_STATE_CONFIRMED:
		{
			NotifyCall(call_id, VOICE_STATE_TALKING);
			break;
		}
	}

}


void SIPProto::on_call_media_state(pjsua_call_id call_id)
{
	Trace(_T("on_call_media_state: %d"), call_id);

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return;
	}

	// Connect ports appropriately when media status is ACTIVE or REMOTE HOLD,
	// otherwise we should NOT connect the ports.
	if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE || info.media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
	{
		ConfigureDevices();

		pjsua_conf_connect(info.conf_slot, 0);
		pjsua_conf_connect(0, info.conf_slot);
	}

	if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE)
	{
		NotifyCall(call_id, VOICE_STATE_TALKING);
	}
	else if (info.media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD)
	{
		NotifyCall(call_id, VOICE_STATE_ON_HOLD);
	}
}


static int GetDevice(bool out, int defaultVal)
{
	DBVARIANT dbv;
	if (DBGetContactSettingString(NULL, "VoiceService", out ? "Output" : "Input", &dbv))
		return defaultVal;

	int ret = defaultVal;

    unsigned count = pjmedia_aud_dev_count();
    for (unsigned i = 0; i < count; ++i) 
	{
		pjmedia_aud_dev_info info;

		pj_status_t status = pjmedia_aud_dev_get_info(i, &info);
		if (status != PJ_SUCCESS)
			continue;

		if (!out && info.input_count < 1)
			continue;
		if (out && info.output_count < 1)
			continue;

		if (strcmp(dbv.pszVal, info.name) == 0)
		{
			ret = i;
			break;
		}
	}

	DBFreeVariant(&dbv);
	return ret;
}


void SIPProto::ConfigureDevices()
{
	int input, output;
	pjsua_get_snd_dev(&input, &output);

	int expectedInput = GetDevice(false, input);
	int expectedOutput = GetDevice(true, output);

	if (input != expectedInput || output != expectedOutput)
		pjsua_set_snd_dev(input, output);

	if (DBGetContactSettingByte(NULL, "VoiceService", "EchoCancelation", TRUE))
		pjsua_set_ec(PJSUA_DEFAULT_EC_TAIL_LEN, 0);
	else
		pjsua_set_ec(0, 0);
}


int __cdecl SIPProto::VoiceCaps(WPARAM wParam,LPARAM lParam)
{
	return VOICE_CAPS_VOICE | VOICE_CAPS_CALL_STRING;
}


static void CleanupNumber(TCHAR *out, int outSize, const TCHAR *number)
{
	int pos = 0;
	int len = lstrlen(number);
	for(int i = 0; i < len && pos < outSize - 1; ++i)
	{
		TCHAR c = number[i];

		if (i == 0 && c == _T('+'))
			out[pos++] = c;
		else if (c >= _T('0') && c <= _T('9'))
			out[pos++] = c;
	}
	out[pos] = 0;
}


void SIPProto::BuildURI(TCHAR *out, int outSize, const TCHAR *number, bool isTel)
{
	bool hasSip = (_tcsnicmp(_T("sip:"), number, 4) == 0);
	bool hasHost = (_tcschr(number, _T('@')) != NULL);

	if (isTel)
	{
		bool hasPhone = (_tcsstr(number, _T(";user=phone")) != NULL);

		if (!hasSip && !hasHost)
		{
			TCHAR tmp[1024];
			CleanupNumber(tmp, MAX_REGS(tmp), number);
			mir_sntprintf(out, outSize, _T("sip:%s@%s"), tmp, opts.host);
		}
		else if (!hasSip)
			mir_sntprintf(out, outSize, _T("sip:%s"), number);
		else
			mir_sntprintf(out, outSize, _T("%s"), number);

		 if (!hasPhone)
		 {
			 int len = lstrlen(out);
			 lstrcpyn(&out[len], _T(";user=phone"), outSize - len);
		 }
	}
	else
	{
		if (!hasSip && !hasHost)
			mir_sntprintf(out, outSize, _T("sip:%s@%s"), number, opts.host);
		else if (!hasSip)
			mir_sntprintf(out, outSize, _T("sip:%s"), number);
		else
			mir_sntprintf(out, outSize, _T("%s"), number);
	}
}


int __cdecl SIPProto::VoiceCall(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	TCHAR *number = (TCHAR *) lParam;

	TCHAR uri[1024];

	if (number != NULL)
	{
		if (!VoiceCallStringValid((WPARAM) number, 0))
			return 1;

		BuildURI(uri, MAX_REGS(uri), number, true);
	}
	else
	{
		// TODO
		return 2;
	}

	pjsua_call_id call_id;
	pj_status_t status = pjsua_call_make_call(acc_id, &pj_str(TcharToUtf8(uri)), 0, NULL, NULL, &call_id);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error making call"));
		return -1;
	}

	NotifyCall(call_id, VOICE_STATE_CALLING, hContact, NULL, number);

	return 0;
}


int __cdecl SIPProto::VoiceAnswerCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	pjsua_call_id call_id = (pjsua_call_id) atoi(id);
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return -1;
	}

	if (info.state == PJSIP_INV_STATE_INCOMING)
	{
		pj_status_t status = pjsua_call_answer(call_id, 200, NULL, NULL);
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error answering call"));
			return -1;
		}
	}
	else if (info.media_status == PJSUA_CALL_MEDIA_LOCAL_HOLD)
	{
		pj_status_t status = pjsua_call_reinvite(call_id, PJ_TRUE, NULL);
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error un-holding call"));
			return -1;
		}
	}
	else
	{
		// Wrong state 
		return 3;
	}

	return 0;
}


int __cdecl SIPProto::VoiceDropCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	pjsua_call_id call_id = (pjsua_call_id) atoi(id);
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	pj_status_t status = pjsua_call_hangup(call_id, 0, NULL, NULL);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error hanging up call"));
		return -1;
	}

	return 0;
}


int __cdecl SIPProto::VoiceHoldCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	pjsua_call_id call_id = (pjsua_call_id) atoi(id);
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	pj_status_t status = pjsua_call_set_hold(call_id, NULL);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error holding call"));
		return -1;
	}

	// NotifyCall(0, VOICE_STATE_ON_HOLD);

	return 0;
}


static bool IsValidDTMF(TCHAR c)
{
	if (c >= _T('A') && c <= _T('D'))
		return true;
	if (c >= _T('0') && c <= _T('9'))
		return true;
	if (c == _T('#') || c == _T('*'))
		return true;

	return false;
}


int __cdecl SIPProto::VoiceSendDTMF(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	TCHAR c = (TCHAR) lParam;
	if (id == NULL || id[0] == 0 || c == 0)
		return 1;

	pjsua_call_id call_id = (pjsua_call_id) atoi(id);
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	if (c >= _T('a') && c <= _T('d'))
		c += _T('A') - _T('a');

	if (!IsValidDTMF(c))
		return 3;

	TCHAR tmp[2];
	tmp[0] = c;
	tmp[1] = 0;
	pjsua_call_dial_dtmf(call_id, &pj_str(TcharToUtf8(tmp)));

	return 0;
}


int __cdecl SIPProto::VoiceCallStringValid(WPARAM wParam, LPARAM lParam)
{
	TCHAR *number = (TCHAR *) wParam;

	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;
	
	if (number == NULL || number[0] == 0)
		return 0;

	TCHAR tmp[1024];
	BuildURI(tmp, MAX_REGS(tmp), number, true);
	return pjsua_verify_sip_url(TcharToUtf8(tmp)) == PJ_SUCCESS;
}






static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SIPProto *proto = (SIPProto *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg) 
	{
		case WM_INITDIALOG: 
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			proto = (SIPProto *) lParam;
			break;
		}
	}

	if (proto == NULL)
		return FALSE;

	return SaveOptsDlgProc(proto->accountManagerCtrls, MAX_REGS(proto->accountManagerCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);
}


static INT_PTR CALLBACK DlgOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SIPProto *proto = (SIPProto *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg) 
	{
		case WM_INITDIALOG: 
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			proto = (SIPProto *) lParam;
			break;
		}
	}

	if (proto == NULL)
		return FALSE;

	return SaveOptsDlgProc(proto->optionsCtrls, MAX_REGS(proto->optionsCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);
}


