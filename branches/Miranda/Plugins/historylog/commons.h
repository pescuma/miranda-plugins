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


#ifndef __COMMONS_H__
# define __COMMONS_H__


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>


// Miranda headers
#define MIRANDA_VER 0x0700

#include <newpluginapi.h>
#include <m_system.h>
#include <m_system_cpp.h>
#include <m_contacts.h>
#include <m_langpack.h>
#include <m_database.h>
#include <m_options.h>
#include <m_utils.h>
#include <m_history.h>
#include <m_updater.h>
#include <m_icolib.h>
#include <m_clist.h>
#include <m_message.h>
#include <m_metacontacts.h>
#include <m_historyevents.h>
#include <m_protocols.h>
#include <m_protosvc.h>
#include <m_chat.h>
#include <m_speak.h>

#include "../utils/mir_memory.h"
#include "../utils/mir_options.h"
#include "../utils/mir_icons.h"
#include "../utils/mir_buffer.h"
#include "../utils/mir_scope.h"
#include "../utils/ContactAsyncQueue.h"

#include "resource.h"
#include "options.h"


#define GC_EVENT_HIGHLIGHT		0x1000


#define MS_GC_GETEVENTPTR  "GChat/GetNewEventPtr"
typedef int (*GETEVENTFUNC)(WPARAM wParam, LPARAM lParam);
typedef struct  {
	GETEVENTFUNC pfnAddEvent;
}GCPTRS;


#define MODULE_NAME		"HistoryLog"


// Global Variables
extern HINSTANCE hInst;
extern PLUGINLINK *pluginLink;

#define MAX_REGS(_A_) ( sizeof(_A_) / sizeof(_A_[0]) )


#define MIID_HISTORYLOG { 0xf64dfb3, 0x4e21, 0x4837, { 0xb6, 0x18, 0xf1, 0x54, 0x6c, 0xf2, 0x85, 0xa3 } }



BOOL IsEventEnabled(WORD eventType);
void SetEventEnabled(WORD eventType, BOOL enable);

BOOL IsChatEventEnabled(WORD type);
void SetChatEventEnabled(WORD type, BOOL enable);


static struct
{
	int type;
	TCHAR *name;
	TCHAR *templ;
	TCHAR *templ_notext;
	TCHAR *templ_nonick;
} 
CHAT_EVENTS[] = 
{
	{ GC_EVENT_MESSAGE,			_T("Message"),				_T("* %nick%: %text%") },
	{ GC_EVENT_MESSAGE,			_T("Highlighted Message"),	_T("* %nick%: %text%") },
	{ GC_EVENT_ACTION,			_T("Action"),				_T("* %nick% %text%") },
	{ GC_EVENT_JOIN,			_T("User joined"),			_T("> %nick% has joined") },
	{ GC_EVENT_PART,			_T("User left"),			_T("< %nick% has left (%text_sl%)"), _T("< %nick% has left") },
	{ GC_EVENT_QUIT,			_T("User disconnected"),	_T("< %nick% has disconnected (%text_sl%)"), _T("< %nick% has disconnected") },
	{ GC_EVENT_KICK,			_T("User kicked"),			_T("~ %status% kicked %nick% (%text_sl%)"), _T("~ %status% kicked %nick%") },
	{ GC_EVENT_NICK,			_T("Nickname change"),		_T("^ %nick% is now known as %text%") },
	{ GC_EVENT_NOTICE,			_T("Notice"),				_T("¤ Notice from %nick%: %text%") },
	{ GC_EVENT_TOPIC,			_T("Topic change"),			_T("# The topic is \'%text_sl%\' (set by %nick%)"), NULL, _T("# The topic is \'%text_sl%\'") },
	{ GC_EVENT_INFORMATION,		_T("Information message"),	_T("! %text_sl%") },
	{ GC_EVENT_ADDSTATUS,		_T("Status enabled"),		_T("+ %text% enables \'%status%\' status for %nick%") },
	{ GC_EVENT_REMOVESTATUS,	_T("Status disabled"),		_T("- %text% disables \'%status%\' status for %nick%") }
};



#endif // __COMMONS_H__
