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

static int sttCompareClients(const SIPClient *p1, const SIPClient *p2)
{
	return p1 < p2;
}
OBJLIST<SIPClient> clients(1, &sttCompareClients);


SIPClient::SIPClient(SIP_REGISTRATION *reg)
{
	hasToDestroy = false;
	udp.transport_id = -1;
	udp.acc_id = -1;
	udp.port = 0;
	tcp.transport_id = -1;
	tcp.acc_id = -1;
	tcp.port = 0;
	tls.transport_id = -1;
	tls.acc_id = -1;
	tls.port = 0;

	hNetlibUser = reg->hNetlib;
	lstrcpynA(name, reg->name, MAX_REGS(name));

	callback = reg->callback;
	callback_param = reg->callback_param;

	InitializeCriticalSection(&cs);
}


SIPClient::~SIPClient()
{
	DeleteCriticalSection(&cs);
}


static void CALLBACK ProcessEvents(void *param)
{
	for(int i = 0; i < clients.getCount(); ++i)
	{
		SIPClient *proto = &clients[i];

		EnterCriticalSection(&proto->cs);

		std::vector<SIPEvent> events(proto->events);
		proto->events.clear();

		LeaveCriticalSection(&proto->cs);

		for(unsigned int i = 0; i < events.size(); ++i)
		{
			SIPEvent &ev = events[i];
			switch(ev.type)
			{
				case SIPEvent::incoming_call:
					proto->on_incoming_call(ev.call_id);
					break;
				case SIPEvent::call_state:
					proto->on_call_state(ev.call_id, ev.call_info);
					break;
				case SIPEvent::call_media_state:
					proto->on_call_media_state(ev.call_id);
					break;
			}
			mir_free(ev.from);
			mir_free(ev.text);
			mir_free(ev.mime);
			delete ev.messageData;
		}
	}
}


static void static_on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
		return;

	SIPClient *proto = (SIPClient *) pjsua_acc_get_user_data(acc_id);
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

	SIPClient *proto = (SIPClient *) pjsua_acc_get_user_data(info.acc_id);
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

	SIPClient *proto = (SIPClient *) pjsua_acc_get_user_data(info.acc_id);
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


static void static_on_log(int level, const char *data, int len)
{
	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "Level %d : %*s", level, len, data);
	OutputDebugStringA(tmp);
}


#define TransportName(_T_) SipToTchar(pj_cstr(pjsip_transport_get_type_name(_T_))).get()

void SIPClient::RegisterTransport(pjsip_transport_type_e type, int port, ta *ta)
{
	ta->transport_id = -1;
	ta->acc_id = -1;
	ta->port = 0;

	if (port < 0)
		return;

	pjsua_transport_config cfg;
	pjsua_transport_config_default(&cfg);
	cfg.port = port;
	
	pj_status_t status = pjsua_transport_create(type, &cfg, &ta->transport_id);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error creating %s transport"), TransportName(type));
		return;
	}

	pjsua_transport_info info;
	status = pjsua_transport_get_info(ta->transport_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error getting %s info"), TransportName(type));
		pjsua_transport_close(ta->transport_id, PJ_TRUE);
		ta->transport_id = -1;
		return;
	}

	status = pjsua_acc_add_local(ta->transport_id, PJ_TRUE, &ta->acc_id);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error adding %s account"), TransportName(type));
		pjsua_transport_close(ta->transport_id, PJ_TRUE);
		ta->transport_id = -1;
		return;
	}

	pjsua_acc_set_user_data(ta->acc_id, this);

	lstrcpyn(host, SipToTchar(info.local_name.host), MAX_REGS(host));
	ta->port = info.local_name.port;
}


int SIPClient::Connect(int udp_port, int tcp_port, int tls_port)
{
	Trace(_T("Connecting..."));

	{
		pj_status_t status = pjsua_create();
		if (status != PJ_SUCCESS) 
		{
			Error(status, _T("Error creating pjsua"));
			return 1;
		}
		hasToDestroy = true;
	}

	{
		pjsua_config cfg;
		pjsua_config_default(&cfg);
#ifndef _DEBUG
		cfg.use_srtp = PJMEDIA_SRTP_OPTIONAL;
#endif
		cfg.cb.on_incoming_call = &static_on_incoming_call;
		cfg.cb.on_call_media_state = &static_on_call_media_state;
		cfg.cb.on_call_state = &static_on_call_state;

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
			return 1;
		}
	}

	{
		RegisterTransport(PJSIP_TRANSPORT_UDP, udp_port, &udp);
		RegisterTransport(PJSIP_TRANSPORT_TCP, tcp_port, &tcp);
		RegisterTransport(PJSIP_TRANSPORT_TLS, tls_port, &tls);

		if (udp.port <= 0 && tcp.port <= 0 && tls.port <= 0)
			return 1;
	}

	{
		pj_status_t status = pjsua_start();
		if (status != PJ_SUCCESS)
		{
			Error(status, _T("Error starting pjsua"));
			return 1;
		}
	}

	return 0;
}


#define MESSAGE_TYPE_TRACE 0
#define MESSAGE_TYPE_INFO 1
#define MESSAGE_TYPE_ERROR 2


void SIPClient::Trace(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_TRACE, fmt, args);

	va_end(args);
}


void SIPClient::Info(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_INFO, TranslateTS(fmt), args);

	va_end(args);
}


void SIPClient::Error(TCHAR *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	ShowMessage(MESSAGE_TYPE_ERROR, TranslateTS(fmt), args);

	va_end(args);
}


void SIPClient::Error(pj_status_t status, TCHAR *fmt, ...)
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


void SIPClient::ShowMessage(int type, TCHAR *fmt, va_list args) 
{
	TCHAR buff[1024];
	if (type == MESSAGE_TYPE_TRACE)
		lstrcpy(buff, _T("SIP: TRACE: "));
	else if (type == MESSAGE_TYPE_INFO)
		lstrcpy(buff, _T("SIP: INFO : "));
	else
		lstrcpy(buff, _T("SIP: ERROR: "));

	mir_vsntprintf(&buff[12], MAX_REGS(buff)-12, fmt, args);

#ifdef _DEBUG
	OutputDebugString(buff);
	OutputDebugString(_T("\n"));
#endif

	if (hNetlibUser)
		CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) TcharToChar(buff).get());
}


void SIPClient::Disconnect()
{
	Trace(_T("Disconnecting..."));

	EnterCriticalSection(&cs);
	events.clear();
	LeaveCriticalSection(&cs);

#define CLOSE(_TA_)											\
	if (_TA_.acc_id >= 0)									\
	{														\
		pjsua_acc_del(_TA_.acc_id);							\
		_TA_.acc_id = -1;									\
	}														\
	if (_TA_.transport_id >= 0)								\
	{														\
		pjsua_transport_close(_TA_.transport_id, PJ_TRUE);	\
		_TA_.transport_id = -1;								\
	}

	CLOSE(udp)
	CLOSE(tcp)
	CLOSE(tls)

	if (hasToDestroy)
	{
		pjsua_destroy();
		hasToDestroy = false;
	}
}


void SIPClient::NotifyCall(pjsua_call_id call_id, int state, const TCHAR *uri)
{
	Trace(_T("NotifyCall %d -> %d"), call_id, state);

	if (callback == NULL)
		return;

	if (state == VOICE_STATE_ENDED || state == VOICE_STATE_BUSY)
	{
		// Can't get call info anymore
		callback(callback_param, (int) call_id, state, 0, NULL);
		return;
	}

	pjsua_call_info info;
	pj_status_t status = pjsua_call_get_info(call_id, &info);
	if (status != PJ_SUCCESS)
	{
		Error(status, _T("Error obtaining call info"));
		callback(callback_param, (int) call_id, state, 0, NULL);
		return;
	}

	TCHAR host_port[1024];
	if (uri != NULL)
		lstrcpyn(host_port, uri, MAX_REGS(host_port));
	else
		CleanupURI(host_port, MAX_REGS(host_port), SipToTchar(info.remote_contact));

	callback(callback_param, (int) call_id, state, VOICE_UNICODE | (info.acc_id == tls.acc_id ? VOICE_SECURE : 0), host_port);
}


void SIPClient::on_incoming_call(pjsua_call_id call_id)
{
	Trace(_T("on_incoming_call: %d"), call_id);

	NotifyCall(call_id, VOICE_STATE_RINGING);
}


void SIPClient::on_call_state(pjsua_call_id call_id, const pjsua_call_info &info)
{
	Trace(_T("on_call_state: %d"), call_id);
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


bool SIPClient::on_call_media_state_sync(pjsua_call_id call_id, const pjsua_call_info &info)
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


void SIPClient::on_call_media_state(pjsua_call_id call_id)
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


void SIPClient::ConfigureDevices()
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


void SIPClient::CleanupURI(TCHAR *out, int outSize, const TCHAR *url)
{
	lstrcpyn(out, url, outSize);

	RemoveLtGt(out);

	if (_tcsnicmp(_T("sip:"), out, 4) == 0)
		lstrcpyn(out, &out[4], outSize);
}


void SIPClient::BuildURI(TCHAR *out, int outSize, const TCHAR *host, int port, int protocol)
{
	if (protocol == PJSIP_TRANSPORT_UDP)
		mir_sntprintf(out, outSize, _T("<sip:%s:%d>"), host, port);
	else
		mir_sntprintf(out, outSize, _T("<sip:%s:%d;transport=%s>"), host, port,
					  TransportName((pjsip_transport_type_e) protocol));
}

pjsua_call_id SIPClient::Call(const TCHAR *host, int port, int protocol)
{
	pjsua_acc_id acc_id;
	switch(protocol)
	{
		case PJSIP_TRANSPORT_UDP: acc_id = udp.acc_id; break;
		case PJSIP_TRANSPORT_TCP: acc_id = tcp.acc_id; break;
		case PJSIP_TRANSPORT_TLS: acc_id = tls.acc_id; break;
		default: return -1;
	}
	if (acc_id < 0)
		return -1;

	TCHAR uri[1024];
	BuildURI(uri, MAX_REGS(uri), host, port, protocol);

	pjsua_call_id call_id;
	pj_str_t ret;
	pj_status_t status = pjsua_call_make_call(acc_id, pj_cstr(&ret, TcharToSip(uri)), 0, NULL, NULL, &call_id);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error making call"));
		return -1;
	}

	mir_sntprintf(uri, MAX_REGS(uri), _T("%s:%d"), host, port);
	NotifyCall(call_id, VOICE_STATE_CALLING, uri);

	return call_id;
}

int SIPClient::DropCall(pjsua_call_id call_id)
{
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

int SIPClient::HoldCall(pjsua_call_id call_id)
{
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	pj_status_t status = pjsua_call_set_hold(call_id, NULL);
	if (status != PJ_SUCCESS) 
	{
		Error(status, _T("Error holding call"));
		return -1;
	}

	return 0;
}

int SIPClient::AnswerCall(pjsua_call_id call_id)
{
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

int SIPClient::SendDTMF(pjsua_call_id call_id, TCHAR dtmf)
{
	if (call_id < 0 || call_id >= (int) pjsua_call_get_max_count())
		return 2;

	if (dtmf >= _T('a') && dtmf <= _T('d'))
		dtmf += _T('A') - _T('a');

	if (!IsValidDTMF(dtmf))
		return 3;

	TCHAR tmp[2];
	tmp[0] = dtmf;
	tmp[1] = 0;

	pj_str_t ret;
	pjsua_call_dial_dtmf(call_id, pj_cstr(&ret, TcharToSip(tmp)));

	return 0;
}
