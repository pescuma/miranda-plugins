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


#ifndef __M_SKINS_H__
# define __M_SKINS_H__

#include <windows.h>

#define MIID_SKINS { 0x917db7a4, 0xd0fe, 0x4b1c, { 0x8c, 0xa3, 0x6d, 0xc1, 0x44, 0x80, 0xf5, 0xcc } }


typedef void * SKINNED_DIALOG;
typedef void * SKINNED_FIELD;


/// Some common parameters:
///  - name : internal name and name used inside skin file
///  - description : name shown to the user
///  - module : the module name where the settings will be stored
/// Do not translate any parameters.
struct SKIN_INTERFACE
{
	int cbSize;

	// Global methods
	SKINNED_DIALOG (*RegisterDialog)(const char *name, const char *description, const char *module);
	void (*DeleteDialog)(SKINNED_DIALOG dlg);
	void (*FinishedConfiguring)(SKINNED_DIALOG dlg);

	// Dialog methods
	SKINNED_FIELD (*AddTextField)(SKINNED_DIALOG dlg, const char *name, const char *description);
	SKINNED_FIELD (*AddIconField)(SKINNED_DIALOG dlg, const char *name, const char *description);
	SKINNED_FIELD (*AddImageField)(SKINNED_DIALOG dlg, const char *name, const char *description);
	SKINNED_FIELD (*GetField)(SKINNED_DIALOG dlg, const char *name);
	void (*SetDialogSize)(SKINNED_DIALOG dlg, int width, int height);
	RECT (*GetBorders)(SKINNED_DIALOG dlg);

	// Field methods
	RECT (*GetRect)(SKINNED_FIELD field);
	BOOL (*IsVisible)(SKINNED_FIELD field);

	// TextField methods
	void (*SetTextA)(SKINNED_FIELD field, const char *text);
	void (*SetTextW)(SKINNED_FIELD field, const WCHAR *text);
	char * (*GetTextA)(SKINNED_FIELD field); // You have to free the result
	WCHAR * (*GetTextW)(SKINNED_FIELD field); // You have to free the result
	HFONT (*GetFont)(SKINNED_FIELD field);
	COLORREF (*GetFontColor)(SKINNED_FIELD field);

	// IconField methods
	void (*SetIcon)(SKINNED_FIELD field, HICON hIcon);
	HICON (*GetIcon)(SKINNED_FIELD field);

	// ImageField methods
	void (*SetImage)(SKINNED_FIELD field, HBITMAP hBmp);
	HBITMAP (*GetImage)(SKINNED_FIELD field);
};



/*
Skins/GetInterface service
Fill the function pointers for a SKIN_INTERFACE struct

wparam = 0
lparam = (SKIN_INTERFACE *) struct to be filled
returns: 0 on success
*/
#define MS_SKINS_GETINTERFACE		"Skins/GetInterface"




static int mir_skins_getInterface(struct SKIN_INTERFACE *dest)
{
	dest->cbSize = sizeof(SKIN_INTERFACE);
	return CallService(MS_SKINS_GETINTERFACE, 0, (LPARAM) dest);
}


#endif // __M_SKINS_H__
