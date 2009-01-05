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


#ifndef __M_SKINS_CPP_H__
# define __M_SKINS_CPP_H__

#include "m_skins.h"

extern struct SKIN_INTERFACE mski;


class SkinField
{
public:
	SkinField(SKINNED_FIELD field) { this->field = field; }
	
	BOOL isValid() { return field != NULL; }
	
	RECT getRect() { return mski.GetRect(field); }
	BOOL isVisible() { return mski.IsVisible(field); }

protected:
	SKINNED_FIELD field;
};

class SkinTextField : public SkinField
{
public:
	SkinTextField(SKINNED_FIELD field) : SkinField(field), text(NULL) {}
	~SkinTextField() { if (text != NULL) mir_free(text); }

	void setText(const TCHAR *text) { 
#ifdef UNICODE 
		mski.SetTextW(field, text); 
#else 
		mski.SetTextA(field, text); 
#endif
	}
	const TCHAR * getText() { 
		if (text != NULL) 
			mir_free(text);

#ifdef UNICODE 
		text = mski.GetTextW(field); 
#else 
		text = mski.GetTextA(field); 
#endif 
		return  text;
	}

	HFONT getFont() { return mski.GetFont(field); }
	COLORREF getFontColor() { return mski.GetFontColor(field); }
	int getHorizontalAlign() { return mski.GetHorizontalAlign(field); } // one of SKN_HALIGN_*

private:
	TCHAR *text;
};

class SkinIconField : public SkinField
{
public:
	SkinIconField(SKINNED_FIELD field) : SkinField(field) {}

	void setIcon(HICON hIcon) { mski.SetIcon(field, hIcon); }
	HICON getIcon() { return mski.GetIcon(field); }
};

class SkinImageField : public SkinField
{
public:
	SkinImageField(SKINNED_FIELD field) : SkinField(field) {}

	void setImage(HBITMAP hBmp) { mski.SetImage(field, hBmp); }
	HBITMAP getImage() { return mski.GetImage(field); }
};


class SkinDialog
{
public:
	SkinDialog(const char *name, const char *description, const char *module)
	{
		dlg = mski.RegisterDialog(name, description, module);
	}

	~SkinDialog()
	{
		mski.DeleteDialog(dlg);
		dlg = NULL;
	}

	BOOL isValid() { return dlg != NULL; }

	void finishedConfiguring() { mski.FinishedConfiguring(dlg); }
	void setSize(int width, int height) { mski.SetDialogSize(dlg, width, height); }
	RECT getBorders() { return mski.GetBorders(dlg); }

	SkinTextField addTextField(const char *name, const char *description) { return SkinTextField( mski.AddTextField(dlg, name, description) ); }
	SkinIconField addIconField(const char *name, const char *description) { return SkinIconField( mski.AddIconField(dlg, name, description) ); }
	SkinImageField addImageField(const char *name, const char *description) { return SkinImageField( mski.AddImageField(dlg, name, description) ); }

	SkinTextField getTextField(const char *name) { return SkinTextField( mski.GetField(dlg, name) ); }
	SkinIconField getIconField(const char *name) { return SkinIconField( mski.GetField(dlg, name) ); }
	SkinImageField getImageField(const char *name) { return SkinImageField( mski.GetField(dlg, name) ); }

private:
	SKINNED_DIALOG dlg;
};


#endif // __M_SKINS_CPP_H__