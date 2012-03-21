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
	PLUGIN_MAKE_VERSION(0,2,0,0),
	"Provides support for Inter-Asterisk eXchange (IAX) protocol",
	"Ricardo Pescuma Domenecci",
	"pescuma@miranda-im.org",
	"© 2009-2010 Ricardo Pescuma Domenecci",
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

void * iaxclientDllCode = NULL;

std::vector<HANDLE> hHooks;

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);


static int sttCompareProtocols(const IAXProto *p1, const IAXProto *p2)
{
	return strcmp(p1->m_szModuleName, p2->m_szModuleName);
}
OBJLIST<IAXProto> instances(1, &sttCompareProtocols);




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
	HMEMORYMODULE iaxclient = MemoryLoadLibrary((const char *) iaxclientDllCode);
	if (iaxclient == NULL)
	{
		MessageBox(NULL, TranslateT("Error loading iaxclient dll"), TranslateT("IAX"), MB_OK | MB_ICONERROR);
		return NULL;
	}
	
	IAXProto *proto = new IAXProto(iaxclient, pszProtoName, tszUserName);
	instances.insert(proto);
	return proto;
}


static int IAXProtoUninit(IAXProto *proto)
{
	instances.remove(proto);
	return 0;
}


static void * LoadDllFromResource(LPCTSTR lpName)
{
	HRSRC hRes = FindResource(hInst, lpName, RT_RCDATA);
	if (hRes == NULL) 
		return NULL;
	
	HGLOBAL hResLoad = LoadResource(hInst, hRes);
	if (hResLoad == NULL) 
		return NULL;
	
	return LockResource(hResLoad);
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	CHECK_VERSION("IAX");

	iaxclientDllCode = LoadDllFromResource(MAKEINTRESOURCE(IDR_IAXCLIENT_DLL));
	if (iaxclientDllCode == NULL)
	{
		MessageBox(NULL, TranslateT("Could not load iaxclient module"), TranslateT("IAX"), MB_OK | MB_ICONERROR);
		return -1;
	}

	HMEMORYMODULE iaxclient = MemoryLoadLibrary((const char *) iaxclientDllCode);
	if (iaxclient == NULL)
	{
		MessageBox(NULL, TranslateT("Error loading iaxclient dll"), TranslateT("IAX"), MB_OK | MB_ICONERROR);
		return -1;
	}
	MemoryFreeLibrary(iaxclient);

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
	if (!ServiceExists(MS_VOICESERVICE_REGISTER))
		MessageBox(NULL, TranslateT("IAX needs Voice Service plugin to work!"), _T("IAX"), MB_OK | MB_ICONERROR);
	
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
