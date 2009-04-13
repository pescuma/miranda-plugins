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

#ifndef __EXTRAICON_H__
#define __EXTRAICON_H__

#include <string>

class ExtraIcon
{
public:
	ExtraIcon(const char *name, const char *description, const char *descIcon, int(*OnClick)(WPARAM wParam,
			LPARAM lParam));
	virtual ~ExtraIcon();

	virtual bool needToRebuildIcons() =0;
	virtual void rebuildIcons() =0;
	virtual void applyIcons();
	virtual void applyIcon(HANDLE hContact) =0;
	virtual void onClick(HANDLE hContact);

	virtual int setIcon(HANDLE hContact, void *icon) =0;

	virtual const char *getName() const;
	virtual const char *getDescription() const;
	virtual void setDescription(const char *desc);
	virtual const char *getDescIcon() const;
	virtual void setDescIcon(const char *icon);
	virtual int getType() const =0;

	virtual int getSlot() const;
	virtual void setSlot(int slot);

	virtual bool isEnabled() const;

protected:
	std::string name;
	std::string description;
	std::string descIcon;
	int(*OnClick)(WPARAM wParam, LPARAM lParam);

	int slot;
};

#endif // __EXTRAICON_H__
