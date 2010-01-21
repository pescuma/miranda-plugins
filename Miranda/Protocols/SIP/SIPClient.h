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

#ifndef __SIPCLIENT_H__
#define __SIPCLIENT_H__


class SIPClient
{
private:
	HANDLE hNetlibUser;
	bool hasToDestroy;

public:
	struct ta {
		pjsua_transport_id transport_id;
		pjsua_acc_id acc_id;
		TCHAR host[256];
		int port;
	};

	ta udp;
	ta tcp;
	ta tls;

	SIPClientCallback callback;
	void *callback_param;

	char name[512];

	CRITICAL_SECTION cs;
	std::vector<SIPEvent> events;

	SIPClient(SIP_REGISTRATION *reg);
	virtual ~SIPClient();

	void on_incoming_call(pjsua_call_id call_id);
	void on_call_state(pjsua_call_id call_id, const pjsua_call_info &info);
	bool on_call_media_state_sync(pjsua_call_id call_id, const pjsua_call_info &info);
	void on_call_media_state(pjsua_call_id call_id);

	pjsua_call_id Call(const TCHAR *host, int port, int protocol);
	int DropCall(pjsua_call_id call_id);
	int HoldCall(pjsua_call_id call_id);
	int AnswerCall(pjsua_call_id call_id);
	int SendDTMF(pjsua_call_id call_id, TCHAR dtmf);

	int Connect(SIP_REGISTRATION *reg);
	void Disconnect();

private:
	void Trace(TCHAR *fmt, ...);
	void Info(TCHAR *fmt, ...);
	void Error(TCHAR *fmt, ...);
	void Error(pj_status_t status, TCHAR *fmt, ...);
	void ShowMessage(int type, TCHAR *fmt, va_list args);

	void RegisterTransport(pjsip_transport_type_e type, int port, ta *ta);

	void ConfigureDevices();
	void BuildURI(TCHAR *out, int outSize, const TCHAR *host, int port, int protocol);
	void CleanupURI(TCHAR *out, int outSize, const TCHAR *url);

	void NotifyCall(pjsua_call_id call_id, int state, const TCHAR *host_port = NULL);
};




#endif // __SIPCLIENT_H__
