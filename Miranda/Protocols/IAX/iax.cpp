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


// Prototypes ///////////////////////////////////////////////////////////////////////////


PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
#ifdef UNICODE
	"IAX protocol (Unicode)",
#else
	"IAX protocol (Ansi)",
#endif
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Provides support for Inter-Asterisk eXchange (IAX) protocol",
	"Ricardo Pescuma Domenecci, sje",
	"pescuma@miranda-im.org",
	"© 2009 Ricardo Pescuma Domenecci, sje, IAXClient",
	"http://pescuma.org/miranda/iax",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0x706e2aca, 0x4310, 0x49c5, { 0x93, 0x3c, 0xea, 0xa1, 0xd6, 0x52, 0xa2, 0x6f } } // {706E2ACA-4310-49c5-933C-EAA1D652A26F}
#else
	{ 0x6d6a97da, 0x86ed, 0x422e, { 0x82, 0xe8, 0xd2, 0x6, 0x32, 0xb0, 0x7, 0x51 } } // {6D6A97DA-86ED-422e-82E8-D20632B00751}
#endif
};


HINSTANCE hInst;
PLUGINLINK *pluginLink;
MM_INTERFACE mmi;
UTF8_INTERFACE utfi;
LIST_INTERFACE li;

std::vector<HANDLE> hHooks;
std::vector<HANDLE> hServices;

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);

// IAXProto /////////////////////////////////////////////////////////////////////////////

class IAXProto;
typedef INT_PTR (IAXProto::*IAXServiceFunc)(WPARAM, LPARAM);
typedef INT_PTR (IAXProto::*IAXServiceFuncParam)(WPARAM, LPARAM, LPARAM);

class IAXProto : public PROTO_INTERFACE
{
private:
	HANDLE hNetlibUser;

public:
	IAXProto(const char *aProtoName, const TCHAR *aUserName)
	{
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
	}

	~IAXProto()
	{
		Netlib_CloseHandle(hNetlibUser);

		mir_free(m_tszUserName);
		mir_free(m_szProtoName);
		mir_free(m_szModuleName);
	}

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

	virtual	DWORD_PTR __cdecl GetCaps( int type, HANDLE hContact = NULL ) 
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

	virtual	HICON     __cdecl GetIcon( int iconIndex ) { return 0; }
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
	virtual	int       __cdecl SetStatus( int iNewStatus ) 
	{
		if (m_iDesiredStatus == iNewStatus) 
			return 0;
		m_iDesiredStatus = iNewStatus;

		if (m_iDesiredStatus == ID_STATUS_OFFLINE)
		{
			BroadcastStatus(ID_STATUS_OFFLINE);
		}
		else if (m_iStatus == ID_STATUS_OFFLINE)
		{
			BroadcastStatus(ID_STATUS_CONNECTING);
		}
		else if (m_iStatus > ID_STATUS_OFFLINE)
		{
			BroadcastStatus(m_iDesiredStatus);
		}

		return 0; 
	}

	virtual	HANDLE    __cdecl GetAwayMsg( HANDLE hContact ) { return 0; }
	virtual	int       __cdecl RecvAwayMsg( HANDLE hContact, int mode, PROTORECVEVENT* evt ) { return 1; }
	virtual	int       __cdecl SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg ) { return 1; }
	virtual	int       __cdecl SetAwayMsg( int iStatus, const char* msg ) { return 1; }

	virtual	int       __cdecl UserIsTyping( HANDLE hContact, int type ) { return 0; }

	virtual	int       __cdecl OnEvent( PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam ) { return 1; }



private:
	void BroadcastStatus(int newStatus)
	{
		int oldStatus = m_iStatus;
		m_iStatus = newStatus;
		SendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
	}

	void CreateProtoService(const char* szService, IAXServiceFunc serviceProc)
	{
		char str[MAXMODULELABELLENGTH];

		mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
		::CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
	}

	void CreateProtoService(const char* szService, IAXServiceFuncParam serviceProc, LPARAM lParam)
	{
		char str[MAXMODULELABELLENGTH];
		mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
		::CreateServiceFunctionObjParam(str, (MIRANDASERVICEOBJPARAM)*(void**)&serviceProc, this, lParam);
	}

	HANDLE CreateProtoEvent(const char* szService)
	{
		char str[MAXMODULELABELLENGTH];
		mir_snprintf(str, sizeof(str), "%s%s", m_szModuleName, szService);
		return ::CreateHookableEvent(str);
	}

	int SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
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


};

OBJLIST<IAXProto> instances(1);


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


static const MUUID interfaces[] = { MIID_PROTOCOL, MIID_LAST };
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}


static IAXProto *IAXProtoInit(const char* pszProtoName, const TCHAR* tszUserName)
{
	IAXProto *proto = new IAXProto(pszProtoName, tszUserName);
	instances.insert(proto);
	return proto;
}


static int IAXProtoUninit(IAXProto *proto)
{
	instances.remove(proto);
	return 0;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	CHECK_VERSION("IAX")

	// TODO Assert results here
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);
	mir_getLI(&li);
	
	hHooks.push_back( HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded) );
	hHooks.push_back( HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown) );

	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize = sizeof(pd);
	pd.szName = MODULE_NAME;
	pd.fnInit = (pfnInitProto) IAXProtoInit;
	pd.fnUninit = (pfnUninitProto) IAXProtoUninit;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) &pd);

	return 0;
}


extern "C" int __declspec(dllexport) Unload(void) 
{
	return 0;
}


// Called when all the modules are loaded
int ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	// Add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME, 0);

    // Updater plugin support
    if(ServiceExists(MS_UPDATE_REGISTER))
	{
		Update upd = {0};
		char szCurrentVersion[30];

		upd.cbSize = sizeof(upd);
		upd.szComponentName = pluginInfo.shortName;

		upd.szUpdateURL = UPDATER_AUTOREGISTER;

		upd.szBetaVersionURL = "http://pescuma.org/miranda/iax_version.txt";
		upd.szBetaChangelogURL = "http://pescuma.org/miranda/iax#Changelog";
		upd.pbBetaVersionPrefix = (BYTE *)"IAX protocol ";
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);
#ifdef UNICODE
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/iaxW.zip";
#else
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/iax.zip";
#endif

		upd.pbVersion = (BYTE *)CreateVersionStringPlugin((PLUGININFO*) &pluginInfo, szCurrentVersion);
		upd.cpbVersion = strlen((char *)upd.pbVersion);

        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	}

	return 0;
}


int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	for(unsigned int i = 0; i < hHooks.size(); ++i)
		UnhookEvent(hHooks[i]);

	return 0;
}

