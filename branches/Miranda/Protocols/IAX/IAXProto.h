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

#ifndef __IAXPROTO_H__
#define __IAXPROTO_H__


class IAXProto;
typedef INT_PTR (__cdecl IAXProto::*IAXServiceFunc)(WPARAM, LPARAM);
typedef INT_PTR (__cdecl IAXProto::*IAXServiceFuncParam)(WPARAM, LPARAM, LPARAM);
typedef int (__cdecl IAXProto::*IAXEventFunc)(WPARAM, LPARAM);

class IAXProto : public PROTO_INTERFACE
{
private:
	HMEMORYMODULE iaxclient;
	IAXCLIENT iax;
	HANDLE hNetlibUser;
	HANDLE hCallStateEvent;
	int reg_id;
	int voiceMessages;

	struct {
		TCHAR host[256];
		int port;
		TCHAR username[16];
		char password[16];
		BYTE savePassword;

		struct {
			TCHAR name[128];
			TCHAR number[128];
		} callerID;
	} opts;

public:
	CRITICAL_SECTION cs;
	std::vector<iaxc_event> events;
	OptPageControl accountManagerCtrls[5];
	OptPageControl optionsCtrls[7];

	IAXProto(HMEMORYMODULE iaxclient, const char *aProtoName, const TCHAR *aUserName);
	virtual ~IAXProto();

	virtual	HANDLE   __cdecl AddToList( int flags, PROTOSEARCHRESULT* psr ) { return 0; }
	virtual	HANDLE   __cdecl AddToListByEvent( int flags, int iContact, HANDLE hDbEvent ) { return 0; }

	virtual	int      __cdecl Authorize( HANDLE hDbEvent ) { return 1; }
	virtual	int      __cdecl AuthDeny( HANDLE hDbEvent, const char* szReason ) { return 1; }
	virtual	int      __cdecl AuthRecv( HANDLE hContact, PROTORECVEVENT* ) { return 1; }
	virtual	int      __cdecl AuthRequest( HANDLE hContact, const char* szMessage ) { return 1; }

	virtual	HANDLE   __cdecl ChangeInfo( int iInfoType, void* pInfoData ) { return 0; }

	virtual	HANDLE   __cdecl FileAllow( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szPath ) { return 0; }
	virtual	int      __cdecl FileCancel( HANDLE hContact, HANDLE hTransfer ) { return 1; }
	virtual	int      __cdecl FileDeny( HANDLE hContact, HANDLE hTransfer, const PROTOCHAR* szReason ) { return 1; }
	virtual	int      __cdecl FileResume( HANDLE hTransfer, int* action, const PROTOCHAR** szFilename ) { return 1; }

	virtual	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL );

	virtual	HICON     __cdecl GetIcon( int iconIndex );
	virtual	int       __cdecl GetInfo( HANDLE hContact, int infoType ) { return 1; }

	virtual	HANDLE    __cdecl SearchBasic( const char* id ) { return 0; }
	virtual	HANDLE    __cdecl SearchByEmail( const char* email ) { return 0; }
	virtual	HANDLE    __cdecl SearchByName( const char* nick, const char* firstName, const char* lastName ) { return 0; }
	virtual	HWND      __cdecl SearchAdvanced( HWND owner ) { return 0; }
	virtual	HWND      __cdecl CreateExtendedSearchUI( HWND owner ) { return 0; }

	virtual	int       __cdecl RecvContacts( HANDLE hContact, PROTORECVEVENT* ) { return 1; }
	virtual	int       __cdecl RecvFile( HANDLE hContact, PROTOFILEEVENT* ) { return 1; }
	virtual	int       __cdecl RecvMsg( HANDLE hContact, PROTORECVEVENT* ) { return 1; }
	virtual	int       __cdecl RecvUrl( HANDLE hContact, PROTORECVEVENT* ) { return 1; }

	virtual	int       __cdecl SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList ) { return 1; }
	virtual	HANDLE    __cdecl SendFile( HANDLE hContact, const PROTOCHAR* szDescription, PROTOCHAR** ppszFiles ) { return 0; }
	virtual	int       __cdecl SendMsg( HANDLE hContact, int flags, const char* msg ) { return 1; }
	virtual	int       __cdecl SendUrl( HANDLE hContact, int flags, const char* url ) { return 1; }

	virtual	int       __cdecl SetApparentMode( HANDLE hContact, int mode ) { return 1; }

	virtual	int       __cdecl SetStatus( int iNewStatus );

	virtual	HANDLE    __cdecl GetAwayMsg( HANDLE hContact ) { return 0; }
	virtual	int       __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt ) { return 1; }
	virtual	int       __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg ) { return 1; }
	virtual	int       __cdecl SetAwayMsg( int iStatus, const char* msg ) { return 1; }

	virtual	int       __cdecl UserIsTyping( HANDLE hContact, int type ) { return 0; }

	virtual	int       __cdecl OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam );

	int iaxc_callback(iaxc_event &e);

private:
	void BroadcastStatus(int newStatus);
	void CreateProtoService(const char* szService, IAXServiceFunc serviceProc);
	void CreateProtoService(const char* szService, IAXServiceFuncParam serviceProc, LPARAM lParam);
	HANDLE CreateProtoEvent(const char* szService);
	void HookProtoEvent(const char* szEvent, IAXEventFunc pFunc);
	int SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam);

	INT_PTR __cdecl OnModulesLoaded(WPARAM wParam, LPARAM lParam);
	INT_PTR __cdecl OnOptionsInit(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl OnPreShutdown(WPARAM wParam,LPARAM lParam);

	void Trace(const TCHAR *fmt, ...);
	void Info(const TCHAR *fmt, ...);
	void Error(const TCHAR *fmt, ...);
	void ShowMessage(int type, const TCHAR *fmt, va_list args);

	void Disconnect();
	INT_PTR __cdecl CreateAccMgrUI(WPARAM wParam, LPARAM lParam);
	INT_PTR __cdecl GetUnreadEmailCount(WPARAM wParam, LPARAM lParam);

	int levels_callback(iaxc_ev_levels &levels);
	int text_callback(iaxc_ev_text &text);
	int state_callback(iaxc_ev_call_state &call);
	int netstats_callback(iaxc_ev_netstats &netstats);
	int url_callback(iaxc_ev_url &url);
	int registration_callback(iaxc_ev_registration &reg);

	void ConfigureDevices();


	// Voice services
	void NotifyCall(int callNo, int state, HANDLE hContact = NULL, TCHAR *name = NULL, TCHAR *number = NULL);
	INT_PTR __cdecl VoiceCaps(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceCall(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceAnswerCall(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceDropCall(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceHoldCall(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceSendDTMF(WPARAM wParam,LPARAM lParam);
	INT_PTR __cdecl VoiceCallStringValid(WPARAM wParam,LPARAM lParam);
};




#endif // __IAXPROTO_H__
