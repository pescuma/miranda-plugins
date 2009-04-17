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

static BOOL CALLBACK OptionsDlgProcOld(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int numSlots;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

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

					HWND tmp = CreateWindow("STATIC", Translate(desc),
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
			HWND cbl = (HWND) lParam;
			if (HIWORD(wParam) != CBN_SELCHANGE || cbl != GetFocus())
				return 0;

			int sel = SendMessage(cbl, CB_GETCURSEL, 0, 0);
			if (sel > 0)
			{
				for (int i = 0; i < numSlots; ++i)
				{
					int id = IDC_SLOT + i * 2;

					if (GetDlgItem(hwndDlg, id) == cbl)
						continue;

					int sl = SendDlgItemMessage(hwndDlg, id, CB_GETCURSEL, 0, 0);
					if (sl == sel)
						SendDlgItemMessage(hwndDlg, id, CB_SETCURSEL, 0, 0);
				}
			}

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR) lParam;

			if (lpnmhdr->idFrom == 0 && lpnmhdr->code == (UINT) PSN_APPLY)
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
			if (lpdis->itemID == (UINT) -1)
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

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ExtraIcon *a = (ExtraIcon *) lParam1;
	ExtraIcon *b = (ExtraIcon *) lParam2;
	return a->compare(b);
}

static BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int dragging = 0;
	static HANDLE hDragItem = NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);
			SetWindowLong(tree, GWL_STYLE, GetWindowLong(tree, GWL_STYLE) | TVS_NOHSCROLL);

			int cx = GetSystemMetrics(SM_CXSMICON);
			HIMAGELIST hImageList = ImageList_Create(cx, cx, ILC_COLOR32 | ILC_MASK, 2, 2);

			unsigned int i;
			for (i = 0; i < extraIcons.size(); ++i)
			{
				ExtraIcon *extra = extraIcons[i];

				HICON hIcon = NULL;
				if (!IsEmpty(extra->getDescIcon()))
					hIcon = (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM) extra->getDescIcon());

				if (hIcon == NULL)
				{
					HICON hDefaultIcon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_EMPTY), IMAGE_ICON, cx, cx,
							LR_DEFAULTCOLOR | LR_SHARED);
					ImageList_AddIcon(hImageList, hDefaultIcon);
					DestroyIcon(hDefaultIcon);
				}
				else
					ImageList_AddIcon(hImageList, hIcon);

				if (hIcon != NULL)
					CallService(MS_SKIN2_RELEASEICON, (WPARAM) hIcon, 0);
			}
			TreeView_SetImageList(tree, hImageList, TVSIL_NORMAL);

			TVINSERTSTRUCT tvis = { 0 };
			tvis.hParent = NULL;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
			tvis.item.stateMask = TVIS_STATEIMAGEMASK;
			for (i = 0; i < extraIcons.size(); ++i)
			{
				ExtraIcon *extra = extraIcons[i];

				tvis.item.lParam = (LPARAM) extra;
				tvis.item.pszText = (char *) extra->getDescription();
				tvis.item.iSelectedImage = tvis.item.iImage = i;
				tvis.item.state = INDEXTOSTATEIMAGEMASK(extra->getSlot() < 0 ? 1 : 2);
				TreeView_InsertItem(tree, &tvis);
			}

			TVSORTCB sort = { 0 };
			sort.hParent = NULL;
			sort.lParam = 0;
			sort.lpfnCompare = CompareFunc;
			TreeView_SortChildrenCB(tree, &sort, 0);

			return TRUE;
		}
		case WM_COMMAND:
		{
			//			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR) lParam;
			if (lpnmhdr->idFrom == 0)
			{
				if (lpnmhdr->code == (UINT) PSN_APPLY)
				{
					int *slots = new int[extraIcons.size()];

					unsigned int i;
					for (i = 0; i < extraIcons.size(); ++i)
						slots[i] = -1;

					HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);


					// Get positions and slots
					HTREEITEM ht = TreeView_GetRoot(tree);
					BYTE pos = 0;
					unsigned int firstEmptySlot = 0;
					TVITEM tvi = { 0 };
					tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
					tvi.stateMask = TVIS_STATEIMAGEMASK;
					while (ht)
					{
						tvi.hItem = ht;
						TreeView_GetItem(tree, &tvi);

						ExtraIcon *extra = (ExtraIcon *) tvi.lParam;
						extra->setPosition(pos);
						if ((tvi.state & INDEXTOSTATEIMAGEMASK(3)) == INDEXTOSTATEIMAGEMASK(2))
							slots[extra->getID() - 1] = firstEmptySlot++;

						ht = TreeView_GetNextSibling(tree, ht);
						pos++;
					}

					// Clean removed slots
					for (int j = firstEmptySlot; j < GetNumberOfSlots(); ++j)
					{
						if (GetExtraIconBySlot(j) != NULL)
							// Had and icon and lost
							RemoveExtraIcons(j);
					}

					// Apply icons to new slots
					for (i = 0; i < extraIcons.size(); ++i)
					{
						ExtraIcon *extra = extraIcons[i];

						char setting[512];
						mir_snprintf(setting, MAX_REGS(setting), "Position_%s", extra->getName());
						DBWriteContactSettingWord(NULL, MODULE_NAME, setting, extra->getPosition());

						int oldSlot = extra->getSlot();
						if (oldSlot == slots[i])
							continue;

						extra->setSlot(slots[i]);

						mir_snprintf(setting, MAX_REGS(setting), "Slot_%s", extra->getName());
						DBWriteContactSettingWord(NULL, MODULE_NAME, setting, extra->getSlot());

						if (slots[i] < 0)
							continue;

						if (oldSlot < 0 && extra->needToRebuildIcons())
							extra->rebuildIcons();
						extra->applyIcons();
					}

					delete[] slots;

					return TRUE;
				}
			}
			else if (lpnmhdr->idFrom == IDC_EXTRAORDER)
			{
				HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);

				switch (lpnmhdr->code)
				{
					case TVN_BEGINDRAG:
					{
						SetCapture(hwndDlg);
						dragging = 1;
						hDragItem = ((LPNMTREEVIEWA) lParam)->itemNew.hItem;
						TreeView_SelectItem(tree, hDragItem);
						break;
					}
					case NM_CLICK:
					{
						DWORD pos = GetMessagePos();

						TVHITTESTINFO hti;
						hti.pt.x = (short) LOWORD(pos);
						hti.pt.y = (short) HIWORD(pos);
						ScreenToClient(lpnmhdr->hwndFrom, &hti.pt);
						if (TreeView_HitTest(lpnmhdr->hwndFrom, &hti))
						{
							if (hti.flags & TVHT_ONITEMSTATEICON)
							{
								TreeView_SelectItem(tree, hti.hItem);
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
							}
						}

						break;
					}
					case TVN_KEYDOWN:
					{
						TV_KEYDOWN *nmkd = (TV_KEYDOWN *) lpnmhdr;
						if (nmkd->wVKey == VK_SPACE)
						{
							// Determine the selected tree item.
							HTREEITEM item = TreeView_GetSelection(tree);
							if (item != NULL)
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
						}
						break;
					}
					case NM_RCLICK:
					{
						HTREEITEM hSelected = (HTREEITEM) SendMessage(tree, TVM_GETNEXTITEM, TVGN_DROPHILITE, 0);
						if (hSelected != NULL)
							TreeView_SelectItem(tree, hSelected);
						break;
					}
				}
			}

			break;
		}
		case WM_MOUSEMOVE:
		{
			if (!dragging)
				break;

			HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);

			TVHITTESTINFO hti;
			hti.pt.x = (short) LOWORD(lParam);
			hti.pt.y = (short) HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(tree, &hti.pt);
			TreeView_HitTest(tree, &hti);
			if (hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT))
			{
				HTREEITEM it = hti.hItem;
				hti.pt.y -= TreeView_GetItemHeight(tree) / 2;
				TreeView_HitTest(tree, &hti);
				if (!(hti.flags & TVHT_ABOVE))
					TreeView_SetInsertMark(tree, hti.hItem, 1);
				else
					TreeView_SetInsertMark(tree, it, 0);
			}
			else
			{
				if (hti.flags & TVHT_ABOVE)
					SendDlgItemMessage(hwndDlg, IDC_EXTRAORDER, WM_VSCROLL, MAKEWPARAM(SB_LINEUP,0), 0);
				if (hti.flags & TVHT_BELOW)
					SendDlgItemMessage(hwndDlg, IDC_EXTRAORDER, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN,0), 0);
				TreeView_SetInsertMark(tree, NULL, 0);
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			if (!dragging)
				break;

			HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);

			TreeView_SetInsertMark(tree, NULL, 0);
			dragging = 0;
			ReleaseCapture();

			TVHITTESTINFO hti;
			hti.pt.x = (short) LOWORD(lParam);
			hti.pt.y = (short) HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(tree, &hti.pt);
			hti.pt.y -= TreeView_GetItemHeight(tree) / 2;
			TreeView_HitTest(tree,&hti);
			if (hDragItem == hti.hItem)
				break;

			if (!(hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT | TVHT_ABOVE | TVHT_BELOW)))
				break;

			if (hti.flags & TVHT_ABOVE)
				hti.hItem = TVI_FIRST;
			else if (hti.flags & TVHT_BELOW)
				hti.hItem = TVI_LAST;

			TVINSERTSTRUCT tvis;
			TCHAR name[128];
			tvis.item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
			tvis.item.stateMask = 0xFFFFFFFF;
			tvis.item.pszText = name;
			tvis.item.cchTextMax = MAX_REGS(name);
			tvis.item.hItem = (HTREEITEM) hDragItem;
			TreeView_GetItem(tree, &tvis.item);

			TreeView_DeleteItem(tree, hDragItem);

			tvis.hParent = NULL;
			tvis.hInsertAfter = hti.hItem;
			TreeView_SelectItem(tree, TreeView_InsertItem(tree, &tvis));

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);

			break;
		}
	}

	return 0;
}
