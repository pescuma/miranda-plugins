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

#define AUTH_STATE_AUTHORIZED 1
#define AUTH_STATE_DENIED 2

static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DlgOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


SIPProto::SIPProto(const char *aProtoName, const TCHAR *aUserName)
{
	hasToDestroy = false;
	udp_transport_id = -1;
	tcp_transport_id = -1;
	tls_transport_id = -1;
	acc_id = -1;
	hNetlibUser = 0;
	hCallStateEvent = 0;
	m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;
	messageID = 0;
	awayMessageID = 0;

	memset(awayMessages, 0, sizeof(awayMessages));

	InitializeCriticalSection(&cs);

	m_tszUserName = mir_tstrdup(aUserName);
	m_szProtoName = mir_strdup(aProtoName);
	m_szModuleName = mir_strdup(aProtoName);

	static OptPageControl amCtrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_REG_HOST,		"RegHost", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_REG_PORT,		"RegPort", 5060, 0, 0 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, IDC_SAVEPASSWORD, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
	};

	memmove(accountManagerCtrls, amCtrls, sizeof(accountManagerCtrls));
	accountManagerCtrls[0].var = &opts.registrar.host;
	accountManagerCtrls[1].var = &opts.registrar.port;
	accountManagerCtrls[2].var = &opts.password;
	accountManagerCtrls[3].var = &opts.savePassword;

	static OptPageControl oCtrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_USERNAME,		"Username", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_TEXT,		IDC_DOMAIN,			"Domain", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, IDC_SAVEPASSWORD, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
		{ NULL,	CONTROL_TEXT,		IDC_REG_HOST,		"RegHost", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_REG_PORT,		"RegPort", 5060, 0, 0 },
		{ NULL,	CONTROL_TEXT,		IDC_PROXY_HOST,		"ProxyHost", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_PROXY_PORT,		"ProxyPort", 5060, 0, 0 },
		{ NULL,	CONTROL_TEXT,		IDC_DNS_HOST,		"DNSHost", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_DNS_PORT,		"DNSPort", 53, 0, 0 },
		{ NULL,	CONTROL_TEXT,		IDC_STUN_HOST,		"STUNHost", (DWORD) _T("stun01.sipphone.com"), 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_STUN_PORT,		"STUNPort", PJ_STUN_PORT, 0, 0 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_PUBLISH,		"Publish", (BYTE) FALSE },
		{ NULL,	CONTROL_CHECKBOX,	IDC_KEEPALIVE,		"SendKeepAlive", (BYTE) TRUE },
	};

	memmove(optionsCtrls, oCtrls, sizeof(optionsCtrls));
	optionsCtrls[0].var = &opts.username;
	optionsCtrls[1].var = &opts.domain;
	optionsCtrls[2].var = &opts.password;
	optionsCtrls[3].var = &opts.savePassword;
	optionsCtrls[4].var = &opts.registrar.host;
	optionsCtrls[5].var = &opts.registrar.port;
	optionsCtrls[6].var = &opts.proxy.host;
	optionsCtrls[7].var = &opts.proxy.port;
	optionsCtrls[8].var = &opts.dns.host;
	optionsCtrls[9].var = &opts.dns.port;
	optionsCtrls[10].var = &opts.stun.host;
	optionsCtrls[11].var = &opts.stun.port;
	optionsCtrls[12].var = &opts.publish;
	optionsCtrls[13].var = &opts.sendKeepAlive;

	LoadOpts(optionsCtrls, MAX_REGS(optionsCtrls), m_szModuleName);

	for(HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		hContact != NULL; 
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) 
	{
		if (!IsMyContact(hContact))
			continue;
		
		DBDeleteContactSetting(hContact, m_szModuleName, "Status");
		DBDeleteContactSetting(hContact, m_szModuleName, "ID");
		DBDeleteContactSetting(hContact, "CList", "StatusMsg");
	}

	char setting[256];
	mir_snprintf(setting, MAX_REGS(setting), "%s/Status", m_szModuleName);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM) setting);

	mir_snprintf(setting, sizeof(setting), "%s/ID", m_szModuleName);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (LPARAM) setting);


	CreateProtoService(PS_CREATEACCMGRUI, &SIPProto::CreateAccMgrUI);

	CreateProtoService(PS_VOICE_CAPS, &SIPProto::VoiceCaps);
	CreateProtoService(PS_VOICE_CALL, &SIPProto::VoiceCall);
	CreateProtoService(PS_VOICE_ANSWERCALL, &SIPProto::VoiceAnswerCall);
	CreateProtoService(PS_VOICE_DROPCALL, &SIPProto::VoiceDropCall);
	CreateProtoService(PS_VOICE_HOLDCALL, &SIPProto::VoiceHoldCall);
	CreateProtoService(PS_VOICE_SEND_DTMF, &SIPProto::VoiceSendDTMF);
	CreateProtoService(PS_VOICE_CALL_STRING_VALID, &SIPProto::VoiceCallStringValid);

	CreateProtoService(PS_GETMYAWAYMSG, &SIPProto::GetMyAwayMsg);

	hCallStateEvent = CreateProtoEvent(PE_VOICE_CALL_STATE);

	HookProtoEvent(ME_OPT_INITIALISE, &SIPProto::OnOptionsInit);
	HookProtoEvent(ME_DB_CONTACT_DELETED, &SIPProto::OnContactDeleted);
}


SIPProto::~SIPProto()
{
	Netlib_CloseHandle(hNetlibUser);

	mir_free(m_tszUserName);
	mir_free(m_szProtoName);
	mir_free(m_szModuleName);

	DeleteCriticalSection(&cs);
}


bool SIPProto::IsMyContact(HANDLE hContact)
{
	char *proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	return proto != NULL && strcmp(m_szModuleName, proto) == 0;
}


DWORD_PTR __cdecl SIPProto::GetCaps( int type, HANDLE hContact ) 
{
	switch(type) 
	{
		case PFLAGNUM_1:
			return PF1_MODEMSG | PF1_IM | PF1_AUTHREQ | PF1_BASICSEARCH | PF1_ADDSEARCHRES;

		case PFLAGNUM_2:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_LIGHTDND | PF2_INVISIBLE;

		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND 
					 | PF2_FREECHAT | PF2_OUTTOLUNCH | PF2_ONTHEPHONE | PF2_INVISIBLE | PF2_IDLE;

		case PFLAGNUM_4:
			return PF4_SUPPORTTYPING | PF4_FORCEADDED | PF4_NOCUSTOMAUTH;

		case PFLAG_UNIQUEIDTEXT:
			return (UINT_PTR) Translate("Username");

		case PFLAG_UNIQUEIDSETTING:
			return (UINT_PTR) "Username";

		case PFLAG_MAXLENOFMESSAGE:
			return 100;
	}

	return 0;
}


static void CALLBACK ProcessEvents(void *param)
{
	for(int i = 0; i < instances.getCount(); ++i)
	{
		SIPProto *proto = &instances[i];

		EnterCriticalSection(&proto->cs);

		std::vector<SIPEvent> events(proto->events);
		proto->events.clear();

		LeaveCriticalSection(&proto->cs);

		if (proto->m_iStatus == ID_STATUS_OFFLINE)
			continue;

		for(unsigned int i = 0; i < events.size(); ++i)
		{
			SIPEvent &ev = events[i];
			switch(ev.type)
			{
				case SIPEvent::reg_state:
					proto->on_reg_state();
					break;
				case SIPEvent::incoming_call:
					proto->on_incoming_call(ev.call_id);
					break;
				case SIPEvent::call_state:
					proto->on_call_state(ev.call_id, ev.call_info);
					break;
				case SIPEvent::call_media_state:
					proto->on_call_media_state(ev.call_id);
					break;
				case SIPEvent::incoming_subscribe:
					proto->on_incoming_subscribe(ev.from, ev.text, ev.srv_pres);
					break;
				case SIPEvent::buddy_state:
					proto->on_buddy_state(ev.buddy_id);
					break;
				case SIPEvent::pager:
					proto->on_pager(ev.from, ev.text, ev.mime);
					break;
				case SIPEvent::pager_status:
					proto->on_pager_status(ev.messageData->hContact, ev.messageData->messageID, 
											ev.messageData->status, ev.text);
					break;
				case SIPEvent::typing:
					proto->on_typing(ev.from, ev.isTyping);
					break;
			}
			mir_free(ev.from);
			mir_free(ev.text);
			mir_free(ev.mime);
			delete ev.messageData;
		}
	}
}


static void static_on_reg_state(pjsua_acc_id acc_id)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::reg_state;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


static void static_on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
		return;

	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::incoming_call;
	ev.call_id = call_id;
	ev.call_info = info;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


static void static_on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
		return;

	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(info.acc_id);
	if (proto == NULL)
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::call_state;
	ev.call_id = call_id;
	ev.call_info = info;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


static void static_on_call_media_state(pjsua_call_id call_id)
{
	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
		return;

	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(info.acc_id);
	if (proto == NULL)
		return;

	if (!proto->on_call_media_state_sync(call_id, info))
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::call_media_state;
	ev.call_id = call_id;
	ev.call_info = info;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


// Handler for incoming presence subscription request
static void static_on_incoming_subscribe(pjsua_acc_id acc_id, pjsua_srv_pres *srv_pres,
										 pjsua_buddy_id buddy_id, const pj_str_t *from,
										 pjsip_rx_data *rdata, pjsip_status_code *code,
										 pj_str_t *reason, pjsua_msg_data *msg_data)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	if (!proto->on_incoming_subscribe_sync(srv_pres, buddy_id, from, rdata, code, reason, msg_data))
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::incoming_subscribe;
	ev.buddy_id = buddy_id;
	ev.from = mir_pjstrdup(from);
	ev.text = mir_pjstrdup(reason);
	ev.srv_pres = srv_pres;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


// Handler on buddy state changed.
static void static_on_buddy_state(pjsua_buddy_id buddy_id)
{
	pjsua_buddy_info info;
    pj_status_t status = pjsua_buddy_get_info(buddy_id, &info);
	if (status != PJ_SUCCESS)
		return;

	HANDLE hContact = (HANDLE) pjsua_buddy_get_user_data(buddy_id);
	if (hContact == NULL)
		return;

	SIPProto *proto = NULL;
	for(int i = 0; i < instances.getCount(); ++i)
	{
		SIPProto *candidate = &instances[i];
		if (candidate->IsMyContact(hContact))
		{
			proto = candidate;
			break;
		}
	}

	if (proto == NULL)
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::buddy_state;
	ev.buddy_id = buddy_id;

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


// Incoming IM message (i.e. MESSAGE request)!
static void static_on_pager(pjsua_call_id call_id, const pj_str_t *from, 
							const pj_str_t *to, const pj_str_t *contact,
							const pj_str_t *mime_type, const pj_str_t *text,
							pjsip_rx_data *rdata, pjsua_acc_id acc_id)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	if (!proto->on_pager_sync(call_id, from, to, contact, mime_type, text, rdata))
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::pager;
	ev.from = mir_pjstrdup(from);
	ev.text = mir_pjstrdup(text);
	ev.mime = mir_pjstrdup(mime_type);

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


void static_on_pager_status(pjsua_call_id call_id, const pj_str_t *to, const pj_str_t *body,
							 void *user_data, pjsip_status_code status, const pj_str_t *reason,
							 pjsip_tx_data *tdata, pjsip_rx_data *rdata, pjsua_acc_id acc_id)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::pager_status;
	ev.messageData = (MessageData *) user_data;
	ev.messageData->status = status;
	ev.text = mir_pjstrdup(reason);

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


// Received typing indication
static void static_on_typing(pjsua_call_id call_id, const pj_str_t *from,
							 const pj_str_t *to, const pj_str_t *contact,
							 pj_bool_t is_typing, pjsip_rx_data *rdata,
							 pjsua_acc_id acc_id)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	if (!proto->on_typing_sync(call_id, from, to, contact, is_typing, rdata))
		return;

	SIPEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SIPEvent::typing;
	ev.from = mir_pjstrdup(from);
	ev.isTyping = (is_typing != PJ_FALSE);

	EnterCriticalSection(&proto->cs);
	proto->events.push_back(ev);
	LeaveCriticalSection(&proto->cs);
	
	CallFunctionAsync(&ProcessEvents, NULL);
}


static void static_on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info)
{
}


static void static_on_log(int level, const char *data, int len)
{
	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "Level %d : %*s", level, len, data);

#ifdef _DEBUB
	OutputDebugStringA(tmp);
#endif

	if (instances.getCount() > 0)
		instances[0].Trace(SipToTchar(pj_str(tmp)));
}


int SIPProto::Connect()
{
	Trace(_T("Connecting..."));

	BroadcastStatus(ID_STATUS_CONNECTING);

	DestroySIP();

	{
		pj_status_t status = pjsua_create();
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error creating pjsua"));
			return -1;
		}
		hasToDestroy = true;
	}

	{
		scoped_mir_free<char> stun;
		scoped_mir_free<char> dns;

		pjsua_config cfg;
		pjsua_config_default(&cfg);
//#ifndef _DEBUG
//		cfg.use_srtp = PJMEDIA_SRTP_OPTIONAL;
//#endif
		cfg.cb.on_incoming_call = &static_on_incoming_call;
		cfg.cb.on_call_media_state = &static_on_call_media_state;
		cfg.cb.on_call_state = &static_on_call_state;
		cfg.cb.on_reg_state = &static_on_reg_state;
		cfg.cb.on_incoming_subscribe = &static_on_incoming_subscribe;
		cfg.cb.on_buddy_state = &static_on_buddy_state;
		cfg.cb.on_pager2 = &static_on_pager;
		cfg.cb.on_pager_status2 = &static_on_pager_status;
		cfg.cb.on_typing2 = &static_on_typing;
		cfg.cb.on_mwi_info = &static_on_mwi_info;

		char mir_ver[1024];
		CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM) sizeof(mir_ver), (LPARAM) mir_ver);

		TCHAR user_agent[1024];
		mir_sntprintf(user_agent, MAX_REGS(user_agent), _T("Miranda IM %s (pjsua v%s)/%s"), 
			CharToTchar(mir_ver).get(), CharToTchar(pj_get_version()).get(), _T(PJ_OS_NAME));
		TcharToSip ua(user_agent);
		cfg.user_agent = pj_str(ua);

		if (!IsEmpty(opts.stun.host))
		{
			TCHAR tmp[1024];
			mir_sntprintf(tmp, MAX_REGS(tmp), _T("%s:%d"), 
				CleanupSip(opts.stun.host), 
				FirstGtZero(opts.stun.port, PJ_STUN_PORT));
			stun = TcharToSip(tmp).detach();

			cfg.stun_srv_cnt = 1;
			cfg.stun_srv[0] = pj_str(stun);
		}

		if (!IsEmpty(opts.dns.host))
		{
			TCHAR tmp[1024];
			mir_sntprintf(tmp, MAX_REGS(tmp), _T("%s:%d"), 
				CleanupSip(opts.dns.host), 
				FirstGtZero(opts.dns.port, 53));
			dns = TcharToSip(tmp).detach();

			cfg.nameserver_count = 1;
			cfg.nameserver[0] = pj_str(dns);
		}

		pjsua_logging_config log_cfg;
		pjsua_logging_config_default(&log_cfg);
		log_cfg.cb = &static_on_log;

		pjsua_media_config media_cfg;
		pjsua_media_config_default(&media_cfg);
		media_cfg.enable_ice = PJ_TRUE;

		pj_status_t status = pjsua_init(&cfg, &log_cfg, &media_cfg);
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error initializing pjsua"));
			Disconnect();
			return 1;
		}
	}

	{
		pjsua_transport_config cfg;
		pjsua_transport_config_default(&cfg);
		pj_status_t status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &udp_transport_id);
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error creating UDP transport"));
			Disconnect();
			return 2;
		}
		
		status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &cfg, &tcp_transport_id);
		if (status != PJ_SUCCESS)
			Error(status, _T("Error creating TCP transport"));

		status = pjsua_transport_create(PJSIP_TRANSPORT_TLS, &cfg, &udp_transport_id);
		if (status != PJ_SUCCESS)
			Error(status, _T("Error creating TLS transport"));

		cfg.port = 4000;
		status = pjsua_media_transports_create(&cfg);
		if (status != PJ_SUCCESS)
			Error(status, _T("Error creating media transports"));
	}

	pjsua_get_snd_dev(&defaultInput, &defaultOutput);
	pjsua_set_null_snd_dev();

	{
		pj_status_t status = pjsua_start();
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error starting pjsua"));
			Disconnect();
			return 3;
		}
	}

	{
		scoped_mir_free<char> registrar;
		scoped_mir_free<char> proxy;
		TCHAR tmp[1024];

		pjsua_acc_config cfg;
		pjsua_acc_config_default(&cfg);
		cfg.user_data = this;
//		cfg.transport_id = transport_id;
//#ifndef _DEBUG
//		cfg.use_srtp = PJMEDIA_SRTP_OPTIONAL;
//#endif

		BuildURI(tmp, MAX_REGS(tmp), opts.username, opts.domain);
		TcharToSip id(tmp);
		cfg.id = pj_str(id);

		if (!IsEmpty(opts.registrar.host))
		{
			BuildURI(tmp, MAX_REGS(tmp), NULL, 
					 CleanupSip(opts.registrar.host), 
					 FirstGtZero(opts.registrar.port, 5060));
			registrar = TcharToSip(tmp).detach();

			cfg.reg_uri = pj_str(registrar);
		}

		if (!IsEmpty(opts.proxy.host))
		{
			BuildURI(tmp, MAX_REGS(tmp), NULL, 
					 CleanupSip(opts.proxy.host), 
					 FirstGtZero(opts.proxy.port, 5060));
			proxy = TcharToSip(tmp).detach();

			cfg.proxy_cnt = 1;
			cfg.proxy[0] = pj_str(proxy);
		}

		cfg.publish_enabled = (opts.publish ? PJ_TRUE : PJ_FALSE);
		cfg.mwi_enabled = PJ_TRUE;

		if (!opts.sendKeepAlive)
			cfg.ka_interval = 0;

		cfg.cred_count = 1;

		TcharToSip realm(CleanupSip(opts.domain));
		cfg.cred_info[0].realm = pj_str(realm);
		cfg.cred_info[0].scheme = pj_str("digest");
		TcharToSip username(opts.username);
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


void SIPProto::AddContactsToBuddyList()
{
	for(HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		hContact != NULL; 
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) 
	{
		if (!IsMyContact(hContact))
			continue;

		DBTString uri(hContact, m_szModuleName, "URI");
		if (uri == NULL)
		{
			Error(_T("Contact %d has no URI!"), hContact);
			continue;
		}

		TcharToSip sip_uri(uri);

		pjsua_buddy_config buddy_cfg;
		pjsua_buddy_config_default(&buddy_cfg);

		buddy_cfg.uri = pj_str(sip_uri);
		buddy_cfg.subscribe = PJ_FALSE;
		buddy_cfg.user_data = hContact;

		pjsua_buddy_id buddy_id;
		pj_status_t status = pjsua_buddy_add(&buddy_cfg, &buddy_id);
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error adding buddy '%s'"), uri);
			continue;
		}

		Attach(hContact, buddy_id);

		if (DBGetContactSettingByte(hContact, m_szModuleName, "WantAuth", 0))
			pjsua_buddy_subscribe_pres(buddy_id, PJ_TRUE);
	}
}


int SIPProto::ConvertStatus(int status)
{
	switch(status)
	{
		case ID_STATUS_ONLINE: return ID_STATUS_ONLINE;
		case ID_STATUS_AWAY: return ID_STATUS_AWAY;
		case ID_STATUS_DND: return ID_STATUS_OCCUPIED;
		case ID_STATUS_NA: return ID_STATUS_AWAY;
		case ID_STATUS_OCCUPIED: return ID_STATUS_OCCUPIED;
		case ID_STATUS_FREECHAT: return ID_STATUS_ONLINE;
		case ID_STATUS_INVISIBLE: return ID_STATUS_INVISIBLE;
		case ID_STATUS_ONTHEPHONE: return ID_STATUS_AWAY;
		case ID_STATUS_OUTTOLUNCH: return ID_STATUS_AWAY;
		default: return ID_STATUS_OFFLINE;
	}
}


int __cdecl SIPProto::SetStatus(int iNewStatus) 
{
	if (m_iDesiredStatus == iNewStatus) 
		return 0;
	m_iDesiredStatus = iNewStatus;

	if (m_iDesiredStatus == ID_STATUS_OFFLINE)
	{
		if (m_iStatus != ID_STATUS_OFFLINE)
			Disconnect();
	}
	else if (m_iStatus == ID_STATUS_OFFLINE)
	{
		return Connect();
	}
	else if (m_iStatus > ID_STATUS_OFFLINE)
	{
		BroadcastStatus(ConvertStatus(m_iDesiredStatus));
		SendPresence();
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


void SIPProto::CreateProtoService(const char *szService, SIPServiceFunc serviceProc)
{
	char str[MAXMODULELABELLENGTH];

	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
}


void SIPProto::CreateProtoService(const char *szService, SIPServiceFuncParam serviceProc, LPARAM lParam)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	::CreateServiceFunctionObjParam(str, (MIRANDASERVICEOBJPARAM)*(void**)&serviceProc, this, lParam);
}


HANDLE SIPProto::CreateProtoEvent(const char *szService)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
	return ::CreateHookableEvent(str);
}


void SIPProto::HookProtoEvent(const char *szEvent, SIPEventFunc pFunc)
{
	::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*(void**)&pFunc, this);
}


void SIPProto::ForkThread(SipThreadFunc pFunc, void* param)
{
	UINT threadID;
	CloseHandle((HANDLE)mir_forkthreadowner((pThreadFuncOwner)*(void**)&pFunc, this, param, &threadID));
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

#ifdef _DEBUG
	OutputDebugString(buff);
	OutputDebugString(_T("\n"));
#endif

	if (type == MESSAGE_TYPE_ERROR)
		ShowErrPopup(buff, m_tszUserName);

	CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) TcharToChar(buff).get());
}


void SIPProto::Disconnect()
{
	Trace(_T("Disconnecting..."));

	EnterCriticalSection(&cs);
	events.clear();
	LeaveCriticalSection(&cs);

	if (acc_id >= 0)
	{
		SendPresence(ID_STATUS_OFFLINE);
		pjsua_acc_del(acc_id);
		acc_id = -1;
	}

#define CLOSE_TRANSPORT(_T_)					\
	if (_T_ >= 0)								\
	{											\
		pjsua_transport_close(_T_, PJ_FALSE);	\
		_T_ = -1;								\
	}

	CLOSE_TRANSPORT(udp_transport_id)
	CLOSE_TRANSPORT(tcp_transport_id)
	CLOSE_TRANSPORT(tls_transport_id)

	for(HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		hContact != NULL; 
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)) 
	{
		if (!IsMyContact(hContact))
			continue;

		DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
		DBDeleteContactSetting(hContact, "CList", "StatusMsg");
	}

	m_iDesiredStatus = ID_STATUS_OFFLINE;
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


void SIPProto::DestroySIP()
{
	if (!hasToDestroy)
		return;

	pjsua_destroy();
	hasToDestroy = false;
}


int __cdecl SIPProto::OnPreShutdown(WPARAM wParam, LPARAM lParam)
{
	DestroySIP();
	return 0;
}


void SIPProto::NotifyCall(pjsua_call_id call_id, int state, HANDLE hContact, TCHAR *name, TCHAR *number)
{
	Trace(_T("NotifyCall %d -> %d"), call_id, state);

	char tmp[16];

	VOICE_CALL vc = {0};
	vc.cbSize = sizeof(vc);
	vc.moduleName = m_szModuleName;
	vc.id = itoa((int) call_id, tmp, 10);
	vc.flags = VOICE_TCHAR;
	vc.hContact = hContact;
	vc.ptszName = name;
	vc.ptszNumber = number;
	vc.state = state;

	NotifyEventHooks(hCallStateEvent, (WPARAM) &vc, 0);
}


bool SIPProto::SendPresence(int aStatus)
{
	pjsua_acc_info info;
	pj_status_t st = pjsua_acc_get_info(acc_id, &info);
	if (st != PJ_SUCCESS)
	{
		Error(st, _T("Error obtaining call info"));
		return false;
	}

	if (info.status != PJSIP_SC_OK)
		return false;
	
	pjrpid_element elem;
	pj_bzero(&elem, sizeof(elem));
	elem.type = PJRPID_ELEMENT_TYPE_PERSON;

	int status = FirstGtZero(aStatus, m_iStatus);

	switch(status)
	{
		case ID_STATUS_AWAY:
			elem.activity = PJRPID_ACTIVITY_AWAY;
			break;
		case ID_STATUS_OCCUPIED:
			elem.activity = PJRPID_ACTIVITY_BUSY;
			break;
		default:
			elem.activity = PJRPID_ACTIVITY_UNKNOWN;
			break;
	}

	TcharToSip sip_msg(GetAwayMsg(FirstGtZero(aStatus, m_iDesiredStatus)));
	elem.note = pj_str(sip_msg);

	pj_bool_t online = (status == ID_STATUS_INVISIBLE ? PJ_FALSE : PJ_TRUE);

	if (online == info.online_status && pj_strcmp(&info.online_status_text, &elem.note) == 0)
		return false;
	
	pjsua_acc_set_online_status2(acc_id, online, &elem);
	return true;
}


void SIPProto::on_reg_state()
{
	Trace(_T("on_reg_state"));

	pjsua_acc_info info;
	pj_status_t status = pjsua_acc_get_info(acc_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining account info"));
		return;
	}

	Trace(_T("on_reg_state : info.status = %d"), info.status);

	if (info.status == PJSIP_SC_OK)
	{
		int oldStatus = m_iStatus;

		BroadcastStatus(ConvertStatus(m_iDesiredStatus));
		SendPresence();

		if (oldStatus <= ID_STATUS_OFFLINE)
		{
			AddContactsToBuddyList();
		}
	}
	else
	{
		Disconnect();
	}
}

void SIPProto::on_incoming_call(pjsua_call_id call_id)
{
	Trace(_T("on_incoming_call: %d"), call_id);

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		return;
	}

	// Send 180/RINGING
	pjsua_call_answer(call_id, 180, NULL, NULL);

	pjsua_buddy_id buddy_id = pjsua_buddy_find(&info.remote_info);
	if (buddy_id != PJSUA_INVALID_ID)
	{
		HANDLE hContact = GetContact(buddy_id);
		
		NotifyCall(call_id, VOICE_STATE_RINGING, hContact);
	}
	else
	{
		SipToTchar remote_info(info.remote_info);

		TCHAR number[1024];
		number[0] = 0;
		CleanupURI(number, MAX_REGS(number), remote_info);

		TCHAR name[256];
		name[0] = 0;
		if (remote_info[0] == _T('"'))
		{
			const TCHAR *other = _tcsstr(&remote_info[1], _T("\" <"));
			if (other != NULL)
				lstrcpyn(name, &remote_info[1], min(MAX_REGS(name), other - remote_info));
		}

		NotifyCall(call_id, VOICE_STATE_RINGING, NULL, name, number);
	}
}


void SIPProto::on_call_state(pjsua_call_id call_id, const pjsua_call_info &info)
{
	Trace(_T("on_call_state: %d"), call_id);
	Trace(_T("Call info: %d / last: %d %s"), info.state, info.last_status, info.last_status_text);

	switch(info.state)
	{
/*		case PJSIP_INV_STATE_CALLING:
		case PJSIP_INV_STATE_INCOMING:
		{
			ConfigureDevices();
			break;
		}
*/		case PJSIP_INV_STATE_NULL:
		case PJSIP_INV_STATE_DISCONNECTED:
		{
			pjsua_set_null_snd_dev();
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


bool SIPProto::on_call_media_state_sync(pjsua_call_id call_id, const pjsua_call_info &info)
{
	Trace(_T("on_call_media_state_sync: %d"), call_id);

	// Connect ports appropriately when media status is ACTIVE or REMOTE HOLD,
	// otherwise we should NOT connect the ports.
	if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE || info.media_status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
	{
		ConfigureDevices();

		pjsua_conf_connect(info.conf_slot, 0);
		pjsua_conf_connect(0, info.conf_slot);
	}

	return true;
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

	int expectedInput = GetDevice(false, defaultInput);
	int expectedOutput = GetDevice(true, defaultOutput);

	if (input != expectedInput || output != expectedOutput)
		pjsua_set_snd_dev(expectedInput, expectedOutput);

	if (DBGetContactSettingByte(NULL, "VoiceService", "EchoCancelation", TRUE))
		pjsua_set_ec(PJSUA_DEFAULT_EC_TAIL_LEN, 0);
	else
		pjsua_set_ec(0, 0);
}


int __cdecl SIPProto::VoiceCaps(WPARAM wParam,LPARAM lParam)
{
	return VOICE_CAPS_VOICE | VOICE_CAPS_CALL_CONTACT | VOICE_CAPS_CALL_STRING;
}


void SIPProto::CleanupURI(TCHAR *out, int outSize, const TCHAR *url)
{
	if (url[0] == _T('"'))
	{
		const TCHAR *other = _tcsstr(&url[1], _T("\" <"));
		if (other != NULL)
			url = other + 2;
	}

	lstrcpyn(out, url, outSize);

	TCHAR *phone = _tcsstr(out, _T(";user=phone"));
	if (phone != NULL)
		*phone = 0;

	RemoveLtGt(out);

	phone = _tcsstr(out, _T(";user=phone"));
	if (phone != NULL)
		*phone = 0;

	if (_tcsnicmp(_T("sip:"), out, 4) == 0)
		lstrcpyn(out, &out[4], outSize);

	TCHAR *host = _tcschr(out, _T('@'));
	if (host != NULL && _tcsicmp(opts.domain, host+1) == 0)
		*host = 0;
}


void SIPProto::BuildTelURI(TCHAR *out, int outSize, const TCHAR *number)
{
	BuildURI(out, outSize, number, NULL, 0, true);
}


void SIPProto::BuildURI(TCHAR *out, int outSize, const TCHAR *user, const TCHAR *aHost, int port, bool isTel)
{
	if (user == NULL)
	{
		TCHAR *host = FirstNotEmpty((TCHAR *) aHost, opts.domain);

		if (port > 0)
			mir_sntprintf(out, outSize, _T("<sip:%s:%d>"), host, port);
		else
			mir_sntprintf(out, outSize, _T("<sip:%s>"), host);

		return;
	}

	TCHAR tmp[1024];
	CleanupURI(tmp, MAX_REGS(tmp), user);

	TCHAR *host = _tcschr(tmp, _T('@'));
	if (host != NULL)
	{
		*host = 0;
		host++;
	}

	if (host != NULL && aHost != NULL && lstrcmp(host, aHost) != 0)
		Error(_T("Two conflicting hosts: %s / %s , ignoring one in argument"), host, aHost);

	host = FirstNotEmpty(host, (TCHAR *) aHost, opts.domain);

	

	if (isTel && port > 0)
		mir_sntprintf(out, outSize, _T("<sip:%s@%s:%d;user=phone>"), tmp, host, port);
	else if (isTel)
		mir_sntprintf(out, outSize, _T("<sip:%s@%s;user=phone>"), tmp, host);
	else if (port > 0)
		mir_sntprintf(out, outSize, _T("<sip:%s@%s:%d>"), tmp, host, port);
	else
		mir_sntprintf(out, outSize, _T("<sip:%s@%s>"), tmp, host);
}


int __cdecl SIPProto::VoiceCall(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	TCHAR *number = (TCHAR *) lParam;

	TCHAR uri[1024];

	if (!IsEmpty(number))
	{
		if (!VoiceCallStringValid((WPARAM) number, 0))
			return 1;

		if (_tcsncmp(_T("sip:"), number, 4) == 0)
		{
			mir_sntprintf(uri, MAX_REGS(uri), _T("<%s>"), number);
		}
		else
			BuildTelURI(uri, MAX_REGS(uri), number);
	}
	else
	{
		DBTString buddy(hContact, m_szModuleName, "URI");
		if (buddy == NULL)
		{
			Error(_T("Contact %d has no URI!"), hContact);
			return -1;
		}

		lstrcpyn(uri, buddy, MAX_REGS(uri));
	}

	pjsua_call_id call_id;
	pj_str_t ret;
	pj_status_t status = pjsua_call_make_call(acc_id, pj_cstr(&ret, TcharToSip(uri)), 0, NULL, NULL, &call_id);
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

	if (info.state == PJSIP_INV_STATE_INCOMING || info.state == PJSIP_INV_STATE_EARLY)
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
	pj_str_t ret;
	pjsua_call_dial_dtmf(call_id, pj_cstr(&ret, TcharToSip(tmp)));

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
	BuildTelURI(tmp, MAX_REGS(tmp), number);
	return pjsua_verify_sip_url(TcharToSip(tmp)) == PJ_SUCCESS;
}


// Buddy ////////////////////////////////////////////////////////////////////////////////


HANDLE __cdecl SIPProto::SearchBasic(const char *id) 
{
	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;

	TCHAR tmp[1024];
	BuildURI(tmp, MAX_REGS(tmp), CharToTchar(id));

	TCHAR *uri = mir_tstrdup(tmp); 
	ForkThread(&SIPProto::SearchUserThread, uri);

	return uri;
}


void __cdecl SIPProto::SearchUserThread(void *param)
{
	TCHAR *uri = (TCHAR *) param;

    pj_thread_desc desc;
    pj_thread_t *this_thread;
    pj_status_t status = pj_thread_register("SearchUserThread", desc, &this_thread);
    if (status != PJ_SUCCESS) 
	{
        Error(status, _T("Error registering thread"));
        return;
    }

	TcharToSip sip_uri(uri);
	if (pjsua_verify_sip_url(sip_uri) != PJ_SUCCESS)
	{
		Error(_T("Not a valid SIP URL: %s"), uri);
		SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_FAILED, param, 0);
		return;
	}

	pj_str_t ret;
	if (pjsua_buddy_find(pj_cstr(&ret, sip_uri)) != PJSUA_INVALID_ID)
	{
		Info(_T("Contact already in your contact list"));
		SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, param, 0);
		return;
	}

	TCHAR name[1024];
	CleanupURI(name, MAX_REGS(name), uri);
	TcharToChar name_char(name);

	PROTOSEARCHRESULT isr = {0};
	isr.cbSize = sizeof(isr);
	isr.nick = (char *) name_char.get();

	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, param, (LPARAM)&isr);
	SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, param, 0);
}


HANDLE SIPProto::CreateContact(const TCHAR *uri, bool temporary)
{
	pjsua_buddy_config buddy_cfg;
	pjsua_buddy_config_default(&buddy_cfg);

	TcharToSip sip_uri(uri);
	buddy_cfg.uri = pj_str(sip_uri);
	buddy_cfg.subscribe = PJ_FALSE;

	pjsua_buddy_id buddy_id;
	pj_status_t status = pjsua_buddy_add(&buddy_cfg, &buddy_id);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error adding buddy '%s'"), uri);
		return NULL;
	}

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_ADD, 0, 0);
	CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) m_szModuleName);

	DBWriteContactSettingTString(hContact, m_szModuleName, "URI", uri);

	TCHAR nick[1024];
	CleanupURI(nick, MAX_REGS(nick), uri);
	DBWriteContactSettingTString(hContact, m_szModuleName, "Nick", nick);

	if (temporary)
		DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
	
	Attach(hContact, buddy_id);

	return hContact;
}


HANDLE SIPProto::AddToList(int flags, const TCHAR *uri)
{
	if (m_iStatus <= ID_STATUS_OFFLINE)
		return NULL;

	HANDLE hContact = GetContact(uri, true, flags & PALF_TEMPORARY);
	
	if (flags & PALF_TEMPORARY)
	{
		DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
		DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
	}
	else
	{
		DBDeleteContactSetting(hContact, "CList", "NotOnList");
		DBDeleteContactSetting(hContact, "CList", "Hidden");
	}

	return hContact;
}


HANDLE __cdecl SIPProto::AddToList(int flags, PROTOSEARCHRESULT *psr)
{
	TCHAR uri[1024];
	BuildURI(uri, MAX_REGS(uri), CharToTchar(psr->nick));
	return AddToList(flags, uri);
}


int __cdecl SIPProto::AuthRequest(HANDLE hContact, const char* szMessage)
{
	pjsua_buddy_id buddy_id = GetBuddy(hContact);
	if (buddy_id == PJSUA_INVALID_ID)
		return 1;

	DBWriteContactSettingByte(hContact, m_szModuleName, "WantAuth", 1);

	pjsua_buddy_subscribe_pres(buddy_id, PJ_TRUE);
	return 0;
}


void SIPProto::on_buddy_state(pjsua_buddy_id buddy_id)
{
	HANDLE hContact = GetContact(buddy_id);
	if (hContact == NULL)
		return;

	pjsua_buddy_info info;
    pj_status_t status = pjsua_buddy_get_info(buddy_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining buddy info"));
		DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
		return;
	}

	if (info.sub_state == PJSIP_EVSUB_STATE_NULL && info.sub_term_code == 404)
	{
		DBTString nick(hContact, m_szModuleName, "Nick");
		Info(_T("User '%s' not found on the server!"), nick.get());
		DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
		return;
	}

	if (info.sub_state != PJSIP_EVSUB_STATE_ACTIVE)
	{
		DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
		return;
	}

	switch(info.status)
	{
		case PJSUA_BUDDY_STATUS_ONLINE:
		{
			if (info.rpid.type == PJRPID_ELEMENT_TYPE_PERSON && info.rpid.id.ptr != NULL)
			{
				SipToTchar note(info.rpid.note);
				TCHAR *defaultNote = _T("");

				switch(info.rpid.activity)
				{
					case PJRPID_ACTIVITY_UNKNOWN:
					{
						DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_ONLINE);
						break;
					}
					case PJRPID_ACTIVITY_AWAY:
					{
						DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_AWAY);
						defaultNote = _T("Away");
						break;
					}
					case PJRPID_ACTIVITY_BUSY:
					{
						DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OCCUPIED);
						defaultNote = _T("Busy");
						break;
					}
				}

				if (IsEmpty(note) || lstrcmpi(note, defaultNote) == 0)
					DBDeleteContactSetting(hContact, "CList", "StatusMsg");
				else
					DBWriteContactSettingTString(hContact, "CList", "StatusMsg", note);
			}
			else
			{
				DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_ONLINE);
				DBDeleteContactSetting(hContact, "CList", "StatusMsg");
			}
			break;
		}
		case PJSUA_BUDDY_STATUS_OFFLINE:
		{
			DBWriteContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE);
			DBDeleteContactSetting(hContact, "CList", "StatusMsg");
			break;
		}
		default:
		{
			break;
		}
	}
}

bool SIPProto::on_pager_sync(pjsua_call_id call_id, const pj_str_t *from, const pj_str_t *to, 
							 const pj_str_t *contact, const pj_str_t *mime_type, const pj_str_t *text, pjsip_rx_data *rdata)
{
	HANDLE hContact = GetContact(SipToTchar(*from), true, true);
	if (hContact == NULL)
		return false;

	LoadMirVer(hContact, rdata);

	return true;
}

void SIPProto::on_pager(char *from, char *text, char *mime_type)
{
	// Ignore empty messages
	if (strlen(text) < 1)
		return;

	if (strcmp("text/plain", mime_type) != 0)
	{
		Error(_T("Unknown mime type in message from %s: %s"), 
			SipToTchar(pj_str(from)).get(), SipToTchar(pj_str(mime_type)).get());
		return;
	}

	HANDLE hContact = GetContact(SipToTchar(pj_str(from)), true, true);
	if (hContact == NULL)
		return;

	CallService(MS_PROTO_CONTACTISTYPING, (WPARAM) hContact, 0);

	PROTORECVEVENT pre;
	pre.szMessage = (char*) text;
	pre.flags = PREF_UTF;
	pre.timestamp = (DWORD) time(NULL);
	pre.lParam = 0;

	CCSDATA ccs = {0};
	ccs.hContact = hContact;
	ccs.szProtoService = PSR_MESSAGE;
	ccs.wParam = 0;
	ccs.lParam = (LPARAM) &pre;
	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) &ccs);
}


int __cdecl SIPProto::RecvMsg(HANDLE hContact, PROTORECVEVENT *pre)
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, (LPARAM) pre };
	return CallService(MS_PROTO_RECVMSG, 0, (LPARAM) &ccs);
}


int __cdecl SIPProto::SendMsg(HANDLE hContact, int flags, const char *msg)
{
	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;

	pjsua_buddy_id buddy_id = GetBuddy(hContact);
	if (buddy_id == PJSUA_INVALID_ID)
		return 0;
	
	pjsua_buddy_info info;
    pj_status_t status = pjsua_buddy_get_info(buddy_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining buddy info"));
		return 0;
	}

	scoped_mir_free<char> text;
	if (flags & PREF_UNICODE) 
	{
		const char *p = strchr(msg, '\0');
		if (p != msg) 
		{
			while (*(++p) == '\0') {}
			text = mir_utf8encodeW((wchar_t *) p);
		}
		else
		{
			text = mir_strdup(msg);
		}
	}
	else
	{
		text = (flags & PREF_UTF) ? mir_strdup(msg) : mir_utf8encode(msg);
	}

	// TODO Send to call

	MessageData *md = new MessageData();
	md->messageID = InterlockedIncrement(&messageID);
	md->hContact = hContact;

	pj_str_t ret;
	status = pjsua_im_send(acc_id, &info.uri, NULL, pj_cstr(&ret, text), NULL, md);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error sending message"));
		ForkThread(&SIPProto::FakeMsgAck, md);
	}

	return md->messageID;
}


void __cdecl SIPProto::FakeMsgAck(void *param)
{
	MessageData *md = (MessageData *) param;

	Sleep(150);
	on_pager_status(md->hContact, md->messageID, PJSIP_SC_BAD_REQUEST, Translate("Error sending message"));

	delete md;
}


void SIPProto::on_pager_status(HANDLE hContact, LONG messageID, pjsip_status_code status, char *text)
{
	SendBroadcast(hContact, ACKTYPE_MESSAGE, 
				  status != PJSIP_SC_OK ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
				  (HANDLE) messageID, (LPARAM) text);
}


bool SIPProto::on_typing_sync(pjsua_call_id call_id, const pj_str_t *from, const pj_str_t *to, 
							  const pj_str_t *contact, pj_bool_t is_typing, pjsip_rx_data *rdata)
{
	pjsua_buddy_id buddy_id = pjsua_buddy_find(from);
	if (buddy_id == PJSUA_INVALID_ID)
		return false;

	HANDLE hContact = GetContact(buddy_id);
	if (hContact == NULL)
		return false;

	LoadMirVer(hContact, rdata);

	return true;
}


void SIPProto::on_typing(char *from, bool isTyping)
{
	pj_str_t ret;
	pjsua_buddy_id buddy_id = pjsua_buddy_find(pj_cstr(&ret, from));
	if (buddy_id == PJSUA_INVALID_ID)
		return;

	HANDLE hContact = GetContact(buddy_id);
	if (hContact == NULL)
		return;

	CallService(MS_PROTO_CONTACTISTYPING, (WPARAM) hContact, isTyping ? 10 : 0);
}


int __cdecl SIPProto::UserIsTyping(HANDLE hContact, int type)
{
	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;

	pjsua_buddy_id buddy_id = GetBuddy(hContact);
	if (buddy_id == PJSUA_INVALID_ID)
		return 0;

	pjsua_buddy_info info;
    pj_status_t status = pjsua_buddy_get_info(buddy_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining buddy info"));
		return 0;
	}

	// TODO Send to call

	status = pjsua_im_typing(acc_id, &info.uri, type == PROTOTYPE_SELFTYPING_ON ? PJ_TRUE : PJ_FALSE, NULL);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error sending typing notification"));
		return 0;
	}

	return 0;
}


void SIPProto::LoadMirVer(HANDLE hContact, pjsip_rx_data *rdata)
{
	pj_str_t ret;
	pjsip_hdr *hdr = (pjsip_hdr *) pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, pj_cstr(&ret, "User-Agent"), NULL);
	if (hdr)
	{
		char buff[1024];
		pjsip_hdr_print_on(hdr, buff, MAX_REGS(buff));

		TCHAR mirver[1024];
		lstrcpyn(mirver, SipToTchar(pj_str(&buff[11])), MAX_REGS(mirver));
		lstrtrim(mirver);

		DBWriteContactSettingTString(hContact, m_szModuleName, "MirVer", mirver);
	}
	else
	{
		DBDeleteContactSetting(hContact, m_szModuleName, "MirVer");
	}
}


// Handler for incoming presence subscription request
bool SIPProto::on_incoming_subscribe_sync(pjsua_srv_pres *srv_pres,
										  pjsua_buddy_id buddy_id,
										  const pj_str_t *from,
										  pjsip_rx_data *rdata,
										  pjsip_status_code *code,
										  pj_str_t *pj_reason,
										  pjsua_msg_data *msg_data)
{
	if (buddy_id == PJSUA_INVALID_ID)
	{
		*code = PJSIP_SC_ACCEPTED;
		return true;
	}

	HANDLE hContact = GetContact(buddy_id);
	if (hContact == NULL)
	{
		*code = PJSIP_SC_ACCEPTED;
		return true;
	}

	LoadMirVer(hContact, rdata);

	int authState = DBGetContactSettingByte(hContact, m_szModuleName, "AuthState", 0);
	switch(authState)
	{
		case AUTH_STATE_AUTHORIZED:
			*code = PJSIP_SC_OK;
			return false;
		case AUTH_STATE_DENIED:
			*code = PJSIP_SC_NOT_FOUND;
			return false;
		default:
			*code = PJSIP_SC_ACCEPTED;
			return true;
	}
}


void SIPProto::on_incoming_subscribe(char *from, char *reason, pjsua_srv_pres *srv_pres)
{
	SipToTchar uri(pj_str(from));

	HANDLE hContact = GetContact(uri, true, true);
	if (hContact == NULL)
		return;

	TCHAR nick[1024];
	CleanupURI(nick, MAX_REGS(nick), uri);

	TcharToChar nicka(nick);

	CCSDATA ccs = {0};
	PROTORECVEVENT pre = {0};

	pre.timestamp = (DWORD) time(NULL);
	pre.lParam = sizeof(DWORD) * 2 + strlen(nicka) + strlen(from) 
				+ sizeof(pjsua_srv_pres *) + 6;

	ccs.szProtoService = PSR_AUTH;
	ccs.hContact = hContact;
	ccs.lParam = (LPARAM)&pre;

	PBYTE pCurBlob = (PBYTE)alloca(pre.lParam);
	pre.szMessage = (char*)pCurBlob;

	*(PDWORD)pCurBlob = 0; pCurBlob+=sizeof(DWORD);
	*(PDWORD)pCurBlob = (DWORD)hContact; pCurBlob+=sizeof(DWORD);
	strcpy((char*)pCurBlob, nicka); pCurBlob += strlen(nicka)+1;
	*pCurBlob = '\0'; pCurBlob++;	   //firstName
	*pCurBlob = '\0'; pCurBlob++;	   //lastName
	*pCurBlob = '\0'; pCurBlob++;	   //email
	*pCurBlob = '\0'; pCurBlob++;	   //reason
	strcpy((char*)pCurBlob, from); pCurBlob += strlen(from)+1;  //from
	*(pjsua_srv_pres **)pCurBlob = srv_pres;

	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) &ccs);
}


int __cdecl SIPProto::AuthRecv(HANDLE hContact, PROTORECVEVENT *pre)
{
	DBEVENTINFO dbei = {0};

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szModuleName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
	dbei.eventType = EVENTTYPE_AUTHREQUEST;

	// Just copy the Blob from PSR_AUTH event
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;
	CallService(MS_DB_EVENT_ADD, 0,(LPARAM)&dbei);

	return 0;
}


HANDLE __cdecl SIPProto::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent) 
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) hDbEvent, 0);

	if ((int) dbei.cbBlob == -1)
		return NULL;

	dbei.pBlob = (PBYTE) alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei))
		return NULL;

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
		return NULL;

	if (strcmp(dbei.szModule, m_szModuleName) != 0)
		return NULL;

	char *nick = (char*)(dbei.pBlob + sizeof(DWORD)*2);
	char *firstName = nick + strlen(nick) + 1;
	char *lastName = firstName + strlen(firstName) + 1;
	char *email = lastName + strlen(lastName) + 1;
	char *reason = email + strlen(email) + 1;
	char *from = reason + strlen(reason) + 1;
	pjsua_srv_pres *srv_pres = (pjsua_srv_pres *) (from + strlen(from) + 1);

	return AddToList(flags, SipToTchar(pj_str(from))); 
}


int __cdecl SIPProto::Authorize(HANDLE hDbEvent)
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) hDbEvent, 0);

	if ((int) dbei.cbBlob == -1)
		return 1;

	dbei.pBlob = (PBYTE) alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei))
		return 1;

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
		return 1;

	if (strcmp(dbei.szModule, m_szModuleName) != 0)
		return 1;

	char *nick = (char*)(dbei.pBlob + sizeof(DWORD)*2);
	char *firstName = nick + strlen(nick) + 1;
	char *lastName = firstName + strlen(firstName) + 1;
	char *email = lastName + strlen(lastName) + 1;
	char *reason = email + strlen(email) + 1;
	char *from = reason + strlen(reason) + 1;
	pjsua_srv_pres *srv_pres = (pjsua_srv_pres *) (from + strlen(from) + 1);

	HANDLE hContact = GetContact(SipToTchar(pj_str(from)));
	if (hContact != NULL)
		DBWriteContactSettingByte(hContact, m_szModuleName, "AuthState", AUTH_STATE_AUTHORIZED);

	pjsua_pres_notify(acc_id, srv_pres, PJSIP_EVSUB_STATE_ACCEPTED, NULL, NULL, true, NULL);

	return 0;
}


int __cdecl SIPProto::AuthDeny(HANDLE hDbEvent, const char *szReason)
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) hDbEvent, 0);

	if ((int) dbei.cbBlob == -1)
		return 1;

	dbei.pBlob = (PBYTE) alloca(dbei.cbBlob);
	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei))
		return 1;

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
		return 1;

	if (strcmp(dbei.szModule, m_szModuleName) != 0)
		return 1;

	char *nick = (char*)(dbei.pBlob + sizeof(DWORD)*2);
	char *firstName = nick + strlen(nick) + 1;
	char *lastName = firstName + strlen(firstName) + 1;
	char *email = lastName + strlen(lastName) + 1;
	char *from = email + strlen(email) + 1;
	pjsua_srv_pres *srv_pres = (pjsua_srv_pres *) (from + strlen(from) + 1);

	HANDLE hContact = GetContact(SipToTchar(pj_str(from)), true, false);
	if (hContact == NULL)
		return 1;

	if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
	{
		DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
		DBDeleteContactSetting(hContact, "CList", "NotOnList");
	}

	DBWriteContactSettingByte(hContact, m_szModuleName, "AuthState", AUTH_STATE_DENIED);

	pj_str_t ret;
	pjsua_pres_notify(acc_id, srv_pres, PJSIP_EVSUB_STATE_TERMINATED, NULL, 
					  pj_cstr(&ret, TcharToSip(CharToTchar(szReason))), false, NULL);

	return 0;
}


pjsua_buddy_id SIPProto::GetBuddy(HANDLE hContact)
{
	pjsua_buddy_id id =(pjsua_buddy_id) DBGetContactSettingDword(hContact, m_szModuleName, "ID", PJSUA_INVALID_ID);

	if (id == PJSUA_INVALID_ID)
		Error(_T("hContact has no buddy: %d"), hContact);
	
	return id;
}


HANDLE SIPProto::GetContact(pjsua_buddy_id buddy_id)
{
	HANDLE hContact = (HANDLE) pjsua_buddy_get_user_data(buddy_id);
	
	if (hContact == NULL)
		Error(_T("Buddy has no hContact: %d"), buddy_id);
	
	return hContact;
}


HANDLE SIPProto::GetContact(const TCHAR *uri, bool addIfNeeded, bool temporary)
{
	pj_str_t ret;
	pjsua_buddy_id buddy_id = pjsua_buddy_find(pj_cstr(&ret, TcharToSip(uri)));

	if (buddy_id != PJSUA_INVALID_ID)
		return GetContact(buddy_id);

	if (addIfNeeded)
		return CreateContact(uri, temporary);
	
	return NULL;
}


void SIPProto::Attach(HANDLE hContact, pjsua_buddy_id buddy_id)
{
	DBWriteContactSettingDword(hContact, m_szModuleName, "ID", (DWORD) buddy_id);
	pjsua_buddy_set_user_data(buddy_id, hContact);
}


int __cdecl SIPProto::OnContactDeleted(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;

	if (acc_id < 0)
		return 0;

	if (!IsMyContact(hContact))
		return 0;

	Trace(_T("Deleted contact"));

	pjsua_buddy_id buddy_id = GetBuddy(hContact);
	if (buddy_id == PJSUA_INVALID_ID)
		return 0;

	//pjsua_buddy_subscribe_pres(buddy_id, PJ_FALSE);
	pjsua_buddy_del(buddy_id);

	return 0;
}


// Away message /////////////////////////////////////////////////////////////////////////

struct AwayMsgInfo
{
	LONG id;
	HANDLE hContact;
};

void __cdecl SIPProto::GetAwayMsgThread(void* arg)
{
	Sleep(150);

	AwayMsgInfo *inf = (AwayMsgInfo *) arg;

	DBTString msg(inf->hContact, "CList", "StatusMsg");
	if (msg != NULL)
		SendBroadcast(inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)inf->id, (LPARAM) TcharToChar(msg).get());
	else 
		SendBroadcast(inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)inf->id, (LPARAM) "");

	mir_free(inf);
}

HANDLE __cdecl SIPProto::GetAwayMsg(HANDLE hContact)
{
	AwayMsgInfo *inf = (AwayMsgInfo*) mir_alloc(sizeof(AwayMsgInfo));
	inf->hContact = hContact;
	inf->id = InterlockedIncrement(&awayMessageID);

	ForkThread(&SIPProto::GetAwayMsgThread, inf);
	return (HANDLE) inf->id;
}


int __cdecl SIPProto::SetAwayMsg(int status, const char *msga)
{
	int pos = status - ID_STATUS_ONLINE;
	if (pos < 0 || pos >= MAX_REGS(awayMessages))
		return 1;

	CharToTchar msg(msga);
	if (lstrcmp(FirstNotEmpty((TCHAR *)msg.get(), _T("")), FirstNotEmpty(awayMessages[pos], _T(""))) == 0)
		return 0;

	mir_free(awayMessages[pos]);
	awayMessages[pos] = msg.detach();

	if (ConvertStatus(status) == m_iStatus)
		SendPresence();

	return 0;
}


INT_PTR __cdecl SIPProto::GetMyAwayMsg(WPARAM wParam, LPARAM lParam)
{
	TCHAR *msg = GetAwayMsg(wParam ? wParam : m_iStatus);
	return (lParam & SGMA_UNICODE) ? (INT_PTR) mir_t2u(msg)
									: (INT_PTR) mir_t2a(msg);
}


TCHAR *SIPProto::GetAwayMsg(int status)
{
	if (status == ID_STATUS_INVISIBLE)
		return _T("");

	int pos = status - ID_STATUS_ONLINE;
	if (pos < 0 || pos >= MAX_REGS(awayMessages))
		return _T("");
	else if (awayMessages[pos] == NULL)
		return _T("");
	else
		return awayMessages[pos];
}


// Dialog procs /////////////////////////////////////////////////////////////////////////


static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SIPProto *proto = (SIPProto *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg) 
	{
		case WM_INITDIALOG: 
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			proto = (SIPProto *) lParam;

			INT_PTR ret = SaveOptsDlgProc(proto->accountManagerCtrls, MAX_REGS(proto->accountManagerCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);

			TCHAR username[1024];
			if (lstrcmp(proto->opts.domain, proto->opts.registrar.host) == 0)
				lstrcpyn(username, proto->opts.username, MAX_REGS(username));
			else
				mir_sntprintf(username, MAX_REGS(username), _T("%s@%s"), 
								proto->opts.username, proto->opts.domain);

			SetDlgItemText(hwnd, IDC_USERNAME, username);
			SendDlgItemMessage(hwnd, IDC_USERNAME, EM_LIMITTEXT, 1024, 0);

			return ret;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_USERNAME)
			{
				if (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())
					return 0;

				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);

				return 0;
			}

			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;

			if (lpnmhdr->idFrom == 0 && lpnmhdr->code == PSN_APPLY)
			{
				INT_PTR ret = SaveOptsDlgProc(proto->accountManagerCtrls, MAX_REGS(proto->accountManagerCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);

				TCHAR username[1024];
				GetDlgItemText(hwnd, IDC_USERNAME, username, MAX_REGS(username));
				TCHAR *host = _tcschr(username, _T('@'));
				if (host != NULL)
				{
					*host = 0;
					host++;
				}

				lstrcpyn(proto->opts.username, username, MAX_REGS(proto->opts.username));
				DBWriteContactSettingTString(NULL, proto->m_szModuleName, "Username", proto->opts.username);

				lstrcpyn(proto->opts.domain, FirstNotEmpty(host, proto->opts.registrar.host), 
						 MAX_REGS(proto->opts.domain));
				DBWriteContactSettingTString(NULL, proto->m_szModuleName, "Domain", proto->opts.domain);

				return ret;
			}

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


