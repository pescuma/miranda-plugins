/* 
Copyright (C) 2010 Ricardo Pescuma Domenecci

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


#ifndef __M_SIP_H__
# define __M_SIP_H__


// state is a VOICE_STATE_*
typedef void (*SIPClientCallback)(void *param, int callId, int state, const TCHAR *name, const TCHAR *uri);

struct SIP_REGISTRATION
{
	int cbSize;				
	const char *name;		// Internal name of client
	const TCHAR *username;
	int udp_port;			// UDP port to be used: 0 means random, -1 means don't want UDP
	int tcp_port;			// UDP port to be used: 0 means TCP, -1 means don't want TCP
	int tls_port;			// UDP port to be used: 0 means TLS, -1 means don't want TLS

	HANDLE hNetlib;			// To be used for logs. Can be 0

	SIPClientCallback callback;
	void *callback_param;
};


struct SIP_CLIENT
{
	void *data; // Do not touch
	const TCHAR *username;
	const TCHAR *host;
	const int udp_port;
	const int tcp_port;
	const int tls_port;

	// @param protocol 1 UDP, 2 TCP, 3 TLS
	// @return callId or <0 on error
	int (*Call)(SIP_CLIENT *sip, const TCHAR *username, const TCHAR *host, int port, int protocol);

	// @return 0 on success
	int (*DropCall)(SIP_CLIENT *sip, int callId);

	// @return 0 on success
	int (*HoldCall)(SIP_CLIENT *sip, int callId);

	// @return 0 on success
	int (*AnswerCall)(SIP_CLIENT *sip, int callId);

	// @return 0 on success
	int (*SendDTMF)(SIP_CLIENT *sip, int callId, TCHAR dtmf);
};


/*
Register a SIP client, allowing it to make calls

wParam = SIP_REGISTRATION *
lParam = 0
return SIP_CLIENT * or NULL on error
*/ 
#define MS_SIP_REGISTER "SIP/Client/Register"


/*
Unregister a SIP client and free the internal structures.

wParam = SIP_CLIENT *
lParam = 0
return 0 on success
*/ 
#define MS_SIP_UNREGISTER "SIP/Client/Unregister"









#endif // __M_SIP_H__
