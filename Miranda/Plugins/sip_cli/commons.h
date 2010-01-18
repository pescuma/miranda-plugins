/* 
Copyright (C) 2010 Ricardo Pescuma Domenecci

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


// Miranda headers
#define MIRANDA_VER 0x0800
#include <win2k.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_langpack.h>
#include <m_database.h>
#include <m_options.h>
#include <m_utils.h>

#include "../utils/mir_memory.h"
#include "../utils/mir_icons.h"

#include "sdk/m_sip.h"
#include "sdk/m_voice.h"
#include "sdk/m_voiceservice.h"

#include "resource.h"


#define MODULE_NAME		"SIP_CLI"


// Global Variables
extern HINSTANCE hInst;
extern PLUGINLINK *pluginLink;


#define MAX_REGS(_A_) ( sizeof(_A_) / sizeof(_A_[0]) )



#endif // __COMMONS_H__
