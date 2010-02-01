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

#define NUM_LINES 3


static INT_PTR CALLBACK DlgProcAccMgrUI(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DlgOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int static_iaxc_callback(iaxc_event e, void *param);


IAXProto::IAXProto(HMEMORYMODULE iaxclient, const char *aProtoName, const TCHAR *aUserName)
	: iaxclient(iaxclient)
{
#define LOAD(_F_) * (void **) & iax._F_ = (void *) MemoryGetProcAddress(iaxclient, "iaxc_" #_F_)
	LOAD(set_event_callback);
	LOAD(set_event_callpost);
	LOAD(free_event);
	LOAD(get_event_levels);
	LOAD(get_event_text);
	LOAD(get_event_state);
	LOAD(set_preferred_source_udp_port);
	LOAD(get_bind_port);
	LOAD(initialize);
	LOAD(shutdown);
	LOAD(set_formats);
	LOAD(set_min_outgoing_framesize);
	LOAD(set_callerid);
	LOAD(start_processing_thread);
	LOAD(stop_processing_thread);
	LOAD(call);
	LOAD(call_ex);
	LOAD(unregister);
	* (void **) & iax.register_ = (void *) MemoryGetProcAddress(iaxclient, "iaxc_register");
	LOAD(register_ex);
	LOAD(send_busy_on_incoming_call);
	LOAD(answer_call);
	LOAD(key_radio);
	LOAD(unkey_radio);
	LOAD(blind_transfer_call);
	LOAD(setup_call_transfer);
	LOAD(dump_all_calls);
	LOAD(dump_call_number);
	LOAD(dump_call);
	LOAD(reject_call);
	LOAD(reject_call_number);
	LOAD(send_dtmf);
	LOAD(send_text);
	LOAD(send_text_call);
	LOAD(send_url);
	LOAD(get_call_state);
	LOAD(millisleep);
	LOAD(set_silence_threshold);
	LOAD(set_audio_output);
	LOAD(select_call);
	LOAD(first_free_call);
	LOAD(selected_call);
	LOAD(quelch);
	LOAD(unquelch);
	LOAD(mic_boost_get);
	LOAD(mic_boost_set);
	LOAD(version);
	LOAD(set_jb_target_extra);
	LOAD(set_networking);
	LOAD(get_netstats);
	LOAD(audio_devices_get);
	LOAD(audio_devices_set);
	LOAD(input_level_get);
	LOAD(output_level_get);
	LOAD(input_level_set);
	LOAD(output_level_set);
	LOAD(play_sound);
	LOAD(stop_sound);
	LOAD(get_filters);
	LOAD(set_filters);
	LOAD(set_speex_settings);
	LOAD(get_audio_prefs);
	LOAD(set_audio_prefs);
	LOAD(video_devices_get);
	LOAD(video_device_set);
	LOAD(get_video_prefs);
	LOAD(set_video_prefs);
	LOAD(video_format_get_cap);
	LOAD(video_format_set_cap);
	LOAD(video_format_set);
	LOAD(video_params_change);
	LOAD(set_holding_frame);
	LOAD(video_bypass_jitter);
	LOAD(is_camera_working);
	LOAD(YUV420_to_RGB32);
	LOAD(set_test_mode);
	LOAD(push_audio);
	LOAD(push_video);
	LOAD(debug_iax_set);

	InitializeCriticalSection(&cs);

	reg_id = -1;
	hNetlibUser = 0;
	hCallStateEvent = 0;
	m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;
	voiceMessages = 0;

	m_tszUserName = mir_tstrdup(aUserName);
	m_szProtoName = mir_strdup(aProtoName);
	m_szModuleName = mir_strdup(aProtoName);

	static OptPageControl amCtrls[] = { 
		{ NULL,	CONTROL_TEXT,		IDC_HOST,			"Host", NULL, 0, 0, 256 },
		{ NULL,	CONTROL_INT,		IDC_PORT,			"Port", 4569, 0, 1 },
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
		{ NULL,	CONTROL_INT,		IDC_PORT,			"Port", 4569, 0, 1 },
		{ NULL,	CONTROL_TEXT,		IDC_USERNAME,		"Username", NULL, 0, 0, 16 },
		{ NULL,	CONTROL_PASSWORD,	IDC_PASSWORD,		"Password", NULL, IDC_SAVEPASSWORD, 0, 16 },
		{ NULL,	CONTROL_CHECKBOX,	IDC_SAVEPASSWORD,	"SavePassword", (BYTE) TRUE },
		{ NULL,	CONTROL_TEXT,		IDC_NAME,			"CallerID_Name", NULL, 0, 0, 128 },
		{ NULL,	CONTROL_TEXT,		IDC_NUMBER,			"CallerID_Number", NULL, 0, 0, 128 },
	};

	memmove(optionsCtrls, oCtrls, sizeof(optionsCtrls));
	optionsCtrls[0].var = &opts.host;
	optionsCtrls[1].var = &opts.port;
	optionsCtrls[2].var = &opts.username;
	optionsCtrls[3].var = &opts.password;
	optionsCtrls[4].var = &opts.savePassword;
	optionsCtrls[5].var = &opts.callerID.name;
	optionsCtrls[6].var = &opts.callerID.number;

	LoadOpts(optionsCtrls, MAX_REGS(optionsCtrls), m_szModuleName);

	CreateProtoService(PS_CREATEACCMGRUI, &IAXProto::CreateAccMgrUI);
	CreateProtoService(PS_GETUNREADEMAILCOUNT, &IAXProto::GetUnreadEmailCount);

	CreateProtoService(PS_VOICE_CAPS, &IAXProto::VoiceCaps);
	CreateProtoService(PS_VOICE_CALL, &IAXProto::VoiceCall);
	CreateProtoService(PS_VOICE_ANSWERCALL, &IAXProto::VoiceAnswerCall);
	CreateProtoService(PS_VOICE_DROPCALL, &IAXProto::VoiceDropCall);
	CreateProtoService(PS_VOICE_HOLDCALL, &IAXProto::VoiceHoldCall);
	CreateProtoService(PS_VOICE_SEND_DTMF, &IAXProto::VoiceSendDTMF);
	CreateProtoService(PS_VOICE_CALL_STRING_VALID, &IAXProto::VoiceCallStringValid);

	hCallStateEvent = CreateProtoEvent(PE_VOICE_CALL_STATE);

	char icon[512];
	mir_snprintf(icon, MAX_REGS(icon), "%s_main", m_szModuleName);

	TCHAR section[100];
	mir_sntprintf(section, MAX_REGS(section), _T("%s/%s"), LPGENT("Protocols"), m_tszUserName);

	IcoLib_Register(icon, section, LPGENT("Protocol icon"), IDI_PROTO);

	HookProtoEvent(ME_OPT_INITIALISE, &IAXProto::OnOptionsInit);
}


IAXProto::~IAXProto()
{
	Netlib_CloseHandle(hNetlibUser);

	mir_free(m_tszUserName);
	mir_free(m_szProtoName);
	mir_free(m_szModuleName);

	DeleteCriticalSection(&cs);

	MemoryFreeLibrary(iaxclient);
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
			return 0;

		case PFLAG_UNIQUEIDTEXT:
			return (UINT_PTR) Translate("Username");

		case PFLAG_UNIQUEIDSETTING:
			return (UINT_PTR) "Username";

		case PFLAG_MAXLENOFMESSAGE:
			return 100;
	}

	return 0;
}


HICON __cdecl IAXProto::GetIcon(int iconIndex)
{
	if (LOWORD(iconIndex) == PLI_PROTOCOL)
	{
		char icon[512];
		mir_snprintf(icon, MAX_REGS(icon), "%s_main", m_szModuleName);

		return IcoLib_LoadIcon(icon, TRUE);
	}

	return NULL;
}


INT_PTR __cdecl IAXProto::SetStatus( int iNewStatus ) 
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
		Trace(_T("Connecting..."));

		BroadcastStatus(ID_STATUS_CONNECTING);

		TCHAR server_port[1024];
		mir_sntprintf(server_port, MAX_REGS(server_port), _T("%s:%d"), opts.host, opts.port);

		reg_id = iax.register_(TcharToUtf8(opts.username), opts.password, TcharToUtf8(server_port));
		if (reg_id < 0)
		{
			Error(_T("Error registering with IAX server"));
			Disconnect();
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
	Trace(_T("BroadcastStatus %d"), newStatus);

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


void IAXProto::HookProtoEvent(const char* szEvent, IAXEventFunc pFunc)
{
	::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*(void**)&pFunc, this);
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


#define MESSAGE_TYPE_TRACE 0
#define MESSAGE_TYPE_INFO 1
#define MESSAGE_TYPE_ERROR 2


void IAXProto::Trace(const TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_TRACE, fmt, args);

	va_end(args);
}


void IAXProto::Info(const TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_INFO, TranslateTS(fmt), args);

	va_end(args);
}


void IAXProto::Error(const TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_ERROR, TranslateTS(fmt), args);

	va_end(args);
}


void IAXProto::ShowMessage(int type, const TCHAR *fmt, va_list args) 
{
	TCHAR buff[1024];
	if (type == MESSAGE_TYPE_TRACE)
		lstrcpy(buff, _T("TRACE: "));
	else if (type == MESSAGE_TYPE_INFO)
		lstrcpy(buff, _T("INFO : "));
	else
		lstrcpy(buff, _T("ERROR: "));

	mir_vsntprintf(&buff[7], MAX_REGS(buff)-7, fmt, args);

	if (type == MESSAGE_TYPE_INFO)
		ShowInfoPopup(&buff[7], m_tszUserName);
	else if (type == MESSAGE_TYPE_ERROR)
		ShowErrPopup(&buff[7], m_tszUserName);

//	OutputDebugString(buff);
//	OutputDebugString(_T("\n"));
	CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) TcharToChar(buff).get());
}


void IAXProto::Disconnect()
{
	Trace(_T("Disconnecting..."));

	if (reg_id >= 0)
	{
		iax.dump_all_calls();
		
		iax.unregister(reg_id);
		reg_id = -1;
	}

	m_iDesiredStatus = ID_STATUS_OFFLINE;
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
			Trace(_T("Status: %s"), Utf8ToTchar(text.message));
			return 1;
		}
		case IAXC_TEXT_TYPE_NOTICE:
		{
			if (strncmp("Originating an ", text.message, 15) == 0)
				Trace(_T("Notice: %s"), Utf8ToTchar(text.message));
			else
				Info(_T("%s"), Utf8ToTchar(text.message)); // To avoid translate
			return 1;
		}
		case IAXC_TEXT_TYPE_ERROR:
		{
			Error(_T("%s"), Utf8ToTchar(text.message)); // To avoid translate
			return 1;
		}
		case IAXC_TEXT_TYPE_FATALERROR:
		{
			Error(_T("Fatal: %s"), Utf8ToTchar(text.message));
			Disconnect();
			return 1;
		}
		case IAXC_TEXT_TYPE_IAX:
		{
			Trace(_T("IAX: %s"), Utf8ToTchar(text.message));
			return 1;
		}
		default:
			return 0;
	}

	return 0;
}


int IAXProto::state_callback(iaxc_ev_call_state &call)
{
	Trace(_T("callNo %d state 0x%x"), call.callNo, call.state);

	if (call.state == IAXC_CALL_STATE_FREE)
	{
		NotifyCall(call.callNo, VOICE_STATE_ENDED);
		return 0;
	}

	TCHAR name[256];
	TCHAR number[256];
	name[0] = 0;
	number[0] = 0;

	if ((call.state & IAXC_CALL_STATE_OUTGOING) == 0)
	{
		const char *otherName = call.remote_name;
		const char *otherNumber = call.remote;

		if (strcmp(otherName, "unknown") == 0)
			otherName = NULL;
		if (strcmp(otherNumber, "unknown") == 0)
			otherNumber = NULL;

		if (otherName != NULL)
		{
			lstrcpyn(name, Utf8ToTchar(otherName), MAX_REGS(name));
			lstrtrim(name);
		}
		if (otherNumber != NULL)
		{
			lstrcpyn(number, Utf8ToTchar(otherNumber), MAX_REGS(number));
			lstrtrim(number);
		}
	}

	if (call.state & IAXC_CALL_STATE_BUSY)
	{
		NotifyCall(call.callNo, VOICE_STATE_BUSY, NULL, name, number);
	}
	else if (call.state & IAXC_CALL_STATE_RINGING)
	{
		NotifyCall(call.callNo, VOICE_STATE_RINGING, NULL, name, number);
	}
	else if (!(call.state & IAXC_CALL_STATE_COMPLETE))
	{
		NotifyCall(call.callNo, VOICE_STATE_CALLING, NULL, name, number);
	}
	else 
	{
		if (call.callNo == iax.selected_call())
			NotifyCall(call.callNo, VOICE_STATE_TALKING, NULL, name, number);
		else
			NotifyCall(call.callNo, VOICE_STATE_ON_HOLD, NULL, name, number);
	}

	return 0;
}


int IAXProto::netstats_callback(iaxc_ev_netstats &netstats)
{
	Trace(_T("netstats"));
	return 0;
}


int IAXProto::url_callback(iaxc_ev_url &url)
{
	Trace(_T("URL"));
	return 0;
}


int IAXProto::registration_callback(iaxc_ev_registration &reg)
{
	Trace(_T("registration_callback %d : %d"), reg.id, reg.reply);

	if (reg.id != reg_id)
		return 0;

	switch(reg.reply)
	{
		case IAXC_REGISTRATION_REPLY_ACK:
		{
			BroadcastStatus(m_iDesiredStatus > ID_STATUS_OFFLINE ? m_iDesiredStatus : ID_STATUS_ONLINE);

			if (reg.msgcount >= 65535)
				reg.msgcount = -1;

			int messages = max(0, reg.msgcount);

			if (messages != voiceMessages)
			{
				if (messages > 0) 
					Info(_T("You have %d voicemail message(s)"), reg.msgcount);

				voiceMessages = messages;
				SendBroadcast(NULL, ACKTYPE_EMAIL, ACKRESULT_STATUS, NULL, 0);
			}

			return 1;
		}
		case IAXC_REGISTRATION_REPLY_REJ:
		{
			Error(_T("Registration rejected"));
			Disconnect();

			return 1;
		}
		case IAXC_REGISTRATION_REPLY_TIMEOUT:
		{
			Error(_T("Registration timeout"));
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

		std::vector<iaxc_event> events(proto->events);
		proto->events.clear();

		LeaveCriticalSection(&proto->cs);

		for(unsigned int i = 0; i < events.size(); ++i)
			proto->iaxc_callback(events[i]);
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


INT_PTR __cdecl IAXProto::OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam ) 
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


INT_PTR __cdecl IAXProto::OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	if (!ServiceExists(MS_VOICESERVICE_REGISTER))
	{
		Error(_T("IAX needs Voice Service plugin to work!"));
		return 1;
	}
	
	TCHAR buffer[MAX_PATH]; 
	mir_sntprintf(buffer, MAX_REGS(buffer), TranslateT("%s plugin connections"), m_tszUserName);
		
	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.flags = /* NUF_INCOMING | NUF_OUTGOING | */ NUF_NOOPTIONS | NUF_TCHAR;
	nl_user.szSettingsModule = m_szModuleName;
	nl_user.ptszDescriptiveName = buffer;
	hNetlibUser = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);

	iax.set_event_callback(&static_iaxc_callback, this); 
	
	// TODO Handle network out port
	iax.set_preferred_source_udp_port(-1);
	if (iax.initialize(NUM_LINES))
	{
		Error(_T("Failed to initialize iaxclient lib"));
		return 1;
	}

	iax.set_formats(IAXC_FORMAT_SPEEX,
					 IAXC_FORMAT_ULAW|IAXC_FORMAT_ALAW|IAXC_FORMAT_GSM|IAXC_FORMAT_SPEEX|IAXC_FORMAT_ILBC);
	iax.set_speex_settings(1,-1,-1,0,8000,3);
	iax.set_silence_threshold(-99);

	if (iax.start_processing_thread())
	{
		Error(_T("Failed to initialize IAX threads"));
		return 1;
	}

	return 0;
}


INT_PTR __cdecl IAXProto::OnOptionsInit(WPARAM wParam, LPARAM lParam)
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


INT_PTR __cdecl IAXProto::OnPreShutdown(WPARAM wParam, LPARAM lParam)
{
	iax.stop_processing_thread();

	iax.shutdown();

	return 0;
}


INT_PTR __cdecl IAXProto::GetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;

	return voiceMessages;
}


void IAXProto::NotifyCall(int callNo, int state, HANDLE hContact, TCHAR *name, TCHAR *number)
{
	Trace(_T("NotifyCall %d -> %d"), callNo, state);

	char tmp[16];

	VOICE_CALL vc = {0};
	vc.cbSize = sizeof(vc);
	vc.moduleName = m_szModuleName;
	vc.id = itoa(callNo, tmp, 10);
	vc.flags = VOICE_TCHAR;
	vc.hContact = hContact;
	vc.ptszName = name;
	vc.ptszNumber = number;
	vc.state = state;

	NotifyEventHooks(hCallStateEvent, (WPARAM) &vc, 0);
}


static int GetDevice(const iaxc_audio_device *devs, int nDevs, bool out)
{
	DBVARIANT dbv;
	if (DBGetContactSettingString(NULL, "VoiceService", out ? "Output" : "Input", &dbv))
		return -1;

	int ret = -1;

	for(int i = 0; i < nDevs; i++)
	{
		if (devs[i].capabilities & (out ? IAXC_AD_OUTPUT : IAXC_AD_INPUT) 
			&& strcmp(dbv.pszVal, devs[i].name) == 0)
		{
			ret = devs[i].devID;
			break;
		}

	}

	DBFreeVariant(&dbv);
	return ret;
}


void IAXProto::ConfigureDevices()
{
	iaxc_audio_device *devs;
	int nDevs; 
	int input;
	int output; 
	int ring;
	iax.audio_devices_get(&devs, &nDevs, &input, &output, &ring);

	int expectedOutput = GetDevice(devs, nDevs, true);
	if (expectedOutput == -1)
		expectedOutput = output;

	int expectedInput = GetDevice(devs, nDevs, false);
	if (expectedInput == -1)
		expectedInput = input;

	if (input != expectedInput || output != expectedOutput || ring != expectedOutput)
		iax.audio_devices_set(expectedInput, expectedOutput, expectedOutput);


	int expectedBoost = DBGetContactSettingByte(NULL, "VoiceService", "MicBoost", TRUE);
	if (expectedBoost != iax.mic_boost_get())
		iax.mic_boost_set(expectedBoost);


	int filters = iax.get_filters();
	int expectedFilters;
	if (DBGetContactSettingByte(NULL, "VoiceService", "EchoCancelation", TRUE))
		expectedFilters = filters | IAXC_FILTER_ECHO;
	else
		expectedFilters = filters & ~IAXC_FILTER_ECHO;
	if (expectedFilters != filters)
		iax.set_filters(expectedFilters);
}


INT_PTR __cdecl IAXProto::VoiceCaps(WPARAM wParam,LPARAM lParam)
{
	return VOICE_CAPS_VOICE | VOICE_CAPS_CALL_STRING;
}


INT_PTR __cdecl IAXProto::VoiceCall(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE) wParam;
	TCHAR *number = (TCHAR *) lParam;

	if (!VoiceCallStringValid((WPARAM) number, 0))
		return 1;

	ConfigureDevices();

	int callNo = iax.first_free_call();
	if (callNo < 0 || callNo >= NUM_LINES)
	{
		Info(_T("No more slots to make calls. You need to drop some calls."));
		return 2;
	}

	iax.select_call(-1);

	char buff[512];
	mir_snprintf(buff, MAX_REGS(buff), "%s:%s@%s/%s", 
				TcharToUtf8(opts.username).get(), opts.password, 
				TcharToUtf8(opts.host).get(), 
				TcharToUtf8(number).get());

	TCHAR *myName = opts.callerID.name;
	TCHAR *myNumber = opts.callerID.number;
	if (myName[0] == 0 && myNumber[0] == 0)
		myName = opts.username;

	callNo = iax.call_ex(buff, TcharToUtf8(myName), TcharToUtf8(myNumber), FALSE);
	if (callNo < 0 || callNo >= NUM_LINES)
	{
		Error(_T("Error making call (callNo=%d)"), callNo);
		return 3;
	}

	NotifyCall(callNo, VOICE_STATE_CALLING, hContact, NULL, number);

	return 0;
}


INT_PTR __cdecl IAXProto::VoiceAnswerCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	int callNo = atoi(id);
	if (callNo < 0 || callNo >= NUM_LINES)
		return 2;

	ConfigureDevices();

	if (iax.select_call(callNo) < 0)
		return 3;

	return 0;
}


INT_PTR __cdecl IAXProto::VoiceDropCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	int callNo = atoi(id);
	if (callNo < 0 || callNo >= NUM_LINES)
		return 2;

	if (iax.get_call_state(callNo) & IAXC_CALL_STATE_RINGING)
		iax.reject_call_number(callNo);
	else
		iax.dump_call_number(callNo);

	return 0;
}


INT_PTR __cdecl IAXProto::VoiceHoldCall(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	if (id == NULL || id[0] == 0)
		return 1;

	int callNo = atoi(id);
	if (callNo < 0 || callNo >= NUM_LINES)
		return 2;

	if (callNo != iax.selected_call())
		return 3;

	iax.select_call(-1);
	NotifyCall(callNo, VOICE_STATE_ON_HOLD);
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


INT_PTR __cdecl IAXProto::VoiceSendDTMF(WPARAM wParam, LPARAM lParam)
{
	char *id = (char *) wParam;
	TCHAR c = (TCHAR) lParam;
	if (id == NULL || id[0] == 0 || c == 0)
		return 1;

	int callNo = atoi(id);
	if (callNo < 0 || callNo >= NUM_LINES)
		return 2;

	if (callNo != iax.selected_call())
		return 3;

	if (c >= _T('a') && c <= _T('d'))
		c += _T('A') - _T('a');

	if (!IsValidDTMF(c))
		return 4;

	iax.send_dtmf((char) c);
	
	return 0;
}


INT_PTR __cdecl IAXProto::VoiceCallStringValid(WPARAM wParam, LPARAM lParam)
{
	TCHAR *number = (TCHAR *) wParam;

	if (m_iStatus <= ID_STATUS_OFFLINE)
		return 0;
	
	if (number == NULL || number[0] == 0 || lstrlen(number) >= IAXC_EVENT_BUFSIZ)
		return 0;

	return 1;
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
	}

	if (proto == NULL)
		return FALSE;

	return SaveOptsDlgProc(proto->accountManagerCtrls, MAX_REGS(proto->accountManagerCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);
}


static INT_PTR CALLBACK DlgOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
	}

	if (proto == NULL)
		return FALSE;

	return SaveOptsDlgProc(proto->optionsCtrls, MAX_REGS(proto->optionsCtrls), 
							proto->m_szModuleName, hwnd, msg, wParam, lParam);
}


