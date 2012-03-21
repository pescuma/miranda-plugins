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
	PLUGIN_MAKE_VERSION(0,3,0,1),
	"Emoticons",
	"Ricardo Pescuma Domenecci",
	"",
	"© 2008-2009 Ricardo Pescuma Domenecci",
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
struct MM_INTERFACE mmi;
struct UTF8_INTERFACE utfi;

HANDLE hHooks[4] = {0};
HANDLE hServices[8] = {0};
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
int DefaultMetaChanged(WPARAM wParam, LPARAM lParam);

int ReplaceEmoticonsService(WPARAM wParam, LPARAM lParam);
int GetInfo2Service(WPARAM wParam, LPARAM lParam);
int ShowSelectionService(WPARAM wParam, LPARAM lParam);
int LoadContactSmileysService(WPARAM wParam, LPARAM lParam);
int BatchParseService(WPARAM wParam, LPARAM lParam);
int BatchFreeService(WPARAM wParam, LPARAM lParam);
int ParseService(WPARAM wParam, LPARAM lParam);
int ParseWService(WPARAM wParam, LPARAM lParam);

TCHAR *GetText(RichEditCtrl &rec, int start, int end);
const char * GetProtoID(const char *proto);


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
	BOOL __inverse = (__old_sel.cpMin >= LOWORD(SendMessage(rec.hwnd, EM_CHARFROMPOS, 0, (LPARAM) &__caretPos)));	\
	BOOL __ready_only = (GetWindowLong(rec.hwnd, GWL_STYLE) & ES_READONLY);		\
	if (__ready_only)															\
		SendMessage(rec.hwnd, EM_SETREADONLY, FALSE, 0)

#define START_RICHEDIT(rec)														\
	if (__ready_only)															\
		SendMessage(rec.hwnd, EM_SETREADONLY, TRUE, 0);							\
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
	_T("https:/"), 
	_T("ftp:/"), 
	_T("irc:/"), 
	_T("gopher:/"), 
	_T("file:/"), 
	_T("www."), 
	_T("www2."),
	_T("ftp."),
	_T("irc.")
};


static TCHAR *urlChars = _T("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789:/?&=%._-#");

struct FlashData
{
	TCHAR url[1024];
	TCHAR flashVars[1024];
	int width;
	int height;

	FlashData() : width(0), height(0)
	{
		url[0] = 0;
		flashVars[0] = 0;
	}
};


class VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash) =0;

protected:
	virtual ~VideoInfo()
	{
	}


	bool startsWith(const TCHAR *url, const TCHAR *start, size_t startLen = 0)
	{
		if (startLen <= 0)
			startLen = lstrlen(start);

		return _tcsnicmp(url, start, startLen) == 0;
	}

	int urlStartsWith(const TCHAR *url, const TCHAR **start, size_t len)
	{
		for(size_t i = 0; i < len; i++)
		{
			int ret = urlStartsWith(url, start[i]);
			if (ret >= 0)
				return ret;
		}
		return -1;
	}

	/// @return the first pos in the text after the start, or -1 if not found
	int urlStartsWith(const TCHAR *url, const TCHAR *start)
	{
		static TCHAR *possibleStarts[] = { _T("http://www."), _T("http://"), _T("https://www."), _T("https://") };

		size_t startLen = lstrlen(start);

		for(int i = 0; i < MAX_REGS(possibleStarts); i++)
		{
			size_t plen = lstrlen(possibleStarts[i]);

			if (!startsWith(url, possibleStarts[i], plen))
				continue;

			if (startsWith(&url[plen], start, startLen))
				return plen + startLen;
		}

		if (startsWith(url, start, startLen))
			return startLen;

		return -1;
	}

	int findArgument(const TCHAR *url, const TCHAR *argument)
	{
		size_t len = lstrlen(argument);
		int startPos = max(0, _tcschr(url, _T('?')) - url);
		while(true)
		{
			int pos = _tcsstr(&url[startPos], argument) - url;
			if (pos < 0)
				return -1;

			if (url[pos + len] != _T('='))
			{
				startPos += pos + len;
				continue;
			}

			if (pos > 0 && url[pos - 1] != _T('?') && url[pos - 1] != _T('&'))
			{
				startPos += pos + len;
				continue;
			}

			return pos + len + 1;
		}
	}

	bool getArgument(const TCHAR *url, const TCHAR *argument, TCHAR *out, size_t outLen)
	{
		int pos = findArgument(url, argument);
		if (pos < 0)
			return false;

		url = &url[pos];

		removeOtherArguments(url, out, outLen);
		return out[0] != _T('\0');
	}

	void removeOtherArguments(const TCHAR *url, TCHAR *out, size_t outLen)
	{
		int pos = _tcschr(url, _T('&')) - url;
		if (pos < 0)
			pos = outLen;

		lstrcpyn(out, url, min(pos+1, outLen));
	}
};

class YoutubeVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("youtube.com/watch?") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		TCHAR id[128];
		if (!getArgument(&url[pos], _T("v"), id, MAX_REGS(id)))
			return false;

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://youtube.com/v/%s"), id);
		flash->width = 425;
		flash->height = 355;
		return true;
	}
};

class AnimotoVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("animoto.com/play/") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		TCHAR id[128];
		removeOtherArguments(&url[pos], id, MAX_REGS(id));

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://animoto.com/swf/animotoplayer-3.15.swf"));
		mir_sntprintf(flash->flashVars, MAX_REGS(flash->flashVars), _T("autostart=false&file=http://s3-p.animoto.com/Video/%s.flv&menu=true&volume=100&quality=high&repeat=false&usekeys=false&showicons=true&showstop=false&showdigits=false&usecaptions=false&bufferlength=12&overstretch=false&remainonlastframe=true&backcolor=0x000000&frontcolor=0xBBBBBB&lightcolor=0xFFFFFF&screencolor=0x000000"), id);
		flash->width = 432;
		flash->height = 263;
		return true;
	}
};

class CleVRVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("clevr.com/pano/") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		TCHAR id[128];
		removeOtherArguments(&url[pos], id, MAX_REGS(id));

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://s3.clevr.com/CleVR"));
		mir_sntprintf(flash->flashVars, MAX_REGS(flash->flashVars), _T("xmldomain=http://www.clevr.com/&mov=%s"), id);
		flash->width = 450;
		flash->height = 350;
		return true;
	}
};

class MagTooVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("magtoo.com/tour.do?method=viewMagShow&") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		TCHAR id[128];
		if (!getArgument(&url[pos], _T("id"), id, MAX_REGS(id)))
			return false;

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://www.magtoo.com/tour.do?method=FlashVarsSender&fl_type=magtooPanorama.swf"));
		mir_sntprintf(flash->flashVars, MAX_REGS(flash->flashVars), _T("tempID=%s&serverURL=http://www.magtoo.com"), id);
		flash->width = 600;
		flash->height = 360;
		return true;
	}
};

class VimeoVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("vimeo.com/") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		TCHAR id[128];
		removeOtherArguments(&url[pos], id, MAX_REGS(id));

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://www.vimeo.com/moogaloop.swf?clip_id=%s&amp;server=www.vimeo.com&amp;show_title=1&amp;show_byline=1&amp;show_portrait=0&amp;color=&amp;fullscreen=0"), id);
		flash->width = 400;
		flash->height = 300;
		return true;
	}
};

class PicasaVideo : public VideoInfo
{
public:
	virtual bool convertToFlash(const TCHAR *url, FlashData *flash)
	{
		const static TCHAR *base[] = { _T("picasaweb.google.com/") };

		int pos = urlStartsWith(url, base, MAX_REGS(base));
		if (pos < 0)
			return false;

		int tmp = _tcschr(&url[pos], _T('/')) - url;
		if (tmp < 0)
			return false;

		TCHAR user[256];
		lstrcpyn(user, &url[pos], min(tmp-pos+1, MAX_REGS(user)));

		pos = tmp+1;

		tmp = _tcschr(&url[pos], _T('?')) - url;
		if (tmp < 0)
			return false;

		TCHAR album[256];
		lstrcpyn(album, &url[pos], min(tmp-pos+1, MAX_REGS(album)));

		pos = tmp+1;

		TCHAR authkey[256];
		if (!getArgument(&url[pos], _T("authkey"), authkey, MAX_REGS(authkey)))
			return false;

		mir_sntprintf(flash->url, MAX_REGS(flash->url), _T("http://picasaweb.google.com/s/c/bin/slideshow.swf"));
		mir_sntprintf(flash->flashVars, MAX_REGS(flash->flashVars), _T("host=picasaweb.google.com&RGB=0x000000&feed=http%%3A%%2F%%2Fpicasaweb.google.com%%2Fdata%%2Ffeed%%2Fapi%%2Fuser%%2F%s%%2Falbum%%2F%s%%3Fkind%%3Dphoto%%26alt%%3Drss%%26authkey%%3D%s"), user, album, authkey);
		flash->width = 400;
		flash->height = 267;
		return true;
	}
};

static VideoInfo *videos[] = { 
	new YoutubeVideo(),
	new AnimotoVideo(),
	new CleVRVideo(),
	new MagTooVideo(),
	new VimeoVideo(),
	new PicasaVideo()

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

	CHECK_VERSION("Emoticons")

	// TODO Assert results here
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);
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


BOOL isURL(TCHAR *text, int text_len)
{
	for (int j = 0; j < MAX_REGS(webs); j++)
	{
		TCHAR *txt = webs[j];
		int len = lstrlen(txt);

		if (text_len < len)
			continue;

		if (_tcsncmp(text, txt, len) != 0)
			continue;

		return TRUE;
	}

	return FALSE;
}


int findURLEnd(TCHAR *text, int text_len)
{
	int i;
	for(i = 0; i < text_len; i++)
		if (_tcschr(urlChars, text[i]) == NULL)
			break;
	return i;
}


BOOL isVideo(const TCHAR *url, size_t urlLen, FlashData *flash)
{
	TCHAR tmp[1024];
	lstrcpyn(tmp, url, min(MAX_REGS(tmp), urlLen+1));

	for (int j = 0; j < MAX_REGS(videos); j++)
	{
		if (videos[j]->convertToFlash(tmp, flash)) 
			return TRUE;
	}
	
	return FALSE;
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
	{
		metacontacts_proto = (char *) CallService(MS_MC_GETPROTOCOLNAME, 0, 0);
		hHooks[3] = HookEvent(ME_MC_DEFAULTTCHANGED, &DefaultMetaChanged);
	}

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
	hServices[4] = CreateServiceFunction(MS_SMILEYADD_BATCHPARSE, BatchParseService);
	hServices[5] = CreateServiceFunction(MS_SMILEYADD_BATCHFREE, BatchFreeService);
	hServices[6] = CreateServiceFunction(MS_SMILEYADD_PARSE, ParseService);
	hServices[7] = CreateServiceFunction(MS_SMILEYADD_PARSEW, ParseWService);

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


struct EmoticonFound
{
	char path[1024];
	int len;
	TCHAR *text;
	HBITMAP img;
	BOOL custom;
};


BOOL FindEmoticonForwards(EmoticonFound &found, Contact *contact, Module *module, TCHAR *text, int text_len, int pos)
{
	if (pos >= text_len)
		return FALSE;

	found.path[0] = 0;
	found.len = -1;
	found.text = NULL;
	found.img = NULL;
	found.custom = FALSE;

	// Lets shit text to current pos
	TCHAR prev_char = (pos == 0 ? _T('\0') : text[pos - 1]);
	text = &text[pos];
	text_len -= pos;

	// This are needed to allow 2 different emoticons that end the same way

	// Replace normal emoticons
	if (!opts.only_replace_isolated || prev_char == _T('\0') || _istspace(prev_char))
	{	
		for(int i = 0; i < module->emoticons.getCount(); i++)
		{
			Emoticon *e = module->emoticons[i];

			for(int j = 0; j < e->texts.getCount(); j++)
			{
				TCHAR *txt = e->texts[j];
				int len = lstrlen(txt);
				if (text_len < len)
					continue;

				if (len <= found.len)
					continue;

				if (_tcsncmp(text, txt, len) != 0)
					continue;

				if (opts.only_replace_isolated && text_len > len && !_istspace(text[len]))
					continue;

				if (e->img == NULL)
					found.path[0] = '\0';
				else
					mir_snprintf(found.path, MAX_REGS(found.path), "%s\\%s", e->img->pack->path, e->img->relPath);

				found.len = len;
				found.text = txt;

				if (e->img != NULL)
				{
					e->img->Load();
					found.img = e->img->img;
				}
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
			if (text_len < len)
				continue;

			if (len <= found.len)
				continue;

			if (_tcsncmp(text, txt, len) != 0)
				continue;

			mir_snprintf(found.path, MAX_REGS(found.path), "%s", e->path);
			found.len = len;
			found.text = txt;
			found.custom = TRUE;
		}
	}

	return (found.len > 0 && found.path[0] != '\0');
}


int ReplaceEmoticon(RichEditCtrl &rec, int pos, EmoticonFound &found)
{
	int ret = 0;

	// Found ya
	CHARRANGE sel = { pos, pos + found.len };
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

		TCHAR *path = mir_a2t(found.path);
		if (InsertAnimatedSmiley(rec.hwnd, path, cf.crBackColor, 0, 0, found.text, NULL))
		{
			ret = - found.len + 1;
		}
		MIR_FREE(path);
	}
	else
	{
		OleImage *img = new OleImage(found.path, found.text, found.text);
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
			ret = - found.len + 1;
		}

		clientSite->Release();
		img->Release();
	}

	return ret;
}


struct StreamData
{
	const char *text;
	size_t pos;
	size_t len;

	StreamData(const char *aText)
	{
		text = aText;
		len = strlen(aText);
		pos = 0;
	}
};

static DWORD CALLBACK StreamInEvents(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	StreamData *data = (StreamData *) dwCookie;

	*pcb = min(cb, data->len - data->pos);
	if (*pcb > 0)
	{
		CopyMemory(pbBuff, &data->text[data->pos], *pcb);
		data->pos += *pcb;
	}

	return 0;
}


int AddVideo(RichEditCtrl &rec, int pos, const FlashData *flash)
{
	int ret = 0;
	
	if (has_anismiley)
	{
		CHARRANGE sel = { pos-1, pos };
		SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);

		CHARFORMAT2 cf;
		memset(&cf, 0, sizeof(CHARFORMAT2));
		cf.cbSize = sizeof(CHARFORMAT2);
		cf.dwMask = CFM_ALL2;
		SendMessage(rec.hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

		sel.cpMin = sel.cpMax = pos;
		SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
		
		if (cf.dwEffects & CFE_AUTOBACKCOLOR)
		{
			cf.crBackColor = SendMessage(rec.hwnd, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_WINDOW));
			SendMessage(rec.hwnd, EM_SETBKGNDCOLOR, 0, cf.crBackColor);
		}
		
		if (InsertAnimatedSmiley(rec.hwnd, flash->url, cf.crBackColor, flash->width, flash->height, _T(""), flash->flashVars))
		{
			ret++;
		
			sel.cpMin = sel.cpMax = pos;
			SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
			SendMessage(rec.hwnd, EM_REPLACESEL, FALSE, (LPARAM) _T("\n"));
			ret++;

			sel.cpMin = sel.cpMax = pos + 2;
			SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
			SendMessage(rec.hwnd, EM_REPLACESEL, FALSE, (LPARAM) _T("\n"));
			ret++;

			sel.cpMin = pos;
			sel.cpMax = pos + 3;
			SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
			cf.dwMask = CFM_ALL2 & ~CFM_LINK;
			SendMessage(rec.hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

			ITextRange *range = NULL;
			if (rec.textDocument->Range(pos+1, pos+2, &range) == S_OK) 
			{
				ITextPara *para = NULL;
				if (range->GetPara(&para) == S_OK)
				{
					para->SetAlignment(tomAlignCenter);
					para->Release();
				}
				range->Release();
			}
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


void ReplaceAllEmoticons(RichEditCtrl &rec, Contact *contact, Module *module, TCHAR *text, int len, int start, CHARRANGE &__old_sel, BOOL inInputArea, BOOL allowVideo)
{
	int diff = 0;
	for(int i = 0; i < len; i++)
	{
		if (isURL(&text[i], len - i))
		{
			int urlLen = findURLEnd(&text[i], len - i);

			if (allowVideo)
			{
				FlashData flash;
				if (isVideo(&text[i], urlLen, &flash))
				{
					i += urlLen;

					int pos = start + i + diff;

					int width = 250;
					int height = 180;

					if (flash.width > width || flash.height > height)
					{
						float f = min(width / (float) flash.width, height / (float) flash.height);
						flash.width = (int) (flash.width * f + 0.5f);
						flash.height = (int) (flash.height * f + 0.5f);
					}

					int this_dif = AddVideo(rec, pos, &flash);

					diff += this_dif;
					i += this_dif - 1;
					continue;
				}
			}

			i += urlLen - 1;
			continue;
		}

		EmoticonFound found;
		if (!FindEmoticonForwards(found, contact, module, text, len, i))
			continue;

		if (found.img == NULL && !found.custom)
			continue;

		int pos = start + i + diff;

		int this_dif = ReplaceEmoticon(rec, pos, found);
		if (this_dif != 0)
		{
			FixSelection(__old_sel.cpMax, pos + found.len, this_dif);
			FixSelection(__old_sel.cpMin, pos + found.len, this_dif);

			diff += this_dif;

			i += found.len - 1;
		}
	}
}


void ReplaceAllEmoticons(RichEditCtrl &rec, Contact *contact, Module *module, int start, int end, BOOL inInputArea, BOOL allowVideo)
{
	STOP_RICHEDIT(rec);

	TCHAR *text = GetText(rec, start, end);
	int len = lstrlen(text);

	ReplaceAllEmoticons(rec, contact, module, text, len, start, __old_sel, inInputArea, allowVideo);

	MIR_FREE(text);

	START_RICHEDIT(rec);
}


int RestoreRichEdit(RichEditCtrl &rec, CHARRANGE &old_sel, int start = 0, int end = -1)
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
		int refCount1 = reObj.poleobj->Release();
		if (FAILED(hr) || ttd == NULL)
			continue;

		BOOL replaced = FALSE;

		BSTR hint = NULL;
		hr = ttd->GetTooltip(&hint);
		if (SUCCEEDED(hr) && hint != NULL)
		{
			CHARRANGE sel = { reObj.cp, reObj.cp + 1 };
			if (hint[0] == 0)
			{
				sel.cpMin--;
				sel.cpMax++;
			}

			ITextRange *range;
			if (rec.textDocument->Range(sel.cpMin, sel.cpMax, &range) == S_OK) 
			{
				HRESULT hr = range->SetText(hint);
				if (hr == S_OK)
					replaced = TRUE;

				range->Release();
			}

			if (!replaced)
			{
				int oldCount = rec.ole->GetObjectCount();

				// Try by EM_REPLACESEL
				SendMessage(rec.hwnd, EM_EXSETSEL, 0, (LPARAM) &sel);
				SendMessageW(rec.hwnd, EM_REPLACESEL, FALSE, (LPARAM) hint);

				replaced = (oldCount != rec.ole->GetObjectCount());
			}

			if (replaced)
			{
				int dif = wcslen(hint) - (sel.cpMax - sel.cpMin);
				ret += dif;

				FixSelection(old_sel.cpMax, sel.cpMax, dif);
				FixSelection(old_sel.cpMin, sel.cpMax, dif);
			}

			SysFreeString(hint);
		}

		int refCount2 = ttd->Release();

		// This is a bug (I don't know where) that makes the OLE control not receive 
		// the Release call in the SetText above
		if (replaced && refCount1 == refCount2 + 1)
			reObj.poleobj->Release();
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
			if (wParam == VK_DELETE)
			{
				STOP_RICHEDIT(dlg->input);

				if (__old_sel.cpMin == __old_sel.cpMax && __old_sel.cpMax + 1 <= GetWindowTextLength(dlg->input.hwnd))
					RestoreRichEdit(dlg->input, __old_sel, __old_sel.cpMin, __old_sel.cpMax + 1);

				START_RICHEDIT(dlg->input);

				break;
			}
			
			if (wParam == VK_BACK)
			{
				STOP_RICHEDIT(dlg->input);

				if (__old_sel.cpMin == __old_sel.cpMax && __old_sel.cpMin > 0)
					RestoreRichEdit(dlg->input, __old_sel, __old_sel.cpMin - 1, __old_sel.cpMax);

				START_RICHEDIT(dlg->input);

				break;
			}

			if ((!(GetKeyState(VK_CONTROL) & 0x8000) || (wParam != 'C' && wParam != 'X' && wParam != VK_INSERT))
				&& (!(GetKeyState(VK_SHIFT) & 0x8000) || wParam != VK_DELETE))
				break;
		case WM_CUT:
		case WM_COPY:
		{
			STOP_RICHEDIT(dlg->input);
			RestoreRichEdit(dlg->input, __old_sel, __old_sel.cpMin, __old_sel.cpMax);
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
			if (msg == WM_CHAR && wParam >= 0 && wParam < 32)
				break;

			if (lParam & (1 << 28))	// ALT key
				break;

			if (wParam != _T('\n') && GetKeyState(VK_CONTROL) & 0x8000)	// CTRL key
				break;

			if ((lParam & 0xFF) > 2)	// Repeat rate
				break;

			STOP_RICHEDIT(dlg->input);

			int lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);

			// Check only the current line, one up and one down
			int line = SendMessage(hwnd, EM_LINEFROMCHAR, (WPARAM) __old_sel.cpMin, 0);

			int start = SendMessage(hwnd, EM_LINEINDEX, (WPARAM) line, 0);
			int end = start + SendMessage(hwnd, EM_LINELENGTH, (WPARAM) start, 0);
//			if (line < lines - 1)
//				end += SendMessage(hwnd, EM_LINELENGTH, (WPARAM) end, 0);

			end += RestoreRichEdit(dlg->input, __old_sel, start, end);

			TCHAR *text = GetText(dlg->input, start, end);
			int len = lstrlen(text);

			ReplaceAllEmoticons(dlg->input, NULL, dlg->module, text, len, start, __old_sel, TRUE, FALSE);

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
		ReplaceAllEmoticons(dlg->input, NULL, dlg->module, 0, -1, TRUE, FALSE);

	return ret;
}


LRESULT CALLBACK LogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	int rebuild = 0;
	switch(msg)
	{
		case EM_STREAMOUT:
			if (wParam & SFF_SELECTION)
				rebuild = 1;
			else
				rebuild = 2;
			break;

		case WM_KEYDOWN:
			if ((!(GetKeyState(VK_CONTROL) & 0x8000) || (wParam != 'C' && wParam != 'X' && wParam != VK_INSERT))
				&& (!(GetKeyState(VK_SHIFT) & 0x8000) || wParam != VK_DELETE))
				break;
		case WM_CUT:
		case WM_COPY:
		{
			rebuild = 1;
			break;
		}
	}

	CHARRANGE sel = {0, -1};
	if (rebuild)
	{
		STOP_RICHEDIT(dlg->log);

		if (rebuild == 1)
		{
			RestoreRichEdit(dlg->log, __old_sel, __old_sel.cpMin, __old_sel.cpMax);
			sel = __old_sel;
		}
		else
		{
			RestoreRichEdit(dlg->log, __old_sel);
		}

		START_RICHEDIT(dlg->log);
	}

	LRESULT ret = CallWindowProc(dlg->log.old_edit_proc, hwnd, msg, wParam, lParam);

	if (rebuild)
		ReplaceAllEmoticons(dlg->log, dlg->contact, dlg->module, sel.cpMin, sel.cpMax, FALSE, FALSE);

	return ret;
}


LRESULT CALLBACK SRMMLogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	int stream_in_pos;
	if (msg == EM_STREAMIN)
		stream_in_pos = GetWindowTextLength(dlg->log.hwnd);

	LRESULT ret = LogProc(hwnd, msg, wParam, lParam);

	if (msg == EM_STREAMIN)
		ReplaceAllEmoticons(dlg->log, dlg->contact, dlg->module, stream_in_pos, -1, FALSE, opts.embed_videos);

	return ret;
}


LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DialogMapType::iterator dlgit = dialogData.find(hwnd);
	if (dlgit == dialogData.end())
		return -1;

	Dialog *dlg = dlgit->second;

	if (msg == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1624))
	{
		dlg->log.sending = TRUE;

		STOP_RICHEDIT(dlg->input);

		RestoreRichEdit(dlg->input, __old_sel);

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
					ReplaceAllEmoticons(dlg->input, NULL, dlg->module, 0, -1, TRUE, FALSE);

				dlg->log.sending = FALSE;
			}
			break;
		}
	}

	return ret;
}


void UnloadRichEdit(RichEditCtrl *rec) 
{
	if (rec->textDocument != NULL)
	{
		rec->textDocument->Release();
		rec->textDocument = NULL;
	}
	if (rec->ole != NULL)
	{
		rec->ole->Release();
		rec->ole = NULL;
	}
}


int LoadRichEdit(RichEditCtrl *rec, HWND hwnd) 
{
	rec->hwnd = hwnd;
	rec->ole = NULL;
	rec->textDocument = NULL;
	rec->old_edit_proc = NULL;
	rec->sending = FALSE;

	SendMessage(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&rec->ole);
	if (rec->ole == NULL)
		return 0;

	if (rec->ole->QueryInterface(IID_ITextDocument, (void**)&rec->textDocument) != S_OK)
	{
		rec->textDocument = NULL;
		UnloadRichEdit(rec);
		return 0;
	}

	return 1;
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


BOOL isSRMM()
{
	if (!ServiceExists(MS_MSG_GETWINDOWCLASS))
		return FALSE;

	char cls[256];
	CallService(MS_MSG_GETWINDOWCLASS, (WPARAM) cls, (LPARAM) MAX_REGS(cls));
	return strcmp(cls, "SRMM") == 0;
}


int MsgWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData *evt = (MessageWindowEventData *)lParam;
	if (evt == NULL)
		return 0;

	if (evt->cbSize < sizeof(MessageWindowEventData))
		return 0;

	if (evt->uType == MSG_WINDOW_EVT_OPEN)
	{
		Module *m = GetContactModule(evt->hContact);
		if (m == NULL)
			return 0;

		Dialog *dlg = (Dialog *) malloc(sizeof(Dialog));
		ZeroMemory(dlg, sizeof(Dialog));

		dlg->hOriginalContact = evt->hContact;
		dlg->contact = GetContact(evt->hContact);
		dlg->module = m;
		dlg->hwnd_owner = evt->hwndWindow;

		int loaded = LoadRichEdit(&dlg->input, evt->hwndInput);
		loaded |= LoadRichEdit(&dlg->log, evt->hwndLog);

		if (!loaded)
		{
			free(dlg);
			return 0;
		}

		if (opts.replace_in_input && dlg->input.isLoaded())
		{
			dlg->input.old_edit_proc = (WNDPROC) SetWindowLong(dlg->input.hwnd, GWL_WNDPROC, (LONG) EditProc);
			dialogData[dlg->input.hwnd] = dlg;

			dlg->owner_old_edit_proc = (WNDPROC) SetWindowLong(dlg->hwnd_owner, GWL_WNDPROC, (LONG) OwnerProc);
		}

		dialogData[dlg->hwnd_owner] = dlg;

		dialogData[dlg->log.hwnd] = dlg;

		if (isSRMM())
		{
			ReplaceAllEmoticons(dlg->log, dlg->contact, dlg->module, 0, -1, FALSE, opts.embed_videos);

			dlg->log.old_edit_proc = (WNDPROC) SetWindowLong(dlg->log.hwnd, GWL_WNDPROC, (LONG) SRMMLogProc);
		}
		else
		{
			dlg->log.old_edit_proc = (WNDPROC) SetWindowLong(dlg->log.hwnd, GWL_WNDPROC, (LONG) LogProc);
		}
	}
	else if (evt->uType == MSG_WINDOW_EVT_CLOSING)
	{
		DialogMapType::iterator dlgit = dialogData.find(evt->hwndWindow);
		if (dlgit != dialogData.end())
		{
			Dialog *dlg = dlgit->second;

			CHARRANGE sel = {0, 0};
			RestoreRichEdit(dlg->input, sel);
			RestoreRichEdit(dlg->log, sel);

			UnloadRichEdit(&dlg->input);
			UnloadRichEdit(&dlg->log);

			if (dlg->input.old_edit_proc != NULL)
				SetWindowLong(dlg->input.hwnd, GWL_WNDPROC, (LONG) dlg->input.old_edit_proc);
			if (dlg->log.old_edit_proc != NULL)
				SetWindowLong(dlg->log.hwnd, GWL_WNDPROC, (LONG) dlg->log.old_edit_proc);
			if (dlg->owner_old_edit_proc != NULL)
				SetWindowLong(dlg->hwnd_owner, GWL_WNDPROC, (LONG) dlg->owner_old_edit_proc);

			free(dlg);
		}

		dialogData.erase(evt->hwndInput);
		dialogData.erase(evt->hwndLog);
		dialogData.erase(evt->hwndWindow);
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


void HandleEmoLine(Module *m, TCHAR *line, char *group)
{
	int len = lstrlen(line);
	int state = 0;
	int pos;

	Emoticon *e = NULL;

	for(int i = 0; i < len; i++)
	{
		TCHAR c = line[i];
		if (c == _T(' '))
			continue;

		if ((state % 2) == 0)
		{
			if (c == _T('#'))
				break;

			if (c != _T('"'))
				continue;

			state ++;
			pos = i+1;
		}
		else
		{
			if (c == _T('\\'))
			{
				i++;
				continue;
			}
			if (c != _T('"'))
				continue;

			line[i] = 0;
			TCHAR *txt = &line[pos];

			for(int j = 0, orig = 0; j <= i - pos; j++)
			{
				if (txt[j] == _T('\\'))
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
					break;
				case 3: 
					e->description = mir_tstrdup(txt); 
					break;
				case 5: 
					if (strncmp(e->name, "service_", 8) == 0)
					{
						char *atxt = mir_t2a(txt);
						int len = strlen(atxt);

						// Is a service
						if (strncmp(atxt, "<Service:", 9) != 0 || atxt[len-1] != '>')
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
							{
								params = NULL;
								break;
							}

							params = pos + 1;
						}

						if (params != NULL && strncmp(params, "Wrapper:", 8) == 0)
						{
							params += 8;
							for(int i = 3; i < 6; i++)
							{
								char *pos = strchr(params, ':');
								if (pos != NULL)
									*pos = '\0';

								e->service[i] = mir_strdup(params);

								if (pos == NULL)
									break;

								params = pos + 1;
							}
						}

						MIR_FREE(atxt)
					}
					else
						e->texts.insert(mir_tstrdup(txt)); 
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
#ifdef _UNICODE
				TCHAR *text = mir_utf8decodeW(tmp);
#else
				TCHAR *text = mir_utf8decode(tmp, NULL);
#endif

				HandleEmoLine(m, text, group);

#ifdef _UNICODE
				mir_free(text);
#endif
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
	if (name == NULL)
		return NULL;

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


const char * GetProtoID(const char *proto)
{
	if (!ServiceExists(MS_PROTO_GETACCOUNT))
		return proto;

	PROTOACCOUNT *acc = ProtoGetAccount(proto);
	if (acc == NULL)
		return proto;

	return acc->szProtoName;
}


/*
Code block copied from Jabber sources (with small modifications)

Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

Idea & portions of code by Artem Shpynov
*/
struct
{
	TCHAR *mask;
	const char *proto;
}
static TransportProtoTable[] =
{
	{ _T("|icq*|jit*"),      "ICQ"},
	{ _T("msn*"),            "MSN"},
	{ _T("yahoo*"),          "YAHOO"},
	{ _T("mrim*"),           "MRA"},
	{ _T("aim*"),            "AIM"},
	//request #3094
	{ _T("|gg*|gadu*"),      "GaduGadu"},
	{ _T("tv*"),             "TV"},
	{ _T("dict*"),           "Dictionary"},
	{ _T("weather*"),        "Weather"},
	{ _T("sms*"),            "SMS"},
	{ _T("smtp*"),           "SMTP"},
	//j2j
	{ _T("gtalk.*.*"),       "JGMAIL"},
	{ _T("xmpp.*.*"),        "Jabber"},
	//jabbim.cz - services
	{ _T("disk*"),           "Jabber Disk"},
	{ _T("irc*"),            "IRC"},
	{ _T("rss*"),            "RSS"},
	{ _T("tlen*"),           "Tlen"}
};


static inline TCHAR qtoupper( TCHAR c )
{
	return ( c >= 'a' && c <= 'z' ) ? c - 'a' + 'A' : c;
}

static BOOL WildComparei( const TCHAR* name, const TCHAR* mask )
{
	const TCHAR* last='\0';
	for ( ;; mask++, name++) {
		if ( *mask != '?' && qtoupper( *mask ) != qtoupper( *name ))
			break;
		if ( *name == '\0' )
			return ((BOOL)!*mask);
	}

	if ( *mask != '*' )
		return FALSE;

	for (;; mask++, name++ ) {
		while( *mask == '*' ) {
			last = mask++;
			if ( *mask == '\0' )
				return ((BOOL)!*mask);   /* true */
		}

		if ( *name == '\0' )
			return ((BOOL)!*mask);      /* *mask == EOS */
		if ( *mask != '?' && qtoupper( *mask ) != qtoupper( *name ))
			name -= (size_t)(mask - last) - 1, mask = last;
}	}

#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

static BOOL MatchMask( const TCHAR* name, const TCHAR* mask)
{
	if ( !mask || !name )
		return mask == name;

	if ( *mask != '|' )
		return WildComparei( name, mask );

	TCHAR* temp = NEWTSTR_ALLOCA(mask);
	for ( int e=1; mask[e] != '\0'; e++ ) {
		int s = e;
		while ( mask[e] != '\0' && mask[e] != '|')
			e++;

		temp[e]= _T('\0');
		if ( WildComparei( name, temp+s ))
			return TRUE;

		if ( mask[e] == 0 )
			return FALSE;
	}

	return FALSE;
}
/*
End of code block copied from Jabber sources
*/

const char *GetTransport(HANDLE hContact, const char *proto)
{
	if (!DBGetContactSettingByte(hContact, proto, "IsTransported", 0))
		return NULL;

	DBVARIANT dbv = {0};
	if (DBGetContactSettingTString(hContact, proto, "Transport", &dbv) != 0)
		return NULL;

	const TCHAR *domain = dbv.ptszVal;
	const char *transport = NULL;

	for (int i = 0 ; i < MAX_REGS(TransportProtoTable); i++)
	{
		if (MatchMask(domain, TransportProtoTable[i].mask))
		{
			transport = TransportProtoTable[i].proto;
			break;
		}
	}

	DBFreeVariant(&dbv);
	return transport;
}


Module * GetContactModule(HANDLE hContact, const char *proto)
{
	if (hContact == NULL)
	{
		if (proto == NULL)
			return NULL;
		return GetModule(GetProtoID(proto));
	}

	hContact = GetRealContact(hContact);

	proto = (const char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (proto == NULL)
		return NULL;

	const char *protoID = GetProtoID(proto);

	// Check for transports
	if (stricmp("JABBER", protoID) == 0)
	{
		const char *transport = GetTransport(hContact, proto);
		if (transport != NULL)
		{
			return GetModule(transport);
		}
	}

	// Check if there is some derivation
	for(int i = 0; i < modules.getCount(); i++) 
	{
		Module *m = modules[i];
		
		if (m->derived.proto_name == NULL)
			continue;

		if (stricmp(m->derived.proto_name, protoID) != 0)
			continue;
		
		DBVARIANT dbv = {0};
		if (DBGetContactSettingString(NULL, proto, m->derived.db_key, &dbv) != 0)
			continue;

		Module *ret = NULL;
		if (stricmp(dbv.pszVal, m->derived.db_val) == 0)
			ret = m;

		DBFreeVariant(&dbv);

		if (ret != NULL)
			return ret;
	}

	return GetModule(protoID);
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
			sre->rangeToReplace == NULL ? -1 : sre->rangeToReplace->cpMax, FALSE, opts.embed_videos);
	}
	else
	{
		Module *m = GetContactModule(sre->hContact, sre->Protocolname);
		if (m == NULL)
			return FALSE;

		RichEditCtrl rec = {0};
		LoadRichEdit(&rec, sre->hwndRichEditControl);
		ReplaceAllEmoticons(rec, GetContact(sre->hContact), m, sre->rangeToReplace == NULL ? 0 : sre->rangeToReplace->cpMin, 
			sre->rangeToReplace == NULL ? -1 : sre->rangeToReplace->cpMax, FALSE, opts.embed_videos);
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

BOOL Emoticon::IgnoreFor(Module *m)
{
	return service[0] != NULL && !EmoticonServiceExists(m->name, service[0]);
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


void EmoticonImage::Load()
{
	int max_height, max_width;
	Load(max_height, max_width);
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
		ce->firstReceived = (DWORD) time(NULL);

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
	DBWriteContactSettingDword(contact->hContact, "CustomSmileys", setting, ce->firstReceived);

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

DWORD ConvertServiceParam(char *param, char *proto, HANDLE hContact)
{
	DWORD ret;
	if (param == NULL)
		ret = 0;
	else if (stricmp("hContact", param) == 0)
		ret = (DWORD) hContact;
	else if (stricmp("protocol", param) == 0)
		ret = (DWORD) proto;
	else
		ret = atoi(param);
	return ret;
}

int CallEmoticonService(char *proto, HANDLE hContact, char *service, char *wparam, char *lparam)
{
	if (service[0] == '/')
		return CallProtoService(proto, service, ConvertServiceParam(wparam, proto, hContact), ConvertServiceParam(lparam, proto, hContact));
	else
		return CallService(service, ConvertServiceParam(wparam, proto, hContact), ConvertServiceParam(lparam, proto, hContact));
}


BOOL EmoticonServiceExists(char *proto, char *service)
{
	if (service[0] == '/')
		return ProtoServiceExists(proto, service);
	else
		return ServiceExists(service);
}


int DefaultMetaChanged(WPARAM wParam, LPARAM lParam)
{
	HANDLE hMetaContact = (HANDLE) wParam;
	HANDLE hDefaultContact = (HANDLE) lParam;

	Module *m = GetContactModule(hDefaultContact);
	if (m == NULL)
		return 0;

	for(DialogMapType::iterator it = dialogData.begin(); it != dialogData.begin(); it++)
	{
		Dialog *dlg = it->second;
		if (dlg->hOriginalContact != hMetaContact)
			continue;
		
		dlg->contact = GetContact(hDefaultContact);
		dlg->module = m;
	}

	return 0;
}


HBITMAP CopyBitmapTo32(HBITMAP hBitmap)
{
	BITMAPINFO RGB32BitsBITMAPINFO; 
	BYTE * ptPixels;
	HBITMAP hDirectBitmap;

	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	dwLen = bmp.bmWidth * bmp.bmHeight * 4;
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return NULL;

	// Create bitmap
	ZeroMemory(&RGB32BitsBITMAPINFO, sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth = bmp.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight = bmp.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

	hDirectBitmap = CreateDIBSection(NULL, 
									(BITMAPINFO *)&RGB32BitsBITMAPINFO, 
									DIB_RGB_COLORS,
									(void **)&ptPixels, 
									NULL, 0);

	// Copy data
	if (bmp.bmBitsPixel != 32)
	{
		HDC hdcOrig, hdcDest;
		HBITMAP oldOrig, oldDest;
		
		hdcOrig = CreateCompatibleDC(NULL);
		oldOrig = (HBITMAP) SelectObject(hdcOrig, hBitmap);

		hdcDest = CreateCompatibleDC(NULL);
		oldDest = (HBITMAP) SelectObject(hdcDest, hDirectBitmap);

		BitBlt(hdcDest, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcOrig, 0, 0, SRCCOPY);

		SelectObject(hdcDest, oldDest);
		DeleteObject(hdcDest);
		SelectObject(hdcOrig, oldOrig);
		DeleteObject(hdcOrig);

		// Set alpha
		fei->FI_CorrectBitmap32Alpha(hDirectBitmap, FALSE);
	}
	else
	{
		GetBitmapBits(hBitmap, dwLen, p);
		SetBitmapBits(hDirectBitmap, dwLen, p);
	}

	free(p);

	return hDirectBitmap;
}


HICON CopyToIcon(HBITMAP hOrigBitmap)
{
	HICON hIcon = NULL;
	HBITMAP hColor = NULL;
	HBITMAP hMask = NULL;
	DWORD dwLen;
	BYTE *p = NULL;
	BYTE *maskBits = NULL;
	BYTE *colorBits = NULL;

	BITMAP bmp;
	GetObject(hOrigBitmap, sizeof(bmp), &bmp);

	HBITMAP hBitmap;
	BOOL transp;
	if (bmp.bmBitsPixel == 32)
	{
		hBitmap = hOrigBitmap;
		transp = TRUE;
	}
	else
	{
		hBitmap = CopyBitmapTo32(hOrigBitmap);
		GetObject(hBitmap, sizeof(bmp), &bmp);
		transp = FALSE;
	}


	hColor = CreateBitmap32(bmp.bmWidth, bmp.bmHeight);
	if (hColor == NULL)
		goto ERR;

	hMask = CreateBitmap32(bmp.bmWidth, bmp.bmHeight);
	if (hMask == NULL)
		goto ERR;

	dwLen = bmp.bmWidth * bmp.bmHeight * 4;

	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		goto ERR;
	maskBits = (BYTE *)malloc(dwLen);
	if (maskBits == NULL)
		goto ERR;
	colorBits = (BYTE *)malloc(dwLen);
	if (colorBits == NULL)
		goto ERR;
	
	{
		GetBitmapBits(hBitmap, dwLen, p);
		GetBitmapBits(hColor, dwLen, maskBits);
		GetBitmapBits(hMask, dwLen, colorBits);

		for (int y = 0; y < bmp.bmHeight; ++y) 
		{
			int shift = bmp.bmWidth * 4 * y;
			BYTE *px = p + shift;
			BYTE *mask = maskBits + shift;
			BYTE *color = colorBits + shift;

			for (int x = 0; x < bmp.bmWidth; ++x) 
			{
				if (transp)
				{
					for(int i = 0; i < 4; i++)
					{
						mask[i] = px[3];
						color[i] = px[i];
					}
				}
				else
				{
					for(int i = 0; i < 3; i++)
					{
						mask[i] = 255;
						color[i] = px[i];
					}
					mask[3] = color[3] = 255;
				}

				px += 4;
				mask += 4;
				color += 4;
			}
		}

		SetBitmapBits(hColor, dwLen, colorBits);
		SetBitmapBits(hMask, dwLen, maskBits);


		ICONINFO ii = {0};
		ii.fIcon = TRUE;
		ii.hbmColor = hColor;
		ii.hbmMask = hMask;

		hIcon = CreateIconIndirect(&ii);
	}

ERR:
	if (hMask != NULL)
		DeleteObject(hMask);
	if (hColor != NULL)
		DeleteObject(hColor);

	if (p != NULL)
		free(p);
	if (maskBits != NULL)
		free(maskBits);
	if (colorBits != NULL)
		free(colorBits);

	if (hBitmap != hOrigBitmap)
		DeleteObject(hBitmap);

	return hIcon;
}


int ParseService(SMADD_PARSE *sp, BOOL unicode)
{
	int start = sp->startChar + sp->size;

	sp->size = 0;

	TCHAR *text;
	if (unicode)
		text = mir_u2t((WCHAR *) sp->str);
	else
		text = mir_a2t(sp->str);
	int len = lstrlen(text);

	Module *module = GetModule(sp->Protocolname);

	if (start >= len || start < 0 || module == NULL)
	{
		mir_free(text);
		return -1;
	}

	EmoticonFound found;
	for(int i = start; i < len; i++)
	{
		if (isURL(&text[i], len - i))
		{
			i += findURLEnd(&text[i], len - i) - 1;
			continue;
		}

		if (!FindEmoticonForwards(found, NULL, module, text, len, i))
			continue;

		if (found.custom || found.img == NULL)
			continue;

		sp->SmileyIcon = CopyToIcon(found.img);
		if (sp->SmileyIcon == NULL)
			continue;

		sp->startChar = i;
		sp->size = found.len;

		break;
	}

	mir_free(text);

	return 0;
}


int ParseService(WPARAM wParam, LPARAM lParam)
{
	SMADD_PARSE *sp = (SMADD_PARSE *) lParam;
	if (sp == NULL || sp->cbSize < sizeof(SMADD_PARSE) || sp->str == NULL)
		return -1;

	return ParseService(sp, FALSE);
}


int ParseWService(WPARAM wParam, LPARAM lParam)
{
	SMADD_PARSEW *sp = (SMADD_PARSEW *) lParam;
	if (sp == NULL || sp->cbSize < sizeof(SMADD_PARSEW) || sp->str == NULL)
		return -1;

	return ParseService((SMADD_PARSE *) sp, TRUE);
}



int BatchParseService(WPARAM wParam, LPARAM lParam)
{
	SMADD_BATCHPARSE2 *bp = (SMADD_BATCHPARSE2 *) lParam;
	if (bp == NULL || bp->cbSize < sizeof(SMADD_BATCHPARSE2) || bp->str == NULL)
		return NULL;

	Contact *contact = GetContact(bp->hContact);
	Module *module = GetContactModule(bp->hContact, bp->Protocolname);

	if (module == NULL)
		return NULL;

	Buffer<SMADD_BATCHPARSERES> ret;

	BOOL path = (bp->flag & SAFL_PATH);
	BOOL unicode = (bp->flag & SAFL_UNICODE);
	BOOL outgoing = (bp->flag & SAFL_OUTGOING);
	BOOL custom = !(bp->flag & SAFL_NOCUSTOM) && !outgoing;

	TCHAR *text;
	if (unicode)
		text = mir_u2t(bp->wstr);
	else
		text = mir_a2t(bp->astr);
	int len = lstrlen(text);

	EmoticonFound found;
	int count = 0;
	for(int i = 0; i < len; i++)
	{
		if (isURL(&text[i], len - i))
		{
			i += findURLEnd(&text[i], len - i) - 1;
			continue;
		}

		if (!FindEmoticonForwards(found, contact, module, text, len, i))
			continue;

		SMADD_BATCHPARSERES res = {0};
		res.startChar = i;
		res.size = found.len;

		if (found.custom && !custom)
			continue;
		if (found.img == NULL && !found.custom)
			continue;

		if (path)
		{
			if (unicode)
				res.wfilepath = mir_a2u(found.path);
			else
				res.afilepath = mir_strdup(found.path);
		}
		else
		{
			res.hIcon = CopyToIcon(found.img);

			if (res.hIcon == NULL)
				continue;
		}

		ret.append(res);
	}

	bp->numSmileys = ret.len;
	bp->oflag = bp->flag;

	mir_free(text);

	return (int) ret.detach();
}


int BatchFreeService(WPARAM wParam, LPARAM lParam)
{
	mir_free((void *) lParam);

	return 0;
}


