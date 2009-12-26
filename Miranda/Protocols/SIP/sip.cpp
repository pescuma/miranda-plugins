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
	"SIP protocol (Unicode)",
#else
	"SIP protocol (Ansi)",
#endif
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Provides support for SIP protocol",
	"Ricardo Pescuma Domenecci",
	"pescuma@miranda-im.org",
	"� 2009 Ricardo Pescuma Domenecci",
	"http://pescuma.org/miranda/sip",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0x9976da3b, 0x8c73, 0x41e3, { 0x9a, 0x22, 0x33, 0xf0, 0xb4, 0xed, 0x74, 0x41 } } // {9976DA3B-8C73-41e3-9A22-33F0B4ED7441}
#else
	{ 0xea7edb84, 0xb87c, 0x4b2d, { 0xbb, 0xce, 0x6f, 0x29, 0xfd, 0x4b, 0xb8, 0x86 } } // {EA7EDB84-B87C-4b2d-BBCE-6F29FD4BB886}
#endif
};


HINSTANCE hInst;
PLUGINLINK *pluginLink;
MM_INTERFACE mmi;
UTF8_INTERFACE utfi;
LIST_INTERFACE li;

std::vector<HANDLE> hHooks;

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);


static int sttCompareProtocols(const SIPProto *p1, const SIPProto *p2)
{
	return strcmp(p1->m_szModuleName, p2->m_szModuleName);
}
OBJLIST<SIPProto> instances(1, &sttCompareProtocols);




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


static SIPProto *SIPProtoInit(const char* pszProtoName, const TCHAR* tszUserName)
{
/*	if (instances.getCount() != 0)
	{
		MessageBox(NULL, TranslateT("Only one instance is allowed.\nI will crash now."), _T("SIP"), MB_OK | MB_ICONERROR);
		return NULL;
	}
*/
	SIPProto *proto = new SIPProto(pszProtoName, tszUserName);
	instances.insert(proto);
	return proto;
}


static int SIPProtoUninit(SIPProto *proto)
{
	instances.remove(proto);
	return 0;
}


static void ShowError(TCHAR *msg, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];
    pj_strerror(status, errmsg, sizeof(errmsg));

	TCHAR err[1024];
	mir_sntprintf(err, MAX_REGS(err), _T("%s: %s"), msg, Utf8ToTchar(errmsg));

	MessageBox(NULL, err, TranslateT("SIP"), MB_OK | MB_ICONERROR);
}


static void static_on_reg_state(pjsua_acc_id acc_id)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	proto->on_reg_state();
}

static void static_on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	SIPProto *proto = (SIPProto *) pjsua_acc_get_user_data(acc_id);
	if (proto == NULL)
		return;

	proto->on_incoming_call(call_id, rdata);
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

	proto->on_call_state(call_id, e);
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

	proto->on_call_media_state(call_id);
}

static void static_on_log(int level, const char *data, int len)
{
	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "Level %d : %*s", level, len, data);
	OutputDebugStringA(tmp);
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	CHECK_VERSION("SIP")

	// TODO Assert results here
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);
	mir_getLI(&li);
	
	pj_status_t status = pjsua_create();
	if (status != PJ_SUCCESS) 
	{
		ShowError(TranslateT("Error creating pjsua"), status);
		return -1;
	}

	pjsua_config cfg;
	pjsua_config_default(&cfg);
	cfg.cb.on_incoming_call = &static_on_incoming_call;
	cfg.cb.on_call_media_state = &static_on_call_media_state;
	cfg.cb.on_call_state = &static_on_call_state;
	cfg.cb.on_reg_state = &static_on_reg_state;
	cfg.stun_srv_cnt = 1;
	cfg.stun_srv[0] = pj_str("stun01.sipphone.com");

	pjsua_logging_config log_cfg;
	pjsua_logging_config_default(&log_cfg);
	log_cfg.console_level = 4;
	log_cfg.cb = &static_on_log;

	status = pjsua_init(&cfg, &log_cfg, NULL);
	if (status != PJ_SUCCESS)
	{
		ShowError(TranslateT("Error initializing pjsua"), status);
		pjsua_destroy();
		return -2;
	}

	status = pjsua_start();
	if (status != PJ_SUCCESS)
	{
		ShowError(TranslateT("Error starting pjsua"), status);
		pjsua_destroy();
		return -3;
	}

	hHooks.push_back( HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded) );
	hHooks.push_back( HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown) );

	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize = sizeof(pd);
	pd.szName = MODULE_NAME;
	pd.fnInit = (pfnInitProto) SIPProtoInit;
	pd.fnUninit = (pfnUninitProto) SIPProtoUninit;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) &pd);

	return 0;
}


extern "C" int __declspec(dllexport) Unload(void) 
{
	pjsua_destroy();

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

		upd.szBetaVersionURL = "http://pescuma.org/miranda/sip_version.txt";
		upd.szBetaChangelogURL = "http://pescuma.org/miranda/sip#Changelog";
		upd.pbBetaVersionPrefix = (BYTE *)"SIP protocol ";
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);
#ifdef UNICODE
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/sipW.zip";
#else
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/sip.zip";
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
