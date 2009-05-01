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

ExtraIconGroup::ExtraIconGroup(const char *name) :
	ExtraIcon(name), setValidExtraIcon(false)
{
	char setting[512];
	mir_snprintf(setting, MAX_REGS(setting), "%s/%s", MODULE_NAME, name);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (WPARAM) setting);
}

ExtraIconGroup::~ExtraIconGroup()
{
	items.clear();
}

void ExtraIconGroup::addExtraIcon(BaseExtraIcon *extra)
{
	items.push_back(extra);

	description = "";
	for (unsigned int i = 0; i < items.size(); ++i)
	{
		if (i > 0)
			description += " / ";
		description += items[i]->getDescription();
	}
}

void ExtraIconGroup::rebuildIcons()
{
	for (unsigned int i = 0; i < items.size(); ++i)
		items[i]->rebuildIcons();
}

void ExtraIconGroup::applyIcon(HANDLE hContact)
{
	if (hContact == NULL)
		return;

	setValidExtraIcon = false;

	unsigned int i;
	for (i = 0; i < items.size(); ++i)
	{
		items[i]->applyIcon(hContact);

		if (setValidExtraIcon)
			break;
	}

	DBWriteContactSettingDword(hContact, MODULE_NAME, name.c_str(), setValidExtraIcon ? items[i]->getID() : 0);
}

void ExtraIconGroup::setSlot(int slot)
{
	ExtraIcon::setSlot(slot);

	for (unsigned int i = 0; i < items.size(); ++i)
		items[i]->setSlot(slot);
}

ExtraIcon * ExtraIconGroup::getCurrentItem(HANDLE hContact) const
{
	int id = (int) DBGetContactSettingDword(hContact, MODULE_NAME, name.c_str(), 0);
	if (id < 1)
		return NULL;

	for (unsigned int i = 0; i < items.size(); ++i)
		if (id == items[i]->getID())
			return items[i];

	return NULL;
}

void ExtraIconGroup::onClick(HANDLE hContact)
{
	ExtraIcon *extra = getCurrentItem(hContact);
	if (extra != NULL)
		extra->onClick(hContact);
}

int ExtraIconGroup::setIcon(int id, HANDLE hContact, void *icon)
{
	ExtraIcon *current = getCurrentItem(hContact);
	int currentPos = items.size();
	ExtraIcon *store = NULL;
	int storePos = items.size();
	for (unsigned int i = 0; i < items.size(); ++i)
	{
		if (items[i]->getID() == id)
		{
			store = items[i];
			storePos = i;
		}

		if (items[i] == current)
		{
			currentPos = i;
		}
	}

	if (store == NULL)
	{
		return -1;
	}
	else if (storePos < currentPos)
	{
		DBWriteContactSettingDword(hContact, MODULE_NAME, name.c_str(), id);
		return store->setIcon(id, hContact, icon);
	}
	else if (storePos == currentPos) // store == current
	{
		return store->setIcon(id, hContact, icon);
	}
	else // if (storePos > currentPos)
	{
		store->storeIcon(hContact, icon);
		return 0;
	}
}

void ExtraIconGroup::storeIcon(HANDLE hContact, void *icon)
{
}

const char *ExtraIconGroup::getDescription() const
{
	return description.c_str();
}

const char *ExtraIconGroup::getDescIcon() const
{
	for (unsigned int i = 0; i < items.size(); ++i)
		if (!IsEmpty(items[i]->getDescIcon()))
			return items[i]->getDescIcon();

	return "";
}

int ExtraIconGroup::getType() const
{
	return EXTRAICON_TYPE_GROUP;
}

int ExtraIconGroup::ClistSetExtraIcon(HANDLE hContact, HANDLE hImage)
{
	if (hImage != NULL && hImage != (HANDLE) -1)
		setValidExtraIcon = true;

	return Clist_SetExtraIcon(hContact, slot, hImage);
}

