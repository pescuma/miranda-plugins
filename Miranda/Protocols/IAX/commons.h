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
#include <time.h>

#include <vector>

// Miranda headers
#define MIRANDA_VER 0x0900
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

#include "../../plugins/utils/mir_memory.h"
#include "../../plugins/utils/mir_options.h"
#include "../../plugins/utils/mir_icons.h"
#include "../../plugins/utils/mir_log.h"

#include <iaxclient.h>

#include "resource.h"
#include "m_iax.h"
#include "options.h"


#define MODULE_NAME		"IAX"


// Global Variables
extern HINSTANCE hInst;
extern PLUGINLINK *pluginLink;

#define MAX_REGS(_A_) ( sizeof(_A_) / sizeof(_A_[0]) )
#define MIR_FREE(_X_) if (_X_ != NULL) { mir_free(_X_); _X_ = NULL; }
#define RELEASE(_X_) if (_X_ != NULL) { _X_->Release(); _X_ = NULL; }




#endif // __COMMONS_H__