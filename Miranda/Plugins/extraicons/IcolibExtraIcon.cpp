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

IcolibExtraIcon::IcolibExtraIcon(const char *name, const char *description, const char *descIcon, int(*OnClick)(
		WPARAM wParam, LPARAM lParam)) :
	ExtraIcon(name, description, descIcon, OnClick)
{
	char setting[512];
	mir_snprintf(setting, MAX_REGS(setting), "%s/%s", MODULE_NAME, name);
	CallService(MS_DB_SETSETTINGRESIDENT, TRUE, (WPARAM) setting);
}

IcolibExtraIcon::~IcolibExtraIcon()
{
}

int IcolibExtraIcon::getType() const
{
	return EXTRAICON_TYPE_ICOLIB;
}

bool IcolibExtraIcon::needToRebuildIcons()
{
	return false;
}

void IcolibExtraIcon::rebuildIcons()
{
}

void IcolibExtraIcon::applyIcon(HANDLE hContact)
{
	if (!isEnabled() || hContact == NULL)
		return;

	HANDLE hImage = NULL;

	DBVARIANT dbv = { 0 };
	if (!DBGetContactSettingString(hContact, MODULE_NAME, name.c_str(), &dbv))
	{
		if (!IsEmpty(dbv.pszVal))
			hImage = GetIcon(dbv.pszVal);

		DBFreeVariant(&dbv);
	}

	Clist_SetExtraIcon(hContact, slot, hImage);
}

int IcolibExtraIcon::setIcon(HANDLE hContact, void *icon)
{
	if (hContact == NULL)
		return -1;

	const char *icolibName = (const char *) icon;

	if (isEnabled())
	{
		DBVARIANT dbv = { 0 };
		if (!DBGetContactSettingString(hContact, MODULE_NAME, name.c_str(), &dbv))
		{
			if (!IsEmpty(dbv.pszVal))
				RemoveIcon(dbv.pszVal);

			DBFreeVariant(&dbv);
		}
	}

	if (IsEmpty(icolibName))
		DBDeleteContactSetting(hContact, MODULE_NAME, name.c_str());
	else
		DBWriteContactSettingString(hContact, MODULE_NAME, name.c_str(), icolibName);

	if (isEnabled())
	{
		HANDLE hImage = NULL;
		if (!IsEmpty(icolibName))
			hImage = AddIcon(icolibName);

		return Clist_SetExtraIcon(hContact, slot, hImage);
	}

	return 0;
}

