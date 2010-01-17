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


#ifndef __COMMONS_H__
# define __COMMONS_H__


#define OEMRESOURCE 
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>

#include <vector>
#include <list>

// Miranda headers
#define MIRANDA_VER 0x0800
#include <win2k.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_system_cpp.h>
#include <m_protocols.h>
#include <m_protosvc.h>
#include <m_protomod.h>
#include <m_protoint.h>
#include <m_clist.h>
#include <m_contacts.h>
#include <m_langpack.h>
#include <m_database.h>
#include <m_options.h>
#include <m_utils.h>
#include <m_updater.h>
#include <m_popup.h>
#include <m_history.h>
#include <m_message.h>
#include <m_folders.h>
#include <m_icolib.h>
#include <m_avatars.h>
#include <m_netlib.h>
#include <m_fontservice.h>

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

#include "../../plugins/utils/mir_memory.h"
#include "../../plugins/utils/mir_options.h"
#include "../../plugins/utils/mir_icons.h"
#include "../../plugins/utils/mir_log.h"
#include "../../plugins/utils/mir_dbutils.h"
#include "../../plugins/utils/utf8_helpers.h"
#include "../../plugins/utils/scope.h"
#include "../../plugins/voiceservice/m_voice.h"
#include "../../plugins/voiceservice/m_voiceservice.h"

#include "resource.h"
#include "strutils.h"


struct MessageData
{
	HANDLE hContact;
	LONG messageID;
	pjsip_status_code status;
};

struct SIPEvent
{
	enum {
		reg_state,
		incoming_call,
		call_state,
		call_media_state,
		incoming_subscribe,
		buddy_state,
		pager,
		pager_status,
		typing

	} type;

	pjsua_call_id call_id;
	pjsua_call_info call_info;
	pjsua_buddy_id buddy_id;
	char *from;
	char *text;
	char *mime;
	bool isTyping;
	MessageData *messageData;
	pjsua_srv_pres *srv_pres;
};


#include "m_sip.h"
#include "SIPProto.h"
#include "SIPClient.h"


#define MODULE_NAME		"SIP"


// Global Variables
extern HINSTANCE hInst;
extern PLUGINLINK *pluginLink;
extern OBJLIST<SIPProto> instances;

#define MAX_REGS(_A_) ( sizeof(_A_) / sizeof(_A_[0]) )
#define MIR_FREE(_X_) if (_X_ != NULL) { mir_free(_X_); _X_ = NULL; }



static TCHAR *lstrtrim(TCHAR *str)
{
	int len = lstrlen(str);

	int i;
	for(i = len - 1; i >= 0 && (str[i] == ' ' || str[i] == '\t'); --i) ;
	if (i < len - 1)
	{
		++i;
		str[i] = _T('\0');
		len = i;
	}

	for(i = 0; i < len && (str[i] == ' ' || str[i] == '\t'); ++i) ;
	if (i > 0)
		memmove(str, &str[i], (len - i + 1) * sizeof(TCHAR));

	return str;
}


static BOOL IsEmptyA(const char *str)
{
	return str == NULL || str[0] == 0;
}

static BOOL IsEmptyW(const WCHAR *str)
{
	return str == NULL || str[0] == 0;
}

#ifdef UNICODE
# define IsEmpty IsEmptyW
#else
# define IsEmpty IsEmptyA
#endif

static char * FirstNotEmptyA(char *str1, char *str2)
{
	if (!IsEmptyA(str1))
		return str1;
	return str2;
}

static char * FirstNotEmptyA(char *str1, char *str2, char *str3)
{
	if (!IsEmptyA(str1))
		return str1;
	if (!IsEmptyA(str2))
		return str2;
	return str3;
}

static char * FirstNotEmptyA(char *str1, char *str2, char *str3, char *str4)
{
	if (!IsEmptyA(str1))
		return str1;
	if (!IsEmptyA(str2))
		return str2;
	if (!IsEmptyA(str3))
		return str3;
	return str4;
}


static WCHAR * FirstNotEmptyW(WCHAR *str1, WCHAR *str2)
{
	if (!IsEmptyW(str1))
		return str1;
	return str2;
}

static WCHAR * FirstNotEmptyW(WCHAR *str1, WCHAR *str2, WCHAR *str3)
{
	if (!IsEmptyW(str1))
		return str1;
	if (!IsEmptyW(str2))
		return str2;
	return str3;
}

static WCHAR * FirstNotEmptyW(WCHAR *str1, WCHAR *str2, WCHAR *str3, WCHAR *str4)
{
	if (!IsEmptyW(str1))
		return str1;
	if (!IsEmptyW(str2))
		return str2;
	if (!IsEmptyW(str3))
		return str3;
	return str4;
}

#ifdef UNICODE
# define FirstNotEmpty FirstNotEmptyW
#else
# define FirstNotEmpty FirstNotEmptyA
#endif


#endif // __COMMONS_H__
