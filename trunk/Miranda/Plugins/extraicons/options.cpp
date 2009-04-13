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

#ifndef PSM_CHANGED
#define PSM_CHANGED             (WM_USER + 104)
#endif

#ifndef PSN_APPLY
#define PSN_APPLY               ((0U-200U)-2)
#endif

#define ICON_SIZE 				16

// Prototypes /////////////////////////////////////////////////////////////////////////////////////

HANDLE hOptHook = NULL;

static BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// Functions //////////////////////////////////////////////////////////////////////////////////////


int InitOptionsCallback(WPARAM wParam, LPARAM lParam)
{
	if (GetNumberOfSlots() < 1)
		return 0;

	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.pszGroup = LPGENT("Contact List");
	odp.pszTitle = LPGENT("Extra icons");
	//	odp.pszTitle = LPGENT("Contact List");
	//	odp.pszTab = LPGENT("Extra icons");
	odp.pfnDlgProc = OptionsDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}

void InitOptions()
{
	hOptHook = HookEvent(ME_OPT_INITIALISE, InitOptionsCallback);
}

void DeInitOptions()
{
	UnhookEvent(hOptHook);
}

BOOL ScreenToClient(HWND hWnd, LPRECT lpRect)
{
	BOOL ret;
	POINT pt;

	pt.x = lpRect->left;
	pt.y = lpRect->top;

	ret = ScreenToClient(hWnd, &pt);

	if (!ret)
		return ret;

	lpRect->left = pt.x;
	lpRect->top = pt.y;

	pt.x = lpRect->right;
	pt.y = lpRect->bottom;

	ret = ScreenToClient(hWnd, &pt);

	lpRect->right = pt.x;
	lpRect->bottom = pt.y;

	return ret;
}

static void RemoveExtraIcons(int slot)
{
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		Clist_SetExtraIcon(hContact, slot, NULL);

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
}

static BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int numSlots;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			numSlots = GetNumberOfSlots();

			RECT rcLabel;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_SLOT_L), &rcLabel);
			ScreenToClient(hwndDlg, &rcLabel);

			RECT rcCombo;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_SLOT), &rcCombo);
			ScreenToClient(hwndDlg, &rcCombo);

			HFONT hFont = (HFONT) SendMessage(hwndDlg, WM_GETFONT, 0, 0);

			int height = MAX(rcLabel.bottom - rcLabel.top, rcCombo.bottom - rcCombo.top) + 3;

			for (int i = 0; i < numSlots; ++i)
			{
				int id = IDC_SLOT + i * 2;


				// Create controls
				if (i > 0)
				{
					char desc[256];
					mir_snprintf(desc, MAX_REGS(desc), "Slot %d:", i + 1);

					HWND tmp = CreateWindow("STATIC", desc,
							WS_CHILD | WS_VISIBLE,
							rcLabel.left, rcLabel.top + i * height,
							rcLabel.right - rcLabel.left, rcLabel.bottom - rcLabel.top,
							hwndDlg, 0, hInst, NULL);
					SendMessage(tmp, WM_SETFONT, (WPARAM) hFont, FALSE);

					HWND combo = CreateWindow("COMBOBOX", "",
							WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL
							| CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
							rcCombo.left, rcCombo.top + i * height,
							rcCombo.right - rcCombo.left, rcCombo.bottom - rcCombo.top,
							hwndDlg, (HMENU) id, hInst, NULL);
					SendMessage(combo, WM_SETFONT, (WPARAM) hFont, FALSE);
				}

				// Fill combo
				int sel = 0;
				SendDlgItemMessage(hwndDlg, id, CB_ADDSTRING, 0, (LPARAM) Translate("<Empty>"));
				for (int j = 0; j < (int) extraIcons.size(); ++j)
				{
					ExtraIcon *extra = extraIcons[j];

					int pos = SendDlgItemMessage(hwndDlg, id, CB_ADDSTRING, 0, (LPARAM) extra->getDescription());
					SendDlgItemMessage(hwndDlg, id, CB_SETITEMDATA, pos, (DWORD) extra);

					if (extra->getSlot() == i)
						sel = j + 1;
				}
				SendDlgItemMessage(hwndDlg, id, CB_SETCURSEL, sel, 0);
			}

			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) != CBN_SELCHANGE || (HWND) lParam != GetFocus())
				return 0;

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR) lParam;

			if (lpnmhdr->idFrom == 0 && lpnmhdr->code == PSN_APPLY)
			{
				int * slots = new int[extraIcons.size()];

				int i;
				for (i = 0; i < (int) extraIcons.size(); ++i)
					slots[i] = -1;

				for (i = 0; i < (int) extraIcons.size(); ++i)
				{
					if (slots[i] != -1)
						continue;

					for (int j = 0; j < numSlots; ++j)
					{
						if (SendDlgItemMessage(hwndDlg, IDC_SLOT + j * 2, CB_GETCURSEL, 0, 0) == i + 1)
						{
							slots[i] = j;
							break;
						}
					}
				}

				for (int j = 0; j < numSlots; ++j)
				{
					// Has icon?
					bool found = false;
					for (i = 0; !found && i < (int) extraIcons.size(); ++i)
						found = (slots[i] == j);
					if (found)
						continue;

					// Had icon?
					if (GetExtraIconBySlot(j) == NULL)
						continue;


					// Had and icon and lost
					RemoveExtraIcons(j);
				}

				for (i = 0; i < (int) extraIcons.size(); ++i)
				{
					ExtraIcon *extra = extraIcons[i];

					int oldSlot = extra->getSlot();
					if (oldSlot == slots[i])
						continue;

					extra->setSlot(slots[i]);

					char setting[512];
					mir_snprintf(setting, MAX_REGS(setting), "Slot_%s", extra->getName());
					DBWriteContactSettingWord(NULL, MODULE_NAME, setting, extra->getSlot());

					if (oldSlot < 0 && extra->needToRebuildIcons())
						extra->rebuildIcons();
					extra->applyIcons();
				}

				delete[] slots;
			}

			break;
		}
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
			if ((lpdis->CtlID % 2) != 0)
				break;
			int slot = (lpdis->CtlID - IDC_SLOT) / 2;
			if (slot < 0 || slot > numSlots * 2)
				break;
			if (lpdis->itemID == -1)
				break;

			ExtraIcon *extra = (ExtraIcon *) lpdis->itemData;

			TEXTMETRIC tm;
			RECT rc;

			GetTextMetrics(lpdis->hDC, &tm);

			COLORREF clrfore = SetTextColor(lpdis->hDC, GetSysColor(lpdis->itemState & ODS_SELECTED
					? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
			COLORREF clrback = SetBkColor(lpdis->hDC, GetSysColor(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT
					: COLOR_WINDOW));

			FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(lpdis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT
					: COLOR_WINDOW));

			rc.left = lpdis->rcItem.left + 2;


			// Draw icon
			HICON hIcon = NULL;
			if (extra != NULL && !IsEmpty(extra->getDescIcon()))
				hIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) extra->getDescIcon());
			if (hIcon != NULL)
			{
				rc.top = (lpdis->rcItem.bottom + lpdis->rcItem.top - ICON_SIZE) / 2;
				DrawIconEx(lpdis->hDC, rc.left, rc.top, hIcon, 16, 16, 0, NULL, DI_NORMAL);
				CallService(MS_SKIN2_RELEASEICON, (WPARAM) hIcon, 0);
			}

			rc.left += ICON_SIZE + 4;


			// Draw text
			rc.right = lpdis->rcItem.right - 2;
			rc.top = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
			rc.bottom = rc.top + tm.tmHeight;
			DrawText(lpdis->hDC, extra == NULL ? Translate("<Empty>") : extra->getDescription(), -1, &rc,
					DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);


			// Restore old colors
			SetTextColor(lpdis->hDC, clrfore);
			SetBkColor(lpdis->hDC, clrback);

			return TRUE;
		}

		case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;
			if ((lpmis->CtlID % 2) != 0)
				break;
			int slot = (lpmis->CtlID - IDC_SLOT) / 2;
			if (slot < 0 || slot > numSlots * 2)
				break;

			TEXTMETRIC tm;
			GetTextMetrics(GetDC(hwndDlg), &tm);

			lpmis->itemHeight = MAX(ICON_SIZE, tm.tmHeight);

			return TRUE;
		}
	}

	return 0;
}
