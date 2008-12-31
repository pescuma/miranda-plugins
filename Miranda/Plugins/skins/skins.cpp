/* 
Copyright (C) 2008 Ricardo Pescuma Domenecci

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
	"Skins (Unicode)",
#else
	"Skins",
#endif
	PLUGIN_MAKE_VERSION(0,0,0,1),
	"Skins",
	"Ricardo Pescuma Domenecci",
	"",
	"© 2008 Ricardo Pescuma Domenecci",
	"http://pescuma.org/miranda/skins",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0xde546127, 0x2cdd, 0x48ca, { 0x8a, 0xe5, 0x36, 0x25, 0xe7, 0x24, 0xf2, 0xda } }
#else
	{ 0x2f630a2a, 0xd8f9, 0x4f81, { 0x8a, 0x4e, 0xce, 0x9a, 0x73, 0x5d, 0xf2, 0xe9 } }
#endif
};


HINSTANCE hInst;
PLUGINLINK *pluginLink;

std::vector<HANDLE> hHooks;
std::vector<HANDLE> hServices;

HANDLE hSkinsFolder = NULL;
TCHAR skinsFolder[1024];

char *metacontacts_proto = NULL;
BOOL loaded = FALSE;

LIST_INTERFACE li;
FI_INTERFACE *fei = NULL;

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);



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


static const MUUID interfaces[] = { MIID_SKINS, MIID_LAST };
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	// TODO Assert results here
	init_mir_malloc();
	mir_getLI(&li);

	hHooks.push_back( HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded) );
	hHooks.push_back( HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown) );

	return 0;
}


extern "C" int __declspec(dllexport) Unload(void) 
{
	return 0;
}


// Called when all the modules are loaded
int ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	if (ServiceExists(MS_MC_GETPROTOCOLNAME))
		metacontacts_proto = (char *) CallService(MS_MC_GETPROTOCOLNAME, 0, 0);

	// add our modules to the KnownModules list
	CallService("DBEditorpp/RegisterSingleModule", (WPARAM) MODULE_NAME, 0);

	TCHAR mirandaFolder[1024];
	GetModuleFileName(GetModuleHandle(NULL), mirandaFolder, MAX_REGS(mirandaFolder));
	TCHAR *p = _tcsrchr(mirandaFolder, _T('\\'));
	if (p != NULL)
		*p = _T('\0');

    // updater plugin support
    if(ServiceExists(MS_UPDATE_REGISTER))
	{
		Update upd = {0};
		char szCurrentVersion[30];

		upd.cbSize = sizeof(upd);
		upd.szComponentName = pluginInfo.shortName;

		upd.szUpdateURL = UPDATER_AUTOREGISTER;

		upd.szBetaVersionURL = "http://pescuma.org/miranda/skins_version.txt";
		upd.szBetaChangelogURL = "http://pescuma.org/miranda/skins#Changelog";
		upd.pbBetaVersionPrefix = (BYTE *)"Skins ";
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);
#ifdef UNICODE
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/skinsW.zip";
#else
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/skins.zip";
#endif

		upd.pbVersion = (BYTE *)CreateVersionStringPlugin((PLUGININFO*) &pluginInfo, szCurrentVersion);
		upd.cpbVersion = strlen((char *)upd.pbVersion);

        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	}

    // Folders plugin support
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
	{
		hSkinsFolder = (HANDLE) FoldersRegisterCustomPathT(Translate("Skins"), 
					Translate("Skins"), 
					_T(MIRANDA_PATH) _T("\\Skins"));

		FoldersGetCustomPathT(hSkinsFolder, skinsFolder, MAX_REGS(skinsFolder), _T("."));
	}
	else
	{
		mir_sntprintf(skinsFolder, MAX_REGS(skinsFolder), _T("%s\\Skins"), mirandaFolder);
	}

	InitOptions();

	loaded = TRUE;

	return 0;
}


int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	unsigned int i;

	for(i = 0; i < hServices.size(); i++)
		DestroyServiceFunction(hServices[i]);
	hServices.clear();

	for(i = 0; i < hHooks.size(); i++)
		UnhookEvent(hHooks[i]);
	hHooks.clear();

	DeInitOptions();

	return 0;
}


BOOL FileExists(const char *filename)
{
	DWORD attrib = GetFileAttributesA(filename);
	if (attrib == 0xFFFFFFFF || (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
	return TRUE;
}

#ifdef UNICODE
BOOL FileExists(const WCHAR *filename)
{
	DWORD attrib = GetFileAttributesW(filename);
	if (attrib == 0xFFFFFFFF || (attrib & FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
	return TRUE;
}
#endif


BOOL DirExists(const char *filename)
{
	DWORD attrib = GetFileAttributesA(filename);
	if (attrib == 0xFFFFFFFF || !(attrib & FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
	return TRUE;
}

#ifdef UNICODE
BOOL DirExists(const WCHAR *filename)
{
	DWORD attrib = GetFileAttributesW(filename);
	if (attrib == 0xFFFFFFFF || !(attrib & FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;
	return TRUE;
}
#endif


BOOL CreatePath(const char *path) 
{
	char folder[1024];
	strncpy(folder, path, MAX_REGS(folder));
	folder[MAX_REGS(folder)-1] = '\0';

	char *p = folder;
	if (p[0] && p[1] == ':' && p[2] == '\\') p += 3; // skip drive letter

	SetLastError(ERROR_SUCCESS);
	while(p = strchr(p, '\\')) 
	{
		*p = '\0';
		CreateDirectoryA(folder, 0);
		*p = '\\';
		p++;
	}
	CreateDirectoryA(folder, 0);

	DWORD lerr = GetLastError();
	return (lerr == ERROR_SUCCESS || lerr == ERROR_ALREADY_EXISTS);
}


void log(const char *fmt, ...)
{
    va_list va;
    char text[1024];

    va_start(va, fmt);
    mir_vsnprintf(text, sizeof(text), fmt, va);
    va_end(va);

	CallService(MS_NETLIB_LOG, (WPARAM) NULL, (LPARAM) text);
}

