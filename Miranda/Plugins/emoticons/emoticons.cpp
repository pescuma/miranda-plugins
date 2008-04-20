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
	"Emoticons (Unicode)",
#else
	"Emoticons",
#endif
	PLUGIN_MAKE_VERSION(0,0,2,2),
	"Emoticons",
	"Ricardo Pescuma Domenecci",
	"",
	"© 2008 Ricardo Pescuma Domenecci",
	"http://pescuma.org/miranda/emoticons",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0x80ad2967, 0x2f29, 0x4550, { 0x87, 0x19, 0x23, 0x26, 0x41, 0xd4, 0xc8, 0x83 } } // {80AD2967-2F29-4550-8719-232641D4C883}
#else
	{ 0x8b47942a, 0xa294, 0x4b25, { 0x95, 0x1a, 0x20, 0x80, 0x44, 0xc9, 0x4f, 0x4d } } // {8B47942A-A294-4b25-951A-208044C94F4D}
#endif
};


HINSTANCE hInst;
PLUGINLINK *pluginLink;

HANDLE hHooks[3] = {0};
HANDLE hServices[4] = {0};
HANDLE hChangedEvent;
HANDLE hNetlibUser = 0;

HANDLE hProtocolsFolder = NULL;
TCHAR protocolsFolder[1024];

HANDLE hEmoticonPacksFolder = NULL;
TCHAR emoticonPacksFolder[1024];

char *metacontacts_proto = NULL;
BOOL has_anismiley = FALSE;
BOOL loaded = FALSE;

typedef map<HWND, Dialog *> DialogMapType;

DialogMapType dialogData;

LIST_INTERFACE li;
FI_INTERFACE *fei = NULL;

LIST<Module> modules(10);
LIST<EmoticonPack> packs(10);
LIST<Contact> contacts(10);

ContactAsyncQueue *downloadQueue;

BOOL LoadModule(Module *m);
void LoadModules();
BOOL LoadPack(EmoticonPack *p);
void LoadPacks();

BOOL isValidExtension(char *name);
#ifdef UNICODE
BOOL isValidExtension(WCHAR *name);
#endif
void DownloadCallback(HANDLE hContact, void *param);
void log(const char *fmt, ...);
void FillModuleImages(EmoticonPack *pack);

EmoticonPack *GetPack(char *name);
Module * GetContactModule(HANDLE hContact, const char *proto);
Module * GetModule(const char *name);
Contact * GetContact(HANDLE hContact);
CustomEmoticon *GetCustomEmoticon(Contact *c, TCHAR *text);
EmoticonImage * GetModuleImage(EmoticonImage *img, Module *m);
void ReleaseModuleImage(EmoticonImage *img);

int ModulesLoaded(WPARAM wParam, LPARAM lParam);
int PreShutdown(WPARAM wParam, LPARAM lParam);
int MsgWindowEvent(WPARAM wParam, LPARAM lParam);

int ReplaceEmoticonsService(WPARAM wParam, LPARAM lParam);
int GetInfo2Service(WPARAM wParam, LPARAM lParam);
int ShowSelectionService(WPARAM wParam, LPARAM lParam);
int LoadContactSmileysService(WPARAM wParam, LPARAM lParam);

TCHAR *GetText(RichEditCtrl &rec, int start, int end);


LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


#define DEFINE_GUIDXXX(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUIDXXX(IID_ITextDocument,0x8CC497C0,0xA1DF,0x11CE,0x80,0x98,
                0x00,0xAA,0x00,0x47,0xBE,0x5D);

#define SUSPEND_UNDO(rec)														\
	if (rec.textDocument != NULL)												\
		rec.textDocument->Undo(tomSuspend, NULL)

#define RESUME_UNDO(rec)														\
	if (rec.textDocument != NULL)												\
		rec.textDocument->Undo(tomResume, NULL)

#define	STOP_RICHEDIT(rec)														\
	SUSPEND_UNDO(rec);															\
	SendMessage(rec.hwnd, WM_SETREDRAW, FALSE, 0);								\
	POINT __old_scroll_pos;														\
	SendMessage(rec.hwnd, EM_GETSCROLLPOS, 0, (LPARAM) &__old_scroll_pos);		\
	CHARRANGE __old_sel;														\
	SendMessage(rec.hwnd, EM_EXGETSEL, 0, (LPARAM) &__old_sel);					\
	POINT __caretPos;															\
	GetCaretPos(&__caretPos);													\
    DWORD __old_mask = SendMessage(rec.hwnd, EM_GETEVENTMASK, 0, 0);			\
	SendMessage(rec.hwnd, EM_SETEVENTMASK, 0, __old_mask & ~ENM_CHANGE);		\
	BOOL __inverse = (__old_sel.cpMin >= LOWORD(SendMessage(rec.hwnd, EM_CHARFROMPOS, 0, (LPARAM) &__caretPos)))

#define START_RICHEDIT(rec)														\
	if (__inverse)																\
	{																			\
		LONG __tmp = __old_sel.cpMin;											\
		__old_sel.cpMin = __old_sel.cpMax;										\
		__old_sel.cpMax = __tmp;												\
	}																			\
	SendMessage(rec.hwnd, EM_SETEVENTMASK, 0, __old_mask);						\
	SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &__old_sel);					\
	SendMessage(rec.hwnd, EM_SETSCROLLPOS, 0, (LPARAM) &__old_scroll_pos);		\
	SendMessage(rec.hwnd, WM_SETREDRAW, TRUE, 0);								\
	InvalidateRect(rec.hwnd, NULL, FALSE);										\
	RESUME_UNDO(rec)


static TCHAR *webs[] = { 
	_T("http:/"), 
	_T("ftp:/"), 
	_T("irc:/"), 
	_T("gopher:/"), 
	_T("file:/"), 
	_T("www."), 
	_T("www2."),
	_T("ftp."),
	_T("irc.")
};


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


static const MUUID interfaces[] = { MIID_SMILEY, MIID_LAST };
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
	CallService(MS_IMG_GETINTERFACE, FI_IF_VERSION, (LPARAM) &fei);

	hHooks[0] = HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	hHooks[1] = HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);

	hServices[0] = CreateServiceFunction(MS_SMILEYADD_LOADCONTACTSMILEYS, LoadContactSmileysService);

	hChangedEvent = CreateHookableEvent(ME_SMILEYADD_OPTIONSCHANGED);

	downloadQueue = new ContactAsyncQueue(DownloadCallback, 1);

	return 0;
}


extern "C" int __declspec(dllexport) Unload(void) 
{
	return 0;
}

COLORREF GetSRMMColor(char *tabsrmm, char *scriver, COLORREF def)
{
	COLORREF colour = (COLORREF) -1;
	if (ServiceExists("SRMsg_MOD/SetUserPrefs"))
		colour = (COLORREF) DBGetContactSettingDword(NULL, "TabSRMM_Fonts", tabsrmm, -1); // TabSRMM
	if (colour == (COLORREF) -1)
		colour = (COLORREF) DBGetContactSettingDword(NULL, "SRMM", scriver, -1); // Scriver / SRMM
	if (colour == (COLORREF) -1)
		colour = def; // Default
	return colour;
}

BYTE GetSRMMByte(char *tabsrmm, char *scriver, BYTE def)
{
	BYTE ret = (BYTE) -1;
	if (ServiceExists("SRMsg_MOD/SetUserPrefs"))
		ret = (BYTE) DBGetContactSettingByte(NULL, "TabSRMM_Fonts", tabsrmm, -1); // TabSRMM
	if (ret == (BYTE) -1)
		ret = (BYTE) DBGetContactSettingByte(NULL, "SRMM", scriver, -1); // Scriver / SRMM
	if (ret == (BYTE) -1)
		ret = def; // Default
	return ret;
}

void GetSRMMTString(TCHAR *out, size_t out_size, char *tabsrmm, char *scriver, TCHAR *def)
{
	DBVARIANT dbv;
	if (ServiceExists("SRMsg_MOD/SetUserPrefs") && !DBGetContactSettingTString(NULL, "TabSRMM_Fonts", tabsrmm, &dbv)) 
	{
		lstrcpyn(out, dbv.ptszVal, out_size);
		DBFreeVariant(&dbv);
	}
	else if (!DBGetContactSettingTString(NULL, "SRMM", scriver, &dbv)) 
	{
		lstrcpyn(out, dbv.ptszVal, out_size);
		DBFreeVariant(&dbv);
	}
	else
	{
		lstrcpyn(out, def, out_size);
	}
}

void InitFonts()
{
	FontIDT font = {0};
	font.cbSize = sizeof(font);
	lstrcpyn(font.group, _T("Emoticons"), MAX_REGS(font.group));
	lstrcpyn(font.backgroundGroup, _T("Emoticons"), MAX_REGS(font.group));
	strncpy(font.dbSettingsGroup, MODULE_NAME, MAX_REGS(font.dbSettingsGroup));
	font.order = 0;
	font.flags = FIDF_DEFAULTVALID | FIDF_ALLOWEFFECTS;

	font.deffontsettings.colour = GetSRMMColor("Font16Col", "SRMFont8Col", RGB(0, 0, 0));
	font.deffontsettings.size = GetSRMMByte("Font16Size", "SRMFont8Size", -12);
	font.deffontsettings.style = GetSRMMByte("Font16Sty", "SRMFont8Sty", 0);
	font.deffontsettings.charset = GetSRMMByte("Font16Set", "SRMFont8Set", DEFAULT_CHARSET);
	GetSRMMTString(font.deffontsettings.szFace, MAX_REGS(font.deffontsettings.szFace), "Font16", "SRMFont8", _T("Tahoma"));

	lstrcpyn(font.name, _T("Group Name"), MAX_REGS(font.name));
	strncpy(font.prefix, "GroupName", MAX_REGS(font.prefix));
	lstrcpyn(font.backgroundName, _T("Group Background"), MAX_REGS(font.backgroundName));
	CallService(MS_FONT_REGISTERT, (WPARAM)&font, 0);

	lstrcpyn(font.name, _T("Emoticons Text"), MAX_REGS(font.name));
	strncpy(font.prefix, "EmoticonsText", MAX_REGS(font.prefix));
	lstrcpyn(font.backgroundName, _T("Emoticons Background"), MAX_REGS(font.backgroundName));
	CallService(MS_FONT_REGISTERT, (WPARAM)&font, 0);

	COLORREF bkg = GetSRMMColor("inputbg", "InputBkgColour", GetSysColor(COLOR_WINDOW));

	ColourIDT cid = {0};
	cid.cbSize = sizeof(ColourID);
	lstrcpyn(cid.group, _T("Emoticons"), MAX_REGS(cid.group));
	strncpy(cid.dbSettingsGroup, MODULE_NAME, MAX_REGS(cid.dbSettingsGroup));
	cid.order = 0;
	cid.flags = 0;

	lstrcpyn(cid.name, _T("Emoticons Background"), MAX_REGS(cid.name));
	strncpy(cid.setting, "EmoticonsBackground", MAX_REGS(cid.setting));
	cid.defcolour = bkg;
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

	lstrcpyn(cid.name, _T("Group Background"), MAX_REGS(cid.name));
	strncpy(cid.setting, "GroupBackground", MAX_REGS(cid.setting));
	cid.defcolour = RGB(GetRValue(bkg) > 120 ? GetRValue(bkg) - 30 : GetRValue(bkg) + 30,
						GetGValue(bkg) > 120 ? GetGValue(bkg) - 30 : GetGValue(bkg) + 30,
						GetBValue(bkg) > 120 ? GetBValue(bkg) - 30 : GetBValue(bkg) + 30);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);
}

// Called when all the modules are loaded
int ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{
	if (ServiceExists(MS_MC_GETPROTOCOLNAME))
		metacontacts_proto = (char *) CallService(MS_MC_GETPROTOCOLNAME, 0, 0);

	has_anismiley = ServiceExists(MS_INSERTANISMILEY);

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

		upd.szBetaVersionURL = "http://pescuma.org/miranda/emoticons_version.txt";
		upd.szBetaChangelogURL = "http://pescuma.org/miranda/emoticons#Changelog";
		upd.pbBetaVersionPrefix = (BYTE *)"Emoticons ";
		upd.cpbBetaVersionPrefix = strlen((char *)upd.pbBetaVersionPrefix);
#ifdef UNICODE
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/emoticonsW.zip";
#else
		upd.szBetaUpdateURL = "http://pescuma.org/miranda/emoticons.zip";
#endif

		upd.pbVersion = (BYTE *)CreateVersionStringPlugin((PLUGININFO*) &pluginInfo, szCurrentVersion);
		upd.cpbVersion = strlen((char *)upd.pbVersion);

        CallService(MS_UPDATE_REGISTER, 0, (LPARAM)&upd);
	}

    // Folders plugin support
	if (ServiceExists(MS_FOLDERS_REGISTER_PATH))
	{
		hProtocolsFolder = (HANDLE) FoldersRegisterCustomPathT(Translate("Emoticons"), 
					Translate("Protocols Configuration"), 
					_T(MIRANDA_PATH) _T("\\Plugins\\Emoticons"));

		FoldersGetCustomPathT(hProtocolsFolder, protocolsFolder, MAX_REGS(protocolsFolder), _T("."));

		hEmoticonPacksFolder = (HANDLE) FoldersRegisterCustomPathT(Translate("Emoticons"), 
					Translate("Emoticon Packs"), 
					_T(MIRANDA_PATH) _T("\\Customize\\Emoticons"));

		FoldersGetCustomPathT(hEmoticonPacksFolder, emoticonPacksFolder, MAX_REGS(emoticonPacksFolder), _T("."));
	}
	else
	{
		mir_sntprintf(protocolsFolder, MAX_REGS(protocolsFolder), _T("%s\\Plugins\\Emoticons"), mirandaFolder);
		mir_sntprintf(emoticonPacksFolder, MAX_REGS(emoticonPacksFolder), _T("%s\\Customize\\Emoticons"), mirandaFolder);
	}

	// Fonts support
	InitFonts();

	InitOptions();
	
	LoadModules();
	LoadPacks();

	if (packs.getCount() > 0)
	{
		// Get default pack
		EmoticonPack *pack = GetPack(opts.pack);
		if (pack == NULL)
			pack = packs[0];
		FillModuleImages(pack);
	}


	hHooks[2] = HookEvent(ME_MSG_WINDOWEVENT, &MsgWindowEvent);

	hServices[1] = CreateServiceFunction(MS_SMILEYADD_REPLACESMILEYS, ReplaceEmoticonsService);
	hServices[2] = CreateServiceFunction(MS_SMILEYADD_GETINFO2, GetInfo2Service);
	hServices[3] = CreateServiceFunction(MS_SMILEYADD_SHOWSELECTION, ShowSelectionService);

	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.szSettingsModule = MODULE_NAME;
	nl_user.flags = NUF_OUTGOING | NUF_HTTPCONNS;
	nl_user.szDescriptiveName = Translate("Emoticon downloads");

	hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);

	loaded = TRUE;

	return 0;
}


int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	int i;

	delete downloadQueue;
	downloadQueue = NULL;

	// Delete packs
	for(i = 0; i < packs.getCount(); i++)
	{
		delete packs[i];
	}
	packs.destroy();

	// Delete modules
	for(i = 0; i < modules.getCount(); i++)
	{
		delete modules[i];
	}
	modules.destroy();

	for(i = 0; i < MAX_REGS(hServices); i++)
		DestroyServiceFunction(hServices[i]);

	for(i = 0; i < MAX_REGS(hHooks); i++)
		UnhookEvent(hHooks[i]);

	DeInitOptions();

	if(hNetlibUser)
		CallService(MS_NETLIB_CLOSEHANDLE, (WPARAM)hNetlibUser, 0);

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


// Return the size difference with the original text
int ReplaceEmoticonBackwards(RichEditCtrl &rec, Contact *contact, Module *module, TCHAR *text, int text_len, int last_pos, TCHAR next_char)
{
	// Check if it is an URL
	for (int j = 0; j < MAX_REGS(webs); j++)
	{
		TCHAR *txt = webs[j];
		int len = lstrlen(txt);
		if (last_pos < len || text_len < len)
			continue;

		if (_tcsncmp(&text[text_len - len], txt, len) == 0) 
			return 0;
	}

	// This are needed to allow 2 different emoticons that end the same way
	char found_path[1024];
	int found_len = -1;
	TCHAR *found_text;

	// Replace normal emoticons
	if (!opts.only_replace_isolated || next_char == _T('\0') || _istspace(next_char))
	{	
		for(int i = 0; i < module->emoticons.getCount(); i++)
		{
			Emoticon *e = module->emoticons[i];

			for(int j = 0; j < e->texts.getCount(); j++)
			{
				TCHAR *txt = e->texts[j];
				int len = lstrlen(txt);
				if (last_pos < len || text_len < len)
					continue;

				if (len <= found_len)
					continue;

				if (_tcsncmp(&text[text_len - len], txt, len) != 0)
					continue;

				if (opts.only_replace_isolated && text_len > len 
						&& !_istspace(text[text_len - len - 1]))
					continue;

				if (e->img == NULL)
					found_path[0] = '\0';
				else
					mir_snprintf(found_path, MAX_REGS(found_path), "%s\\%s", e->img->pack->path, e->img->relPath);
				found_len = len;
				found_text = txt;
			}
		}
	}

	// Replace custom smileys
	if (contact != NULL && opts.enable_custom_smileys)
	{
		for(int i = 0; i < contact->emoticons.getCount(); i++)
		{
			CustomEmoticon *e = contact->emoticons[i];

			TCHAR *txt = e->text;
			int len = lstrlen(txt);
			if (last_pos < len || text_len < len)
				continue;

			if (len <= found_len)
				continue;

			if (_tcsncmp(&text[text_len - len], txt, len) != 0)
				continue;

			mir_snprintf(found_path, MAX_REGS(found_path), "%s", e->path);
			found_len = len;
			found_text = txt;
		}
	}

	int ret = 0;

	if (found_len > 0 && found_path[0] != '\0')
	{
		// Found ya
		CHARRANGE sel = { last_pos - found_len, last_pos };
		SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);

		if (has_anismiley)
		{
			CHARFORMAT2 cf;
			memset(&cf, 0, sizeof(CHARFORMAT2));
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BACKCOLOR;
			SendMessage(rec.hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

			if (cf.dwEffects & CFE_AUTOBACKCOLOR)
			{
				cf.crBackColor = SendMessage(rec.hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
				SendMessage(rec.hwnd, EM_SETBKGNDCOLOR, 0, cf.crBackColor);
			}

			TCHAR *path = mir_a2t(found_path);
			if (InsertAnimatedSmiley(rec.hwnd, path, cf.crBackColor, 0 , found_text))
			{
				ret = - found_len + 1;
			}
			MIR_FREE(path);
		}
		else
		{
			OleImage *img = new OleImage(found_path, found_text, found_text);
			if (!img->isValid())
			{
				delete img;
				return 0;
			}

			IOleClientSite *clientSite; 
			rec.ole->GetClientSite(&clientSite);

			REOBJECT reobject = {0};
			reobject.cbStruct = sizeof(REOBJECT);
			reobject.cp = REO_CP_SELECTION;
			reobject.dvaspect = DVASPECT_CONTENT;
			reobject.poleobj = img;
			reobject.polesite = clientSite;
			reobject.dwFlags = REO_BELOWBASELINE; // | REO_DYNAMICSIZE;

			if (rec.ole->InsertObject(&reobject) == S_OK)
			{
				img->SetClientSite(clientSite);
				ret = - found_len + 1;
			}

			clientSite->Release();
			img->Release();
		}
	}

	return ret;
}

void FixSelection(LONG &sel, LONG end, int dif)
{
	if (sel >= end)
		sel += dif;
	else if (sel >= min(end, end + dif))
		sel = min(end, end + dif);
}


TCHAR *GetText(RichEditCtrl &rec, int start, int end)
{
	if (end <= start)
		end = GetWindowTextLength(rec.hwnd);

	ITextRange *range;
	if (rec.textDocument->Range(start, end, &range) != S_OK) 
		return mir_tstrdup(_T(""));

	BSTR text = NULL;
	if (range->GetText(&text) != S_OK || text == NULL)
	{
		range->Release();
		return mir_tstrdup(_T(""));
	}

	TCHAR *ret = mir_u2t(text);

	SysFreeString(text);

	range->Release();

	return ret;
/*
	CHARRANGE sel = { start, end };
	if (sel.cpMax <= sel.cpMin)
		sel.cpMax = GetWindowTextLength(hwnd);
	SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
	SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);

	int len = sel.cpMax - sel.cpMin;
	TCHAR *text = (TCHAR *) malloc((len + 1) * sizeof(TCHAR));

	GETTEXTEX ste = {0};
	ste.cb = (len + 1) * sizeof(TCHAR);
	ste.flags = ST_SELECTION;
#ifdef UNICODE
	ste.codepage = 1200; // UNICODE
	SendMessage(hwnd, EM_GETTEXTEX, (WPARAM) &ste, (LPARAM) text);
#else
	ste.codepage = CP_ACP;
	SendMessage(hwnd, EM_GETTEXTEX, (WPARAM) &ste, (LPARAM) text);
#endif

	return text;
*/
}


BOOL IsHidden(RichEditCtrl &rec, int start, int end)
{
	ITextRange *range;
	if (rec.textDocument->Range(start, end, &range) != S_OK) 
		return FALSE;

	ITextFont *font;
	if (range->GetFont(&font) != S_OK) 
	{
		range->Release();
		return FALSE;
	}

	long hidden;
	font->GetHidden(&hidden);
	BOOL ret = (hidden == tomTrue);

	font->Release();
	range->Release();

	return ret;

/*	CHARRANGE sel = { start, end };
	SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
	SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);

	CHARFORMAT2 cf;
	memset(&cf, 0, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(CHARFORMAT2);
	cf.dwMask = CFM_HIDDEN;
	SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);
	return (cf.dwEffects & CFE_HIDDEN) != 0;
*/
}


int ReplaceAllEmoticonsBackwards(RichEditCtrl &rec, Contact *contact, Module *module, TCHAR *text, int len, TCHAR next_char, int start, CHARRANGE &__old_sel)
{
	int ret = 0;
	for(int i = len; i > 0; i--)
	{
		int dif = ReplaceEmoticonBackwards(rec, contact, module, text, i, start + i, i == len ? next_char : text[i]);
		if (dif != 0)
		{
			FixSelection(__old_sel.cpMax, i, dif);
			FixSelection(__old_sel.cpMin, i, dif);

			i += dif;
			ret += dif;
		}
	}
	return ret;
}


int ReplaceAllEmoticonsBackwards(RichEditCtrl &rec, Contact *contact, Module *module)
{
	int ret;

	STOP_RICHEDIT(rec);

	TCHAR *text = GetText(rec, 0, -1);
	int len = lstrlen(text);

	ret = ReplaceAllEmoticonsBackwards(rec, contact, module, text, len, _T('\0'), 0, __old_sel);

	MIR_FREE(text);

	START_RICHEDIT(rec);

	return ret;
}


int matches(const TCHAR *tag, const TCHAR *text)
{
	int len = lstrlen(tag);
	if (_tcsncmp(tag, text, len) == 0)
		return len;
	else
		return 0;
}


void ReplaceAllEmoticons(RichEditCtrl &rec, Contact *contact, Module *module, int start, int end)
{
	STOP_RICHEDIT(rec);

	if (start < 0)
		start = 0;

	TCHAR *text = GetText(rec, start, end);
	int len = lstrlen(text);

	int diff = 0;
	int last_start_pos = 0;
	BOOL replace = TRUE;
	HANDLE hContact = (contact == NULL ? NULL : contact->hContact);
	for(int i = 0; i <= len; i++)
	{
		int tl;
		if (replace)
		{
			if (i == 0 || !_istalnum(text[i - 1]))
			{
				for (int j = 0; j < MAX_REGS(webs); j++)
				{
					if (tl = matches(webs[j], &text[i]))
					{
						diff += ReplaceAllEmoticonsBackwards(rec, contact, module, &text[last_start_pos], i - last_start_pos, _T('\0'), start + last_start_pos + diff, __old_sel);

						i += tl;
						
						for(;  (text[i] >= _T('a') && text[i] <= _T('z'))
							|| (text[i] >= _T('A') && text[i] <= _T('Z'))
							|| (text[i] >= _T('0') && text[i] <= _T('9'))
							|| text[i] == _T('.') || text[i] == _T('/')
							|| text[i] == _T('?') || text[i] == _T('_')
							|| text[i] == _T('=') || text[i] == _T('&')
							|| text[i] == _T('%') || text[i] == _T('-')
							; i++) ;

						last_start_pos = i;
					}
				}
			}
		}

		if (tl = matches(_T("<no-emoticon>"), &text[i]))
		{
			if (IsHidden(rec, start + i, start + i + tl))
			{
				diff += ReplaceAllEmoticonsBackwards(rec, contact, module, &text[last_start_pos], i - last_start_pos, _T('\0'), start + last_start_pos + diff, __old_sel);

				replace = FALSE;
				i += tl - 1;
			}
			continue;
		}

		if (tl = matches(_T("</no-emoticon>"), &text[i]))
		{
			if (IsHidden(rec, start + i, start + i + tl))
			{
				replace = TRUE; 
				i += tl - 1;
				last_start_pos = i + 1;
			}
			continue;
		}

		if (tl = matches(_T("</emoticon-contact>"), &text[i]))
		{
			if (IsHidden(rec, start + i, start + i + tl))
			{
				diff += ReplaceAllEmoticonsBackwards(rec, contact, module, &text[last_start_pos], i - last_start_pos, _T('\0'), start + last_start_pos + diff, __old_sel);

				hContact = (contact == NULL ? NULL : contact->hContact);
				i += tl - 1;
				last_start_pos = i + 1;
			}
			continue;
		}
		
		if (tl = matches(_T("<emoticon-contact "), &text[i]))
		{
			int len = tl;
			for(int j = 0; j < 10 && text[i + len] != '>'; j++, len++) 
				;

			if (text[i + len] != '>')
				continue;

			len++;

			if (IsHidden(rec, start + i, start + i + len))
			{
				diff += ReplaceAllEmoticonsBackwards(rec, contact, module, &text[last_start_pos], i - last_start_pos, _T('\0'), start + last_start_pos + diff, __old_sel);

				hContact = (HANDLE) _ttoi(&text[i + tl]);
				i += len - 1;
				last_start_pos = i + 1;
			}
		}
	}

	if (replace)
		ReplaceAllEmoticonsBackwards(rec, contact, module, &text[last_start_pos], len - last_start_pos, _T('\0'), start + last_start_pos + diff, __old_sel);
	
	MIR_FREE(text);

	START_RICHEDIT(rec);
}


int RestoreInput(RichEditCtrl &rec, int start = 0, int end = -1)
{
	int ret = 0;

	int objectCount = rec.ole->GetObjectCount();
	for (int i = objectCount - 1; i >= 0; i--)
	{
		REOBJECT reObj = {0};
		reObj.cbStruct  = sizeof(REOBJECT);

		HRESULT hr = rec.ole->GetObject(i, &reObj, REO_GETOBJ_POLEOBJ);
		if (!SUCCEEDED(hr))
			continue;

		if (reObj.cp < start || (end >= start && reObj.cp >= end))
		{
			reObj.poleobj->Release();
			continue;
		}

		ITooltipData *ttd = NULL;
		hr = reObj.poleobj->QueryInterface(__uuidof(ITooltipData), (void**) &ttd);
		reObj.poleobj->Release();
		if (FAILED(hr) || ttd == NULL)
			continue;

		BSTR hint = NULL;
		hr = ttd->GetTooltip(&hint);
		if (SUCCEEDED(hr) && hint != NULL)
		{
			ITextRange *range;
			if (rec.textDocument->Range(reObj.cp, reObj.cp + 1, &range) == S_OK) 
			{
				if (range->SetText(hint) == S_OK)
					ret += wcslen(hint) - 1;

				range->Release();
			}

			SysFreeString(hint);
		}

		ttd->Release();
	}

	return ret;
}


LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	BOOL rebuild = FALSE;
	switch(msg)
	{
		case WM_KEYDOWN:
			if ((!(GetKeyState(VK_CONTROL) & 0x8000) || (wParam != 'C' && wParam != 'X' && wParam != VK_INSERT))
				&& (!(GetKeyState(VK_SHIFT) & 0x8000) || wParam != VK_DELETE))
				break;
		case WM_CUT:
		case WM_COPY:
		{
			STOP_RICHEDIT(dlg->input);
			__old_sel.cpMax += RestoreInput(dlg->input, __old_sel.cpMin, __old_sel.cpMax);
			START_RICHEDIT(dlg->input);

			rebuild = TRUE;
			break;
		}
	}

	LRESULT ret = CallWindowProc(dlg->input.old_edit_proc, hwnd, msg, wParam, lParam);

	switch(msg)
	{
		case WM_KEYDOWN:
		{
			if ((GetKeyState(VK_SHIFT) & 0x8000) && wParam == VK_DELETE) 
				break;

			if (wParam != VK_DELETE && wParam != VK_BACK)
				break;
		}
		case WM_CHAR:
		{
			if (msg == WM_CHAR && wParam >= 0 && wParam <= 32 && (!_istspace(wParam) || !opts.only_replace_isolated))
				break;

			if (lParam & (1 << 28))	// ALT key
				break;

			if (wParam != _T('\n') && GetKeyState(VK_CONTROL) & 0x8000)	// CTRL key
				break;

			if ((lParam & 0xFF) > 2)	// Repeat rate
				break;

			STOP_RICHEDIT(dlg->input);

			CHARRANGE sel = {0};
			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);

			int min = max(0, sel.cpMax - 10);

			int dif = RestoreInput(dlg->input, min, sel.cpMax);
			if (dif != 0)
			{
				FixSelection(__old_sel.cpMax, sel.cpMax, dif);
				FixSelection(__old_sel.cpMin, sel.cpMax, dif);
				sel.cpMax += dif;
			}

			TCHAR *text = GetText(dlg->input, min, sel.cpMax + 1);
			int len = lstrlen(text);
			TCHAR last;
			if (len == sel.cpMax + 1 - min) 
			{
				// Strip
				len--;
				last = text[len];
			}
			else
			{
				last = _T('\0');
			}

			if (dif == 0 && !opts.only_replace_isolated)
			{
				// Can replace just last text
				dif = ReplaceEmoticonBackwards(dlg->input, NULL, dlg->module, text, len, sel.cpMax, last);
				if (dif != 0)
				{
					FixSelection(__old_sel.cpMax, sel.cpMax, dif);
					FixSelection(__old_sel.cpMin, sel.cpMax, dif);
				}
			}
			else
			{
				// Because we already changed the text, we need to replace all range
				ReplaceAllEmoticonsBackwards(dlg->input, NULL, dlg->module, text, len, last, min, __old_sel);
			}

			MIR_FREE(text);

			START_RICHEDIT(dlg->input);

			break;
		}
		case EM_REPLACESEL:
		case WM_SETTEXT:
		case EM_SETTEXTEX:
			if (dlg->log.sending)
				break;
		case EM_PASTESPECIAL:
		case WM_PASTE:
			rebuild = TRUE;
			break;
	}

	if (rebuild)
		ReplaceAllEmoticonsBackwards(dlg->input, NULL, dlg->module);

	return ret;
}

/*
LRESULT CALLBACK LogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	switch(msg)
	{
		case WM_SETREDRAW:
		{
			if (wParam == FALSE)
			{
				dlg->log.received_stream_in = FALSE;
			}
			else
			{
				if (dlg->log.received_stream_in)
				{
					RichEditCtrl &rec = dlg->log;
					SUSPEND_UNDO(rec);															\
					POINT __old_scroll_pos;														\
					SendMessage(rec.hwnd, EM_GETSCROLLPOS, 0, (LPARAM) &__old_scroll_pos);		\
					CHARRANGE __old_sel;														\
					SendMessage(rec.hwnd, EM_EXGETSEL, 0, (LPARAM) &__old_sel);					\
					POINT __caretPos;															\
					GetCaretPos(&__caretPos);													\
					DWORD __old_mask = SendMessage(rec.hwnd, EM_GETEVENTMASK, 0, 0);			\
					SendMessage(rec.hwnd, EM_SETEVENTMASK, 0, __old_mask & ~ENM_CHANGE);		\
					BOOL __inverse = (__old_sel.cpMin >= LOWORD(SendMessage(rec.hwnd, EM_CHARFROMPOS, 0, (LPARAM) &__caretPos)));

					CHARRANGE sel = { dlg->log.stream_in_pos, GetWindowTextLength(dlg->log.hwnd) };
					SendMessage(dlg->log.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);

					int len = sel.cpMax - sel.cpMin;
					TCHAR *text = (TCHAR *) malloc((len + 1) * sizeof(TCHAR));
					SendMessage(dlg->log.hwnd, EM_GETSELTEXT, 0, (LPARAM) text);

					for(int i = len; i > 0; i--)
					{
						int dif = ReplaceEmoticonBackwards(dlg->log.hwnd, text, i, sel.cpMin + i, dlg->module);

						FixSelection(__old_sel.cpMax, i, dif);
						FixSelection(__old_sel.cpMin, i, dif);
					}

					if (__inverse)																\
					{																			\
						LONG __tmp = __old_sel.cpMin;											\
						__old_sel.cpMin = __old_sel.cpMax;										\
						__old_sel.cpMax = __tmp;												\
					}																			\
					SendMessage(rec.hwnd, EM_SETEVENTMASK, 0, __old_mask);						\
					SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &__old_sel);				\
					SendMessage(rec.hwnd, EM_SETSCROLLPOS, 0, (LPARAM) &__old_scroll_pos);		\
					InvalidateRect(rec.hwnd, NULL, FALSE);										\
					RESUME_UNDO(rec);
				}
			}
			break;
		}
		case EM_STREAMIN:
		{
			dlg->log.received_stream_in = TRUE;
			dlg->log.stream_in_pos = GetWindowTextLength(dlg->log.hwnd);
			break;
		}
	}

	LRESULT ret = CallWindowProc(dlg->log.old_edit_proc, hwnd, msg, wParam, lParam);

	return ret;
}
*/


LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	if (msg == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1624) && dlg->input.old_edit_proc != NULL)
	{
		dlg->log.sending = TRUE;

		STOP_RICHEDIT(dlg->input);

		RestoreInput(dlg->input);

		START_RICHEDIT(dlg->input);
	}

	LRESULT ret = CallWindowProc(dlg->owner_old_edit_proc, hwnd, msg, wParam, lParam);

	switch(msg)
	{
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1624)
			{
				if (!ret)
					// Add emoticons again
					ReplaceAllEmoticonsBackwards(dlg->input, NULL, dlg->module);

				dlg->log.sending = FALSE;
			}
			break;
		}
	}

	return ret;
}


int LoadRichEdit(RichEditCtrl *rec, HWND hwnd) 
{
	rec->hwnd = hwnd;
	rec->ole = NULL;
	rec->textDocument = NULL;
	rec->old_edit_proc = NULL;
	rec->received_stream_in = FALSE;
	rec->sending = FALSE;

	SendMessage(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&rec->ole);
	if (rec->ole == NULL)
		return 0;

	if (rec->ole->QueryInterface(IID_ITextDocument, (void**)&rec->textDocument) != S_OK)
		rec->textDocument = NULL;

	return 1;
}


void UnloadRichEdit(RichEditCtrl *rec) 
{
	if (rec->textDocument != NULL)
		rec->textDocument->Release();
	if (rec->ole != NULL)
		rec->ole->Release();
}


HANDLE GetRealContact(HANDLE hContact)
{
	if (hContact == NULL || !ServiceExists(MS_MC_GETMOSTONLINECONTACT))
		return hContact;

	HANDLE hReal = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) hContact, 0);
	if (hReal == NULL)
		hReal = hContact;
	return hReal;
}


int MsgWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData *event = (MessageWindowEventData *)lParam;
	if (event == NULL)
		return 0;

	if (event->cbSize < sizeof(MessageWindowEventData))
		return 0;

	if (event->uType == MSG_WINDOW_EVT_OPEN)
	{
		Module *m = GetContactModule(event->hContact);
		if (m == NULL)
			return 0;

		Dialog *dlg = (Dialog *) malloc(sizeof(Dialog));
		ZeroMemory(dlg, sizeof(Dialog));

		dlg->contact = GetContact(event->hContact);
		dlg->module = m;
		dlg->hwnd_owner = event->hwndWindow;

		LoadRichEdit(&dlg->input, event->hwndInput);
		LoadRichEdit(&dlg->log, event->hwndLog);

		if (opts.replace_in_input)
		{
			dlg->input.old_edit_proc = (WNDPROC) SetWindowLong(dlg->input.hwnd, GWL_WNDPROC, (LONG) EditProc);
			dialogData[dlg->input.hwnd] = dlg;
		}

		dlg->owner_old_edit_proc = (WNDPROC) SetWindowLong(dlg->hwnd_owner, GWL_WNDPROC, (LONG) OwnerProc);
		dialogData[dlg->hwnd_owner] = dlg;

//		dlg->log.old_edit_proc = (WNDPROC) SetWindowLong(dlg->log.hwnd, GWL_WNDPROC, (LONG) LogProc);
		dialogData[dlg->log.hwnd] = dlg;
	}
	else if (event->uType == MSG_WINDOW_EVT_CLOSING)
	{
		DialogMapType::iterator dlgit = dialogData.find(event->hwndWindow);
		if (dlgit != dialogData.end())
		{
			Dialog *dlg = dlgit->second;

			UnloadRichEdit(&dlg->input);
			UnloadRichEdit(&dlg->log);

			if (dlg->input.old_edit_proc != NULL)
				SetWindowLong(dlg->input.hwnd, GWL_WNDPROC, (LONG) dlg->input.old_edit_proc);
			SetWindowLong(dlg->hwnd_owner, GWL_WNDPROC, (LONG) dlg->owner_old_edit_proc);

			free(dlg);
		}

		dialogData.erase(event->hwndInput);
		dialogData.erase(event->hwndLog);
		dialogData.erase(event->hwndWindow);
	}

	return 0;
}


TCHAR *lstrtrim(TCHAR *str)
{
	int len = lstrlen(str);

	int i;
	for(i = len - 1; i >= 0 && (str[i] == _T(' ') || str[i] == _T('\t')); --i) ;
	if (i < len - 1)
	{
		++i;
		str[i] = _T('\0');
		len = i;
	}

	for(i = 0; i < len && (str[i] == _T(' ') || str[i] == _T('\t')); ++i) ;
	if (i > 0)
		memmove(str, &str[i], (len - i + 1) * sizeof(TCHAR));

	return str;
}


char *strtrim(char *str)
{
	int len = strlen(str);

	int i;
	for(i = len - 1; i >= 0 && (str[i] == ' ' || str[i] == '\t'); --i) ;
	if (i < len - 1)
	{
		++i;
		str[i] = '\0';
		len = i;
	}

	for(i = 0; i < len && (str[i] == ' ' || str[i] == '\t'); ++i) ;
	if (i > 0)
		memmove(str, &str[i], (len - i + 1) * sizeof(char));

	return str;
}


BOOL HasProto(char *proto)
{
	PROTOCOLDESCRIPTOR **protos;
	int count;
	CallService(MS_PROTO_ENUMPROTOS, (WPARAM)&count, (LPARAM)&protos);

	for (int i = 0; i < count; i++)
	{
		PROTOCOLDESCRIPTOR *p = protos[i];

		if (p->type != PROTOTYPE_PROTOCOL)
			continue;

		if (p->szName == NULL || p->szName[0] == '\0')
			continue;

		if (stricmp(proto, p->szName) == 0)
			return TRUE;
	}

	return FALSE;
}


void LoadModules()
{
	// Load the language files and create an array with then
	TCHAR file[1024];
	mir_sntprintf(file, MAX_REGS(file), _T("%s\\*.emo"), protocolsFolder);

	WIN32_FIND_DATA ffd = {0};
	HANDLE hFFD = FindFirstFile(file, &ffd);
	if (hFFD != INVALID_HANDLE_VALUE)
	{
		do
		{
			mir_sntprintf(file, MAX_REGS(file), _T("%s\\%s"), protocolsFolder, ffd.cFileName);

			if (!FileExists(file))
				continue;

			char *name = mir_t2a(ffd.cFileName);
			name[strlen(name) - 4] = 0;

			Module *m = new Module();
			m->name = name;
			m->path = mir_tstrdup(file);
			modules.insert(m);

			LoadModule(m);
		}
		while(FindNextFile(hFFD, &ffd));

		FindClose(hFFD);
	}
}


void HandleEmoLine(Module *m, char *tmp, char *group)
{
	int len = strlen(tmp);
	int state = 0;
	int pos;

	Emoticon *e = NULL;

	for(int i = 0; i < len; i++)
	{
		char c = tmp[i];
		if (c == ' ')
			continue;

		if ((state % 2) == 0)
		{
			if (c == '#')
				break;

			if (c != '"')
				continue;

			state ++;
			pos = i+1;
		}
		else
		{
			if (c == '\\')
			{
				i++;
				continue;
			}
			if (c != '"')
				continue;

			tmp[i] = 0;
			TCHAR *txt = mir_a2t(&tmp[pos]);
			char *atxt = &tmp[pos];

			for(int j = 0, orig = 0; j <= i - pos; j++)
			{
				if (txt[j] == '\\')
					j++;
				txt[orig] = txt[j];
				orig++;
			}

			// Found something
			switch(state)
			{
				case 1: 
					e = new Emoticon();
					e->name = mir_t2a(txt);
					e->group = group;
					MIR_FREE(txt);
					break;
				case 3: 
					e->description = txt; 
					break;
				case 5: 
					if (strncmp(e->name, "service_", 8) == 0)
					{
						MIR_FREE(txt); // Not needed

						int len = strlen(atxt);

						// Is a service
						if (!strncmp(atxt, "<Service:", 9) == 0 || atxt[len-1] != '>')
						{
							delete e;
							e = NULL;
							return;
						}

						atxt[len-1] = '\0';

						char *params = &atxt[9];
						for(int i = 0; i < 3; i++)
						{
							char *pos = strchr(params, ':');
							if (pos != NULL)
								*pos = '\0';

							e->service[i] = mir_strdup(params);

							if (pos == NULL)
								break;

							params = pos + 1;
						}

//						if (e->service[0] == NULL || e->service[0][0] == '\0' || !ProtoServiceExists(m->name, e->service[0]))
//						{
//							delete e;
//							e = NULL;
//							return;
//						}
					}
					else
						e->texts.insert(txt); 
					break;
			}

			state++;
			if (state == 6)
				state = 4;
		}
	}

	if (e != NULL)
		m->emoticons.insert(e);
}


void HandleDerived(Module *m, char *derived)
{
	char *db_key = strchr(derived, ':');
	if (db_key == NULL) 
	{
		log("Invalid derived line '%s' in module %s", derived, m->name);
		return;
	}

	*db_key = '\0';
	db_key++;

	char *db_val = strchr(db_key, '=');
	if (db_val == NULL)
	{
		log("Invalid db string '%s' in derived line in module %s", db_key, m->name);
		return;
	}

	*db_val = '\0';
	db_val++;

	if (db_val[0] != 'a')
	{
		log("Invalid db val '%s' (should start with a for ASCII) in derived line in module %s", db_val, m->name);
		return;
	}

	db_val++;

	strtrim(derived);
	strtrim(db_key);
	strtrim(db_val);

	m->derived.proto_name = mir_strdup(derived);
	m->derived.db_key = mir_strdup(db_key);
	m->derived.db_val = mir_strdup(db_val);
}


BOOL LoadModule(Module *m)
{
	FILE *file = _tfopen(m->path, _T("rb"));
	if (file == NULL) 
		return FALSE;
	
	char tmp[1024];
	char c;
	int pos = 0;
	char *group = "";
	do
	{
		c = fgetc(file);

		if (c == '\n' || c == '\r' || c == EOF || pos >= MAX_REGS(tmp) - 1) 
		{
			tmp[pos] = 0;
			strtrim(tmp);
			size_t len = strlen(tmp);

			if (tmp[0] == '#')
			{
				// Do nothing
			}
			else if (tmp[0] == '[' && tmp[len-1] == ']')
			{
				tmp[len-1] = '\0';
				strtrim(&tmp[1]);
				group = mir_strdup(&tmp[1]);
			}
			else if (strnicmp("Derived:", tmp, 8) == 0)
			{
				HandleDerived(m, &tmp[8]);
			}
			else
			{
				HandleEmoLine(m, tmp, group);
			}

			pos = 0;
		}
		else
		{
			tmp[pos] = c;
			pos ++;
		}
	}
	while(c != EOF);
	fclose(file);

	return TRUE;
}



void LoadPacks()
{
	// Load the language files and create an array with then
	TCHAR file[1024];
	mir_sntprintf(file, MAX_REGS(file), _T("%s\\*"), emoticonPacksFolder);

	WIN32_FIND_DATA ffd = {0};
	HANDLE hFFD = FindFirstFile(file, &ffd);
	if (hFFD != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (lstrcmp(ffd.cFileName, _T(".")) == 0 || lstrcmp(ffd.cFileName, _T("..")) == 0)
				continue;

			mir_sntprintf(file, MAX_REGS(file), _T("%s\\%s"), emoticonPacksFolder, ffd.cFileName);

			DWORD attrib = GetFileAttributes(file);
			if (attrib == 0xFFFFFFFF || !(attrib & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			EmoticonPack *p = new EmoticonPack();
			p->name = mir_t2a(ffd.cFileName);
			p->description = mir_tstrdup(ffd.cFileName);
			p->path = mir_t2a(file);
			packs.insert(p);

			LoadPack(p);
		}
		while(FindNextFile(hFFD, &ffd));

		FindClose(hFFD);
	}
}


BOOL isValidExtension(char *name)
{
	char *p = strrchr(name, '.');
	if (p == NULL)
		return FALSE;
	if (strcmp(p, ".jpg") != 0
			&& strcmp(p, ".jpeg") != 0
			&& strcmp(p, ".gif") != 0
			&& strcmp(p, ".png") != 0
			&& strcmp(p, ".ico") != 0
			&& strcmp(p, ".swf") != 0)
		return FALSE;
	return TRUE;
}


#ifdef UNICODE
BOOL isValidExtension(WCHAR *name)
{
	WCHAR *p = wcsrchr(name, L'.');
	if (p == NULL)
		return FALSE;
	if (lstrcmpW(p, L".jpg") != 0
			&& lstrcmpW(p, L".jpeg") != 0
			&& lstrcmpW(p, L".gif") != 0
			&& lstrcmpW(p, L".png") != 0
			&& lstrcmpW(p, L".ico") != 0
			&& lstrcmpW(p, L".swf") != 0)
		return FALSE;
	return TRUE;
}
#endif


EmoticonImage * HandleMepLine(EmoticonPack *p, char *line)
{
	int len = strlen(line);
	int state = 0;
	int pos;
	int module_pos = -1;
	BOOL noDelimiter;

	EmoticonImage *img = NULL;

	for(int i = 0; i <= len && state < 6; i++)
	{
		char c = line[i];
		if (c == ' ')
			continue;

		if ((state % 2) == 0)
		{
			if (c == '"')
			{
				state ++;
				pos = i+1;
				noDelimiter = FALSE;
			}
			else if (c != '=' && c != ',' && c != ' ' && c != '\t' && c != '\r' && c != '\n')
			{
				state ++;
				pos = i;
				noDelimiter = TRUE;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (state == 1 && c == '\\') // Module name
				module_pos = i;

			if (noDelimiter)
			{
				if (c != ' ' && c != ',' && c != '=' && c != '\0')
					continue;
			}
			else
			{
				if (c != '"')
					continue;
			}

			line[i] = 0;

			char *module;
			char *txt;
			
			if (state == 1 && module_pos >= 0)
			{
				line[module_pos] = 0;

				module = mir_strdup(&line[pos]);
				txt = mir_strdup(&line[module_pos + 1]);
			}
			else
			{
				module = NULL;
				txt = mir_strdup(&line[pos]);
			}

			// Found something
			switch(state)
			{
				case 1: 
				{
					img = new EmoticonImage();
					img->pack = p;
					img->name = txt;
					img->module = module;
					break;
				}
				case 3: 
				{
					if (!isValidExtension(txt))
					{
						delete img;
						img = NULL;
						return img;
					}

					if (strncmp("http://", txt, 7) == 0)
					{
						img->url = txt; 

						char *p = strrchr(txt, '/');
						p++;

						char tmp[1024];
						if (img->module == NULL)
							mir_snprintf(tmp, MAX_REGS(tmp), "cache\\%s", p);
						else
							mir_snprintf(tmp, MAX_REGS(tmp), "cache\\%s\\%s", img->module, p);

						img->relPath = mir_strdup(tmp); 
					}
					else
					{
						img->relPath = txt; 
					}
					break;
				}
				case 5:
				{
					img->selectionFrame = max(0, atoi(txt) - 1);
					mir_free(txt);
					break;
				}
			}

			state++;
		}
	}

	return img;
}


BOOL LoadPack(EmoticonPack *pack)
{
	// Load the language files and create an array with then
	char filename[1024];
	mir_snprintf(filename, MAX_REGS(filename), "%s\\*.*", pack->path);

	WIN32_FIND_DATAA ffd = {0};
	HANDLE hFFD = FindFirstFileA(filename, &ffd);
	if (hFFD != INVALID_HANDLE_VALUE)
	{
		do
		{
			mir_snprintf(filename, MAX_REGS(filename), "%s\\%s", pack->path, ffd.cFileName);

			if (!FileExists(filename))
				continue;

			if (!isValidExtension(ffd.cFileName))
				continue;

			EmoticonImage *img = new EmoticonImage();
			img->pack = pack;
			img->name = mir_strdup(ffd.cFileName);
			*strrchr(img->name, '.') = '\0';
			img->relPath = mir_strdup(ffd.cFileName);
			pack->images.insert(img);
		}
		while(FindNextFileA(hFFD, &ffd));

		FindClose(hFFD);
	}

	// Load the mep file
	mir_snprintf(filename, MAX_REGS(filename), "%s\\%s.mep", pack->path, pack->name);
	FILE *file = fopen(filename, "rb");
	if (file == NULL) 
		return TRUE;
	
	char tmp[1024];
	char c;
	int pos = 0;
	do
	{
		c = fgetc(file);

		if (c == '\n' || c == '\r' || c == EOF || pos >= MAX_REGS(tmp) - 1) 
		{
			tmp[pos] = 0;
			
			strtrim(tmp);
			if (tmp[0] == '#')
			{
				// Do nothing
			}
			else if (strnicmp("Name:", tmp, 5) == 0)
			{
				char *name = strtrim(&tmp[5]);
				if (name[0] != '\0') 
				{
					MIR_FREE(pack->description);
					pack->description = mir_a2t(name);
				}
			}
			else if (strnicmp("Creator:", tmp, 8) == 0)
			{
				char *creator = strtrim(&tmp[8]);
				if (creator[0] != '\0') 
					pack->creator = mir_a2t(creator);
			}
			else if (strnicmp("Updater URL:", tmp, 12) == 0)
			{
				char *updater_URL = strtrim(&tmp[12]);
				if (updater_URL[0] != '\0') 
					pack->updater_URL = mir_a2t(updater_URL);
			}
			else if (tmp[0] == '"')
			{
				EmoticonImage *img = HandleMepLine(pack, tmp);
				if (img != NULL)
				{
					// Chek if already exists
					for(int i = 0; i < pack->images.getCount(); i++)
					{
						EmoticonImage *tmp_img = pack->images[i];
						if (strcmp(img->name, tmp_img->name) == 0
							&& ((tmp_img->module == NULL && img->module == NULL) 
								|| (tmp_img->module != NULL && img->module != NULL 
									&& stricmp(tmp_img->module, img->module) == 0)))
						{
							delete tmp_img;
							pack->images.remove(i);
							break;
						}
					}

					// Add the new one
					pack->images.insert(img);
				}
			}

			pos = 0;
		}
		else
		{
			tmp[pos] = c;
			pos ++;
		}
	}
	while(c != EOF);
	fclose(file);

	return TRUE;
}


EmoticonImage * GetModuleImage(EmoticonPack *pack, EmoticonImage *img, Module *m)
{
	if (img->isAvaiableFor(m->name))
	{
		EmoticonImage * ret = new EmoticonImage();
		ret->pack = img->pack;
		ret->name = mir_strdup(img->name);

		int size = strlen(m->name) + 1 + strlen(img->relPath) + 1;
		ret->relPath = (char *) mir_alloc(size * sizeof(char));
		mir_snprintf(ret->relPath, size, "%s\\%s", m->name, img->relPath);

		ret->module = mir_strdup(m->name);
		pack->images.insert(ret);
		return ret;
	}
	else if (img->isAvaiable())
	{
		return img;
	}
	else
	{
		return NULL;
	}
}


EmoticonImage * GetEmoticomImageFromDisk(EmoticonPack *pack, Emoticon *e, Module *module)
{
	EmoticonImage *img = NULL;

	char filename[1024];
	mir_snprintf(filename, MAX_REGS(filename), "%s\\%s\\%s.*", pack->path, module->name, e->name);

	WIN32_FIND_DATAA ffd = {0};
	HANDLE hFFD = FindFirstFileA(filename, &ffd);
	if (hFFD != INVALID_HANDLE_VALUE)
	{
		do
		{
			mir_snprintf(filename, MAX_REGS(filename), "%s\\%s\\%s", pack->path, module->name, ffd.cFileName);

			if (!FileExists(filename))
				continue;

			if (!isValidExtension(ffd.cFileName))
				continue;

			img = new EmoticonImage();
			img->pack = pack;
			img->name = mir_strdup(ffd.cFileName);
			*strrchr(img->name, '.') = '\0';
			img->module = module->name;
			mir_snprintf(filename, MAX_REGS(filename), "%s\\%s", module->name, ffd.cFileName);
			img->relPath = mir_strdup(filename);
			break;
		}
		while(FindNextFileA(hFFD, &ffd));

		FindClose(hFFD);
	}

	return img;
}


void FillModuleImages(EmoticonPack *pack)
{
	for(int j = 0; j < modules.getCount(); j++)
	{
		Module *m = modules[j];
		for(int k = 0; k < m->emoticons.getCount(); k++)
		{
			Emoticon *e = m->emoticons[k];

			// Free old one
			e->img = NULL;

			// First try from pack file for this module
			int i;
			for(i = 0; i < pack->images.getCount(); i++)
			{
				EmoticonImage *img = pack->images[i];
				if (img->module != NULL && stricmp(img->module, m->name) == 0 && strcmp(img->name, e->name) == 0)
				{
					e->img = img;
					break;
				}
			}
			if (e->img != NULL)
			{
				e->img->Download();
				continue;
			}

			// Now try to load from disk
			e->img = GetEmoticomImageFromDisk(pack, e, m);
			if (e->img != NULL)
				continue;

			// Now try to load from defaults cache
			for(i = 0; i < pack->images.getCount(); i++)
			{
				EmoticonImage *img = pack->images[i];
				if (img->module == NULL && strcmp(img->name, e->name) == 0)
				{
					e->img = GetModuleImage(pack, img, m);
					break;
				}
			}
			if (e->img != NULL)
				continue;

			log("  ***  The pack '" TCHAR_STR_PARAM "' does not have the emoticon '%s' needed by %s\n", pack->description, e->name, m->name);
		}
	}
}


EmoticonPack *GetPack(char *name)
{
	EmoticonPack *ret = NULL;
	for(int i = 0; i < packs.getCount(); i++) 
	{
		if (strcmpi(packs[i]->name, name) == 0)
		{
			ret = packs[i];
			break;
		}
	}
	return ret;
}


Module *GetModuleByName(const char *name)
{
	Module *ret = NULL;
	for(int i = 0; i < modules.getCount(); i++) 
	{
		if (stricmp(modules[i]->name, name) == 0)
		{
			ret = modules[i];
			break;
		}
	}
	return ret;
}


Module * GetContactModule(HANDLE hContact, const char *proto)
{
	if (hContact == NULL)
	{
		if (proto == NULL)
			return NULL;
		return GetModule(proto);
	}

	hContact = GetRealContact(hContact);

	proto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (proto == NULL)
		return NULL;

	PROTOACCOUNT *acc = ProtoGetAccount(proto);
	if (acc == NULL)
		return NULL;

	proto = acc->szProtoName;

	// Check for transports
	if (stricmp("JABBER", proto) == 0)
	{
		DBVARIANT dbv = {0};
		if (DBGetContactSettingString(hContact, acc->szModuleName, "Transport", &dbv) == 0)
		{
			Module *ret = GetModule(dbv.pszVal);

			DBFreeVariant(&dbv);

			return ret;
		}
	}

	// Check if there is some derivation
	for(int i = 0; i < modules.getCount(); i++) 
	{
		Module *m = modules[i];
		
		if (m->derived.proto_name == NULL)
			continue;

		if (stricmp(m->derived.proto_name, proto) != 0)
			continue;
		
		DBVARIANT dbv = {0};
		if (DBGetContactSettingString(NULL, acc->szModuleName, m->derived.db_key, &dbv) != 0)
			continue;

		Module *ret = NULL;
		if (stricmp(dbv.pszVal, m->derived.db_val) == 0)
			ret = m;

		DBFreeVariant(&dbv);

		if (ret != NULL)
			return ret;
	}

	return GetModule(proto);
}


Module *GetModule(const char *name)
{
	Module *ret = GetModuleByName(name);
	if (ret == NULL && opts.use_default_pack)
		ret = GetModuleByName("Default");
	return ret;
}


int ReplaceEmoticonsService(WPARAM wParam, LPARAM lParam)
{
	SMADD_RICHEDIT3 *sre = (SMADD_RICHEDIT3 *) lParam;
	if (sre == NULL || sre->cbSize < sizeof(SMADD_RICHEDIT3))
		return FALSE;
	if (sre->hwndRichEditControl == NULL ||
			(sre->Protocolname == NULL && sre->hContact == NULL))
		return FALSE;


	DialogMapType::iterator dlgit = dialogData.find(sre->hwndRichEditControl);
	if (dlgit != dialogData.end())
	{
		Dialog *dlg = dlgit->second;
		ReplaceAllEmoticons(dlg->log, dlg->contact, dlg->module, sre->rangeToReplace == NULL ? 0 : sre->rangeToReplace->cpMin, 
			sre->rangeToReplace == NULL ? -1 : sre->rangeToReplace->cpMax);
	}
	else
	{
		Module *m = GetContactModule(sre->hContact, sre->Protocolname);
		if (m == NULL)
			return FALSE;

		RichEditCtrl rec = {0};
		LoadRichEdit(&rec, sre->hwndRichEditControl);
		ReplaceAllEmoticons(rec, GetContact(sre->hContact), m, sre->rangeToReplace == NULL ? 0 : sre->rangeToReplace->cpMin, 
			sre->rangeToReplace == NULL ? -1 : sre->rangeToReplace->cpMax);
	}

	return TRUE;
}


int GetInfo2Service(WPARAM wParam, LPARAM lParam)
{
	SMADD_INFO2 *si = (SMADD_INFO2 *) lParam;
	if (si == NULL || si->cbSize < sizeof(SMADD_INFO2))
		return FALSE;

	Module *m = GetContactModule(si->hContact, si->Protocolname);
	if (m == NULL)
		return FALSE;

	si->NumberOfVisibleSmileys = si->NumberOfSmileys = m->emoticons.getCount();

	return TRUE;
}


void PreMultiply(HBITMAP hBitmap)
{
	BYTE *p = NULL;
	DWORD dwLen;
	int width, height, x, y;
	BITMAP bmp;
	BYTE alpha;
	BOOL transp = FALSE;

	GetObject(hBitmap, sizeof(bmp), &bmp);
	width = bmp.bmWidth;
	height = bmp.bmHeight;
	dwLen = width * height * 4;
	p = (BYTE *)malloc(dwLen);
	if (p != NULL)
	{
		GetBitmapBits(hBitmap, dwLen, p);

		for (y = 0; y < height; ++y)
		{
			BYTE *px = p + width * 4 * y;

			for (x = 0; x < width; ++x)
			{
				alpha = px[3];

				if (alpha < 255)
				{
					transp  = TRUE;

					px[0] = px[0] * alpha/255;
					px[1] = px[1] * alpha/255;
					px[2] = px[2] * alpha/255;
				}

				px += 4;
			}
		}

		if (transp)
			dwLen = SetBitmapBits(hBitmap, dwLen, p);
		free(p);
	}
}


Module::~Module()
{
	MIR_FREE(name);
	MIR_FREE(path);

	for(int i = 0; i < emoticons.getCount(); i++)
	{
		delete emoticons[i];
	}
	emoticons.destroy();
}


Emoticon::~Emoticon()
{
	MIR_FREE(name);
	MIR_FREE(description);

	for(int j = 0; j < MAX_REGS(service); j++)
		MIR_FREE(service[j])

	for(int i = 0; i < texts.getCount(); i++)
	{
		mir_free(texts[i]);
	}
	texts.destroy();
}


EmoticonPack::~EmoticonPack()
{
	MIR_FREE(name);
	MIR_FREE(path);
	MIR_FREE(creator);
	MIR_FREE(updater_URL);

	for(int i = 0; i < images.getCount(); i++)
	{
		delete images[i];
	}
	images.destroy();
}


EmoticonImage::~EmoticonImage()
{
	Release();

	MIR_FREE(name);
	MIR_FREE(module);
	MIR_FREE(relPath);
	MIR_FREE(url);
}


BOOL EmoticonImage::isAvaiableFor(char *module)
{
	char tmp[1024];
	if (module == NULL)
		mir_snprintf(tmp, MAX_REGS(tmp), "%s\\%s", pack->path, relPath);
	else
		mir_snprintf(tmp, MAX_REGS(tmp), "%s\\%s\\%s", pack->path, module, relPath);

	return FileExists(tmp);
}


BOOL EmoticonImage::isAvaiable()
{
	return isAvaiableFor(module);
}


void EmoticonImage::Download()
{
	if (url == NULL)
		return;

	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "%s\\%s", pack->path, relPath);
	if (FileExists(tmp))
		return;

	downloadQueue->AddIfDontHave(1000, (HANDLE) this);
}

struct ANIMATED_GIF_DATA
{
	FIMULTIBITMAP *multi;
	FIBITMAP *dib;
	int frameCount;
	RGBQUAD background;
	int width;
	int height;
	BOOL transparent;

	struct {
		int top;
		int left;
		int width;
		int height;
		int disposal_method;
	} frame;
};

BOOL AnimatedGifGetData(ANIMATED_GIF_DATA &ag)
{
	FIBITMAP *page = fei->FI_LockPage(ag.multi, 0);
	if (page == NULL)
		return FALSE;
	
	// Get info
	FITAG *tag = NULL;
	if (!fei->FI_GetMetadata(FIMD_ANIMATION, page, "LogicalWidth", &tag))
		goto ERR;
	ag.width = *(WORD *)fei->FI_GetTagValue(tag);
	
	if (!fei->FI_GetMetadata(FIMD_ANIMATION, page, "LogicalHeight", &tag))
		goto ERR;
	ag.height = *(WORD *)fei->FI_GetTagValue(tag);
	
	if (fei->FI_HasBackgroundColor(page))
		fei->FI_GetBackgroundColor(page, &ag.background);

	fei->FI_UnlockPage(ag.multi, page, FALSE);
	return TRUE;

ERR:
	fei->FI_UnlockPage(ag.multi, page, FALSE);
	return FALSE;
}


void AnimatedGifMountFrame(ANIMATED_GIF_DATA &ag, int page)
{
	FIBITMAP *dib = fei->FI_LockPage(ag.multi, page);
	if (dib == NULL)
		return;

	FITAG *tag = NULL;
	if (fei->FI_GetMetadata(FIMD_ANIMATION, dib, "FrameLeft", &tag))
		ag.frame.left = *(WORD *)fei->FI_GetTagValue(tag);
	else
		ag.frame.left = 0;

	if (fei->FI_GetMetadata(FIMD_ANIMATION, dib, "FrameTop", &tag))
		ag.frame.top = *(WORD *)fei->FI_GetTagValue(tag);
	else
		ag.frame.top = 0;

	if (fei->FI_GetMetadata(FIMD_ANIMATION, dib, "DisposalMethod", &tag))
		ag.frame.disposal_method = *(BYTE *)fei->FI_GetTagValue(tag);
	else
		ag.frame.disposal_method = 0;

	ag.frame.width  = fei->FI_GetWidth(dib);
	ag.frame.height = fei->FI_GetHeight(dib);


	//decode page
	int palSize = fei->FI_GetColorsUsed(dib);
	RGBQUAD *pal = fei->FI_GetPalette(dib);
	BOOL have_transparent = FALSE;
	int transparent_color = -1;
	if( fei->FI_IsTransparent(dib) ) {
		int count = fei->FI_GetTransparencyCount(dib);
		BYTE *table = fei->FI_GetTransparencyTable(dib);
		for( int i = 0; i < count; i++ ) {
			if( table[i] == 0 ) {
				ag.transparent = TRUE;
				have_transparent = TRUE;
				transparent_color = i;
				break;
			}
		}
	}

	//copy page data into logical buffer, with full alpha opaqueness
	for( int y = 0; y < ag.frame.height; y++ ) {
		RGBQUAD *scanline = (RGBQUAD *)fei->FI_GetScanLine(ag.dib, ag.height - (y + ag.frame.top) - 1) + ag.frame.left;
		BYTE *pageline = fei->FI_GetScanLine(dib, ag.frame.height - y - 1);
		for( int x = 0; x < ag.frame.width; x++ ) {
			if( !have_transparent || *pageline != transparent_color ) {
				*scanline = pal[*pageline];
				scanline->rgbReserved = 255;
			}
			scanline++;
			pageline++;
		}
	}

	fei->FI_UnlockPage(ag.multi, dib, FALSE);
}



HBITMAP LoadAnimatedGifFrame(char *filename, int frame, DWORD *transp)
{
	HBITMAP ret = NULL;
	ANIMATED_GIF_DATA ag = {0};
	int x, y, i;

	FREE_IMAGE_FORMAT fif = fei->FI_GetFileType(filename, 0);
	if(fif == FIF_UNKNOWN)
		fif = fei->FI_GetFIFFromFilename(filename);

	ag.multi = fei->FI_OpenMultiBitmap(fif, filename, FALSE, TRUE, FALSE, GIF_LOAD256);
	if (ag.multi == NULL)
		return NULL;

	ag.frameCount = fei->FI_GetPageCount(ag.multi);
	if (ag.frameCount <= 1)
		goto ERR;

	if (!AnimatedGifGetData(ag))
		goto ERR;

	//allocate entire logical area
	ag.dib = fei->FI_Allocate(ag.width, ag.height, 32, 0, 0, 0);
	if (ag.dib == NULL)
		goto ERR;

	//fill with background color to start
	for (y = 0; y < ag.height; y++) 
	{
		RGBQUAD *scanline = (RGBQUAD *) fei->FI_GetScanLine(ag.dib, y);
		for (x = 0; x < ag.width; x++)
			*scanline++ = ag.background;
	}

	for (i = 0; i <= frame && i < ag.frameCount; i++)
		AnimatedGifMountFrame(ag, i);
	
	ret = fei->FI_CreateHBITMAPFromDIB(ag.dib);

	if (transp != NULL)
		*transp = ag.transparent;

ERR:
	if (ag.multi != NULL)
		fei->FI_CloseMultiBitmap(ag.multi, 0);
	if (ag.dib != NULL)
		fei->FI_Unload(ag.dib);

	return ret;
}


HBITMAP LoadFlashBitmap(char *filename, int frame = 0)
{
	typedef HRESULT (WINAPI *LPAtlAxAttachControl)(IUnknown* pControl, HWND hWnd, IUnknown** ppUnkContainer);
	static LPAtlAxAttachControl AtlAxAttachControl3 = NULL;

	if (AtlAxAttachControl3 == (LPAtlAxAttachControl) -1)
	{
		return NULL;
	}
	else if (AtlAxAttachControl3 == NULL)
	{
		HMODULE atl = LoadLibrary(_T("atl"));
		if (atl == NULL)
		{
			AtlAxAttachControl3 = (LPAtlAxAttachControl) -1;
			return NULL;
		}
		void* init = GetProcAddress(atl, "AtlAxWinInit"); 
		AtlAxAttachControl3 = (LPAtlAxAttachControl) GetProcAddress(atl, "AtlAxAttachControl"); 
		if (init == NULL || AtlAxAttachControl3 == NULL)
		{
			AtlAxAttachControl3 = (LPAtlAxAttachControl) -1;
			return NULL;
		}
		_asm call init;
	}

	int width = 40;
	int height = 40;
	IShockwaveFlash *flash = NULL;
	IViewObjectEx *viewObject = NULL;
	HRESULT hr;
	HWND hWindow = NULL;
	HBITMAP hBmp = NULL;
	HDC hdc = NULL;
	BOOL succeded = FALSE;
	double val;

	hWindow = CreateWindow(_T("AtlAxWin"), _T(""), WS_POPUP, 0, 0, width, height, NULL, (HMENU) 0, hInst, NULL);
	if (hWindow == NULL) goto err;

	hr = CoCreateInstance(__uuidof(ShockwaveFlash), 0, CLSCTX_INPROC_SERVER, __uuidof(IShockwaveFlash), (void **) &flash); 
	if (FAILED(hr)) goto err;

	hr = AtlAxAttachControl3(flash, hWindow, 0);
	if (FAILED(hr)) goto err;

	{
		WCHAR *tmp = mir_a2u(filename);
		BSTR url = SysAllocString(tmp);

		hr = flash->LoadMovie(0, url);

		SysFreeString(url);
		mir_free(tmp);
	}

	if (FAILED(hr)) goto err;

	hr = flash->TGetPropertyAsNumber(L"/", 8, &val);
	if (FAILED(hr)) goto err;
	width = (int)(val + 0.5);

	hr = flash->TGetPropertyAsNumber(L"/", 9, &val);
	if (FAILED(hr)) goto err;
	height = (int)(val + 0.5);

	MoveWindow(hWindow, 0, 0, width, height, FALSE);

	hr = flash->GotoFrame(frame);
	if (FAILED(hr)) goto err;

	hr = flash->QueryInterface(__uuidof(IViewObjectEx), (void **)&viewObject);
	if (FAILED(hr)) goto err;

	hBmp = CreateBitmap32(width, height);

	hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, hBmp);
	SetBkMode(hdc, TRANSPARENT);

	{
		RECTL rectl = { 0, 0, width, height };
		hr = viewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, &rectl, &rectl, NULL, NULL); 
		if (FAILED(hr)) goto err;
	}

	succeded = TRUE;

err:
	if (hdc != NULL)
		DeleteDC(hdc);

	if (viewObject != NULL)
		viewObject->Release();

	if (flash != NULL)
		flash->Release();

	if (hWindow != NULL)
		DestroyWindow(hWindow);

	if (succeded)
	{
		return hBmp;
	}
	else
	{
		if (hBmp != NULL)
			DeleteObject(hBmp);

		return NULL;
	}
}

void EmoticonImage::Load(int &max_height, int &max_width)
{
	if (img != NULL)
	{
		BITMAP bmp;
		GetObject(img, sizeof(bmp), &bmp);

		max_height = max(max_height, bmp.bmHeight);
		max_width = max(max_width, bmp.bmWidth);

		return;
	}

	DWORD transp;
	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "%s\\%s", pack->path, relPath);

	if (strcmp(&tmp[strlen(tmp)-4], ".swf") == 0)
	{
		img = LoadFlashBitmap(tmp, selectionFrame);
		transp = TRUE;
	}
	else
	{
		// Try to get a frame
		if (selectionFrame > 1)
			img = LoadAnimatedGifFrame(tmp, selectionFrame, &transp);

		if (img == NULL)
			img = (HBITMAP) CallService(MS_AV_LOADBITMAP32, (WPARAM) &transp, (LPARAM) tmp);
	}

	if (img == NULL)
		return;

	BITMAP bmp;
	if (!GetObject(img, sizeof(bmp), &bmp))
	{
		DeleteObject(img);
		img = NULL;
		return;
	}

	transparent = (bmp.bmBitsPixel == 32 && transp);
	if (transparent)
		PreMultiply(img);

	max_height = max(max_height, bmp.bmHeight);
	max_width = max(max_width, bmp.bmWidth);
}


void EmoticonImage::Release()
{
	if (img == NULL)
		return;

	DeleteObject(img);
	img = NULL;
}




Contact * GetContact(HANDLE hContact)
{
	if (hContact == NULL)
		return NULL;
	
	hContact = GetRealContact(hContact);

	// Check if already loaded
	for(int i = 0; i < contacts.getCount(); i++)
		if (contacts[i]->hContact == hContact)
			return contacts[i];

	Contact *c = new Contact(hContact);
	contacts.insert(c);

	// Get from db
	BOOL pack = FALSE;
	c->lastId = -1;
	char setting[256];
	while(TRUE) 
	{
		c->lastId++;

		mir_snprintf(setting, MAX_REGS(setting), "%d_Text", c->lastId);
		DBVARIANT dbv_text = {0};
		if (DBGetContactSettingTString(hContact, "CustomSmileys", setting, &dbv_text))
			break;

		mir_snprintf(setting, MAX_REGS(setting), "%d_Path", c->lastId);
		DBVARIANT dbv_path = {0};
		if (DBGetContactSettingString(hContact, "CustomSmileys", setting, &dbv_path))
		{
			pack = TRUE;
			DBFreeVariant(&dbv_text);
			continue;
		}

		mir_snprintf(setting, MAX_REGS(setting), "%d_FirstReceived", c->lastId);
		DWORD firstReceived = DBGetContactSettingDword(hContact, "CustomSmileys", setting, 0);
		
		if (!FileExists(dbv_path.pszVal))
		{
			pack = TRUE;
		}
		else if (GetCustomEmoticon(c, dbv_text.ptszVal) != NULL)
		{
			pack = TRUE;
		}
		else
		{
			CustomEmoticon *ce = new CustomEmoticon();
			ce->text = mir_tstrdup(dbv_text.ptszVal);
			ce->path = mir_strdup(dbv_path.pszVal);
			ce->firstReceived = firstReceived;

			c->emoticons.insert(ce);
		}

		DBFreeVariant(&dbv_path);
		DBFreeVariant(&dbv_text);
	}

	c->lastId--;

	if (pack)
	{
		// Re-store then
		int i;
		for(i = 0; i < c->emoticons.getCount(); i++)
		{
			CustomEmoticon *ce = c->emoticons[i];

			mir_snprintf(setting, MAX_REGS(setting), "%d_Text", i);
			DBWriteContactSettingTString(c->hContact, "CustomSmileys", setting, ce->text);

			mir_snprintf(setting, MAX_REGS(setting), "%d_Path", i);
			DBWriteContactSettingString(c->hContact, "CustomSmileys", setting, ce->path);

			mir_snprintf(setting, MAX_REGS(setting), "%d_FirstReceived", i);
			DBWriteContactSettingDword(c->hContact, "CustomSmileys", setting, ce->firstReceived);
		}
		for(int j = i; j <= c->lastId; j++)
		{
			mir_snprintf(setting, MAX_REGS(setting), "%d_Text", j);
			DBDeleteContactSetting(c->hContact, "CustomSmileys", setting);

			mir_snprintf(setting, MAX_REGS(setting), "%d_Path", j);
			DBDeleteContactSetting(c->hContact, "CustomSmileys", setting);

			mir_snprintf(setting, MAX_REGS(setting), "%d_FirstReceived", j);
			DBDeleteContactSetting(c->hContact, "CustomSmileys", setting);
		}

		c->lastId = i - 1;
	}

	return c;
}


CustomEmoticon *GetCustomEmoticon(Contact *c, TCHAR *text)
{
	for(int i = 0; i < c->emoticons.getCount(); i++)
	{
		if (lstrcmp(c->emoticons[i]->text, text) == 0)
			return c->emoticons[i];
	}
	return NULL;
}


int SingleHexToDecimal(char c)
{
	if ( c >= '0' && c <= '9' ) return c-'0';
	if ( c >= 'a' && c <= 'f' ) return c-'a'+10;
	if ( c >= 'A' && c <= 'F' ) return c-'A'+10;
	return -1;
}


void UrlDecode(char* str)
{
	char* s = str, *d = str;

	while( *s )
	{
		if ( *s == '%' ) {
			int digit1 = SingleHexToDecimal( s[1] );
			if ( digit1 != -1 ) {
				int digit2 = SingleHexToDecimal( s[2] );
				if ( digit2 != -1 ) {
					s += 3;
					*d++ = (char)(( digit1 << 4 ) | digit2);
					continue;
		}	}	}
		*d++ = *s++;
	}

	*d = 0;
}


void CreateCustomSmiley(Contact *contact, TCHAR *fullpath)
{
	char *path = mir_t2a(fullpath);

	// Get smiley text
	char enc[1024];
	char *start = strrchr(path, '\\');
	if (start == NULL)
		return;
	start++;
	char *end = strrchr(path, '.');
	if (end == NULL || end < start)
		return;
	size_t enclen = min(end - start, MAX_REGS(enc));
	strncpy(enc, start, enclen);
	enc[enclen] = '\0';

	UrlDecode(enc);

	enclen = strlen(enc);
	size_t len = Netlib_GetBase64DecodedBufferSize(enclen) + 1;
	char *tmp = (char *) malloc(len * sizeof(char));

	NETLIBBASE64 nlb = { enc, enclen, (PBYTE) tmp, len };
	if (CallService(MS_NETLIB_BASE64DECODE, 0, (LPARAM) &nlb) == 0) 
		return;
	tmp[nlb.cbDecoded] = 0; 

#ifdef _UNICODE
	TCHAR *text = mir_utf8decodeW(tmp);
#else
	mir_utf8decode(tmp, NULL);
	TCHAR *text = mir_a2t(tmp);
#endif

	free(tmp);

	// Create it
	CustomEmoticon *ce = GetCustomEmoticon(contact, text);
	if (ce != NULL)
	{
		MIR_FREE(ce->path);
		ce->path = path;
	}
	else
	{
		ce = new CustomEmoticon();
		ce->text = text;
		ce->path = path;

		contact->emoticons.insert(ce);
		contact->lastId++;
	}

	// Store in DB
	char setting[256];
	mir_snprintf(setting, MAX_REGS(setting), "%d_Text", contact->lastId);
	DBWriteContactSettingTString(contact->hContact, "CustomSmileys", setting, ce->text);

	mir_snprintf(setting, MAX_REGS(setting), "%d_Path", contact->lastId);
	DBWriteContactSettingString(contact->hContact, "CustomSmileys", setting, ce->path);

	mir_snprintf(setting, MAX_REGS(setting), "%d_FirstReceived", contact->lastId);
	DBWriteContactSettingDword(contact->hContact, "CustomSmileys", setting, (DWORD) time(NULL));

	NotifyEventHooks(hChangedEvent, (WPARAM) contact->hContact, 0);
}


int LoadContactSmileysService(WPARAM wParam, LPARAM lParam)
{
	SMADD_CONT *sc = (SMADD_CONT *) lParam;
	if (sc == NULL || sc->cbSize < sizeof(SMADD_CONT) || sc->path == NULL || sc->hContact == NULL || sc->type < 0 || sc->type > 1)
		return 0;

	Contact *c = GetContact(sc->hContact);

	if (sc->type == 0)
	{
		TCHAR filename[1024];
		mir_sntprintf(filename, MAX_REGS(filename), _T("%s\\*.*"), sc->path);

		WIN32_FIND_DATA ffd = {0};
		HANDLE hFFD = FindFirstFile(filename, &ffd);
		if (hFFD != INVALID_HANDLE_VALUE)
		{
			do
			{
				mir_sntprintf(filename, MAX_REGS(filename), _T("%s\\%s"), sc->path, ffd.cFileName);

				if (!FileExists(filename))
					continue;

				if (!isValidExtension(ffd.cFileName))
					continue;

				CreateCustomSmiley(c, filename);
			}
			while(FindNextFile(hFFD, &ffd));

			FindClose(hFFD);
		}
	}
	else if (sc->type == 1)
	{
		if (FileExists(sc->path))
			CreateCustomSmiley(c, sc->path);
	}

	return 0;
}


BOOL CreatePath(const char *path) 
{
	char folder[1024];
	strncpy(folder, path, MAX_REGS(folder));
	folder[MAX_REGS(folder)-1] = '\0';

	char *p = folder;
	if (p[0] && p[1] && p[1] == ':' && p[2] == '\\') p += 3; // skip drive letter

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


BOOL GetFile(char *url, char *dest, int recurse_count = 0) 
{
	if(recurse_count > 5) 
	{
		log("GetFile: error, too many redirects");
		return FALSE;
	}

	// Assert path exists
	char *p = strrchr(dest, '\\');
	if (p != NULL)
	{
		*p = '\0';
		if (!CreatePath(dest)) 
		{
			log("GetFile: error creating temp folder, code %d", (int) GetLastError());
			*p = '\\';
			return FALSE;
		}
		*p = '\\';
	}

	NETLIBHTTPREQUEST req = {0};

	req.cbSize = sizeof(req);
	req.requestType = REQUEST_GET;
	req.szUrl = url;
	req.flags = NLHRF_NODUMP;

	NETLIBHTTPREQUEST *resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)hNetlibUser, (LPARAM)&req);
	if (resp == NULL) 
	{
		log("GetFile: failed to download \"%s\" : error %d", url, (int) GetLastError());
		return FALSE;
	}

	BOOL ret = FALSE;

	if (resp->resultCode == 200) 
	{
		HANDLE hSaveFile = CreateFileA(dest, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hSaveFile != INVALID_HANDLE_VALUE) 
		{
			unsigned long bytes_written = 0;
			if (WriteFile(hSaveFile, resp->pData, resp->dataLength, &bytes_written, NULL) == TRUE) 
				ret = TRUE;
			else 
				log("GetFile: error writing file \"%s\", code %d", dest, (int) GetLastError());

			CloseHandle(hSaveFile);
		} 
		else 
		{
			log("GetFile: error creating file \"%s\", code %d", dest, (int) GetLastError());
		}
	}
	else if (resp->resultCode >= 300 && resp->resultCode < 400) 
	{
		// get new location
		for (int i = 0; i < resp->headersCount; i++) 
		{
			if (strcmp(resp->headers[i].szName, "Location") == 0) 
			{
				ret = GetFile(resp->headers[i].szValue, dest, recurse_count + 1);
				break;
			}
		}
	}
	else 
	{
		log("GetFile: failed to download \"%s\" : Invalid response, code %d", url, resp->resultCode);
	}

	CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);

	return ret;
}


void DownloadCallback(HANDLE hContact, void *param)
{
	EmoticonImage *img = (EmoticonImage *) hContact;
	if (img == NULL || img->url == NULL)
		return;

	char tmp[1024];
	mir_snprintf(tmp, MAX_REGS(tmp), "%s\\%s", img->pack->path, img->relPath);
	if (FileExists(tmp))
		return;

	if (!GetFile(img->url, tmp))
	{
		// Avoid further downloads
		MIR_FREE(img->url);
	}
}


void log(const char *fmt, ...)
{
    va_list va;
    char text[1024];

    va_start(va, fmt);
    mir_vsnprintf(text, sizeof(text), fmt, va);
    va_end(va);

	CallService(MS_NETLIB_LOG, (WPARAM) hNetlibUser, (LPARAM) text);
}