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

#include "commons.h"

ExtraIcon::ExtraIcon(const char *name, const char *description, const char *descIcon, int(*OnClick)(WPARAM wParam,
		LPARAM lParam)) :
	name(name), description(Translate(description)), descIcon(descIcon), OnClick(OnClick), slot(-1)
{
}

ExtraIcon::~ExtraIcon()
{
}

const char *ExtraIcon::getName() const
{
	return name.c_str();
}

const char *ExtraIcon::getDescription() const
{
	return description.c_str();
}

const char *ExtraIcon::getDescIcon() const
{
	return descIcon.c_str();
}

int ExtraIcon::getSlot() const
{
	return slot;
}

void ExtraIcon::setSlot(int slot)
{
	this->slot = slot;
}

bool ExtraIcon::isEnabled() const
{
	return slot >= 0;
}

void ExtraIcon::applyIcons()
{
	if (!isEnabled())
		return;

	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		// Clear to assert that it will be cleared
		Clist_SetExtraIcon(hContact, slot, NULL);

		applyIcon(hContact);

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

void ExtraIcon::onClick(HANDLE hContact)
{
	if (OnClick == NULL)
		return;

	OnClick((WPARAM) hContact, (LPARAM) ConvertToClistSlot(slot));
}

