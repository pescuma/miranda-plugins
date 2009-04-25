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

/*
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
 */

#ifndef TVIS_FOCUSED
#define TVIS_FOCUSED	1
#endif

WNDPROC origTreeProc;

static bool IsSelected(HWND tree, HTREEITEM hItem)
{
	return (TVIS_SELECTED & TreeView_GetItemState(tree, hItem, TVIS_SELECTED)) == TVIS_SELECTED;
}

static void Tree_Select(HWND tree, HTREEITEM hItem)
{
	TreeView_SetItemState(tree, hItem, TVIS_SELECTED, TVIS_SELECTED);
}

static void Tree_Unselect(HWND tree, HTREEITEM hItem)
{
	TreeView_SetItemState(tree, hItem, 0, TVIS_SELECTED);
}

static void Tree_DropHilite(HWND tree, HTREEITEM hItem)
{
	TreeView_SetItemState(tree, hItem, TVIS_DROPHILITED, TVIS_DROPHILITED);
}

static void Tree_DropUnhilite(HWND tree, HTREEITEM hItem)
{
	TreeView_SetItemState(tree, hItem, 0, TVIS_DROPHILITED);
}

static void UnselectAll(HWND tree)
{
	TreeView_SelectItem(tree, NULL);

	HTREEITEM hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		Tree_Unselect(tree, hItem);
		hItem = TreeView_GetNextSibling(tree, hItem);
	}
}

static void Tree_SelectRange(HWND tree, HTREEITEM hStart, HTREEITEM hEnd)
{
	int start = 0;
	int end = 0;
	int i = 0;
	HTREEITEM hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (hItem == hStart)
			start = i;
		if (hItem == hEnd)
			end = i;

		i++;
		hItem = TreeView_GetNextSibling(tree, hItem);
	}

	if (end < start)
	{
		int tmp = start;
		start = end;
		end = tmp;
	}

	i = 0;
	hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (i >= start)
			Tree_Select(tree, hItem);
		if (i == end)
			break;

		i++;
		hItem = TreeView_GetNextSibling(tree, hItem);
	}
}

static int GetNumSelected(HWND tree)
{
	int ret = 0;
	HTREEITEM hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (IsSelected(tree, hItem))
			ret++;
		hItem = TreeView_GetNextSibling(tree, hItem);
	}
	return ret;
}

LRESULT CALLBACK TreeProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_LBUTTONDOWN:
		{
			DWORD pos = (DWORD) lParam;

			TVHITTESTINFO hti;
			hti.pt.x = (short) LOWORD(pos);
			hti.pt.y = (short) HIWORD(pos);
			if (!TreeView_HitTest(hwndDlg, &hti))
			{
				UnselectAll(hwndDlg);
				break;
			}

			if (!(wParam & (MK_CONTROL | MK_SHIFT)) || !(hti.flags & (TVHT_ONITEMICON | TVHT_ONITEMLABEL
					| TVHT_ONITEMRIGHT)))
			{
				UnselectAll(hwndDlg);
				TreeView_SelectItem(hwndDlg, hti.hItem);
				break;
			}

			if (wParam & MK_CONTROL)
			{
				HTREEITEM hItem = TreeView_GetSelection(hwndDlg);
				TreeView_SelectItem(hwndDlg, hti.hItem);
				if (hItem != NULL)
					Tree_Select(hwndDlg, hItem);
			}
			else if (wParam & MK_SHIFT)
			{
				HTREEITEM hItem = TreeView_GetSelection(hwndDlg);
				if (hItem == NULL)
					break;

				TreeView_SelectItem(hwndDlg, NULL);
				Tree_SelectRange(hwndDlg, hItem, hti.hItem);
				TreeView_SelectItem(hwndDlg, hti.hItem);
			}

			return 0;
		}
	}

	return CallWindowProc(origTreeProc, hwndDlg, msg, wParam, lParam);
}

static vector<int> * Tree_GetIDs(HWND tree, HTREEITEM hItem)
{
	TVITEM tvi = { 0 };
	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = hItem;
	TreeView_GetItem(tree, &tvi);

	return (vector<int> *) tvi.lParam;
}

static HTREEITEM Tree_AddExtraIcon(HWND tree, ExtraIcon *extra, bool selected, HTREEITEM hAfter = TVI_LAST)
{
	vector<int> *ids = new vector<int> ;
	ids->push_back(extra->getID());

	TVINSERTSTRUCT tvis = { 0 };
	tvis.hParent = NULL;
	tvis.hInsertAfter = hAfter;
	tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	tvis.item.iSelectedImage = tvis.item.iImage = extra->getID();
	tvis.item.lParam = (LPARAM) ids;
	tvis.item.pszText = (char *) extra->getDescription();
	tvis.item.state = INDEXTOSTATEIMAGEMASK(selected ? 2 : 1);
	return TreeView_InsertItem(tree, &tvis);
}

static HTREEITEM Tree_AddExtraIconGroup(HWND tree, vector<int> &group, bool selected, HTREEITEM hAfter = TVI_LAST)
{
	vector<int> *ids = new vector<int> ;
	string desc;
	int img = 0;
	for (unsigned int i = 0; i < group.size(); ++i)
	{
		ExtraIcon *extra = GetExtraIcon((HANDLE) group[i]);
		ids->push_back(extra->getID());

		if (img == 0 && !IsEmpty(extra->getDescIcon()))
			img = extra->getID();

		if (i > 0)
			desc += " / ";
		desc += extra->getDescription();
	}

	TVINSERTSTRUCT tvis = { 0 };
	tvis.hParent = NULL;
	tvis.hInsertAfter = hAfter;
	tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	tvis.item.iSelectedImage = tvis.item.iImage = img;
	tvis.item.lParam = (LPARAM) ids;
	tvis.item.pszText = (char *) desc.c_str();
	tvis.item.state = INDEXTOSTATEIMAGEMASK(selected ? 2 : 1);
	return TreeView_InsertItem(tree, &tvis);
}

static void GroupSelectedItems(HWND tree)
{
	vector<HTREEITEM> toRemove;
	vector<int> ids;
	bool selected = false;
	HTREEITEM hPlace = NULL;


	// Find items

	HTREEITEM hItem = TreeView_GetRoot(tree);
	TVITEM tvi = { 0 };
	tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
	while (hItem)
	{
		if (IsSelected(tree, hItem))
		{
			if (hPlace == NULL)
				hPlace = hItem;

			tvi.hItem = hItem;
			TreeView_GetItem(tree, &tvi);

			vector<int> *iids = (vector<int> *) tvi.lParam;
			ids.insert(ids.end(), iids->begin(), iids->end());

			if ((tvi.state & INDEXTOSTATEIMAGEMASK(3)) == INDEXTOSTATEIMAGEMASK(2))
				selected = true;

			toRemove.push_back(hItem);
		}

		hItem = TreeView_GetNextSibling(tree, hItem);
	}

	if (hPlace == NULL)
		return; // None selected

	// Add new
	HTREEITEM hNew = Tree_AddExtraIconGroup(tree, ids, selected, hPlace);


	// Remove old
	for (unsigned int i = 0; i < toRemove.size(); ++i)
	{
		delete Tree_GetIDs(tree, toRemove[i]);
		TreeView_DeleteItem(tree, toRemove[i]);
	}

	// Select
	UnselectAll(tree);
	TreeView_SelectItem(tree, hNew);
}

static void UngroupSelectedItems(HWND tree)
{
	HTREEITEM hItem = TreeView_GetSelection(tree);
	if (hItem == NULL)
		return;
	vector<int> *ids = Tree_GetIDs(tree, hItem);
	if (ids->size() < 2)
		return;

	bool selected = IsSelected(tree, hItem);

	for (unsigned int i = ids->size(); i > 0; --i)
	{
		ExtraIcon *extra = GetExtraIcon((HANDLE) (*ids)[i - 1]);
		Tree_AddExtraIcon(tree, extra, selected, hItem);
	}

	delete Tree_GetIDs(tree, hItem);
	TreeView_DeleteItem(tree, hItem);

	UnselectAll(tree);
}

static int ShowPopup(HWND hwndDlg, int popup)
{
	// Fix selection
	HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);
	HTREEITEM hSelected = (HTREEITEM) SendMessage(tree, TVM_GETNEXTITEM, TVGN_DROPHILITE, 0);
	HTREEITEM hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (hItem != hSelected && IsSelected(tree, hItem))
			Tree_DropHilite(tree, hItem);
		hItem = TreeView_GetNextSibling(tree, hItem);
	}
//	InvalidateRect(tree, NULL, FALSE);

	HMENU menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_OPT_POPUP));
	HMENU submenu = GetSubMenu(menu, popup);
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) submenu, 0);

	DWORD pos = GetMessagePos();
	int ret = TrackPopupMenu(submenu, TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_LEFTALIGN, LOWORD(pos),
			HIWORD(pos), 0, hwndDlg, NULL);

	DestroyMenu(menu);

	// Revert selection
	hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (hItem != hSelected && IsSelected(tree, hItem))
			Tree_DropUnhilite(tree, hItem);
		hItem = TreeView_GetNextSibling(tree, hItem);
	}

	return ret;
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	vector<int> *a = (vector<int> *) lParam1;
	vector<int> *b = (vector<int> *) lParam2;
	int aid = (*a)[0];
	int bid = (*b)[0];
	return GetExtraIcon((HANDLE) aid)->compare(GetExtraIcon((HANDLE) bid));
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

			int numSlots = GetNumberOfSlots();
			if (numSlots < (int) registeredExtraIcons.size())
			{
				char txt[512];
				mir_snprintf(txt, MAX_REGS(txt), Translate("* only the first %d icons will be shown"), numSlots);

				HWND label = GetDlgItem(hwndDlg, IDC_MAX_ICONS_L);
				SetWindowText(label, txt);
				ShowWindow(label, SW_SHOW);
			}

			HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);
			SetWindowLong(tree, GWL_STYLE, GetWindowLong(tree, GWL_STYLE) | TVS_NOHSCROLL);

			int cx = GetSystemMetrics(SM_CXSMICON);
			HIMAGELIST hImageList = ImageList_Create(cx, cx, ILC_COLOR32 | ILC_MASK, 2, 2);

			HICON hDefaultIcon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_EMPTY), IMAGE_ICON, cx, cx,
					LR_DEFAULTCOLOR | LR_SHARED);
			ImageList_AddIcon(hImageList, hDefaultIcon);
			DestroyIcon(hDefaultIcon);

			unsigned int i;
			for (i = 0; i < registeredExtraIcons.size(); ++i)
			{
				ExtraIcon *extra = registeredExtraIcons[i];

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

			for (i = 0; i < extraIcons.size(); ++i)
			{
				ExtraIcon *extra = extraIcons[i];

				if (extra->getType() == EXTRAICON_TYPE_GROUP)
				{
					ExtraIconGroup *group = (ExtraIconGroup *) extra;
					vector<int> ids;
					for (unsigned int j = 0; j < group->items.size(); ++j)
						ids.push_back(group->items[j]->getID());
					Tree_AddExtraIconGroup(tree, ids, extra->getSlot() >= 0);
				}
				else
				{
					Tree_AddExtraIcon(tree, extra, extra->getSlot() >= 0);
				}
			}

			TVSORTCB sort = { 0 };
			sort.hParent = NULL;
			sort.lParam = 0;
			sort.lpfnCompare = CompareFunc;
			TreeView_SortChildrenCB(tree, &sort, 0);

			origTreeProc = (WNDPROC) SetWindowLong(tree, GWL_WNDPROC, (LONG) TreeProc);

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
					int *slots = new int[registeredExtraIcons.size()];

					unsigned int i;
					for (i = 0; i < registeredExtraIcons.size(); ++i)
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
					for (i = 0; i < registeredExtraIcons.size(); ++i)
					{
						ExtraIcon *extra = registeredExtraIcons[i];

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
							HTREEITEM hItem = TreeView_GetSelection(tree);
							if (hItem != NULL)
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
						}
						break;
					}
					case NM_RCLICK:
					{
						HTREEITEM hSelected = (HTREEITEM) SendMessage(tree, TVM_GETNEXTITEM, TVGN_DROPHILITE, 0);
						if (hSelected != NULL && !IsSelected(tree, hSelected))
						{
							UnselectAll(tree);
							TreeView_SelectItem(tree, hSelected);
						}

						int sels = GetNumSelected(tree);
						if (sels > 1)
						{
							if (ShowPopup(hwndDlg, 0) == ID_GROUP)
								GroupSelectedItems(tree);
						}
						else if (sels == 1)
						{
							HTREEITEM hItem = TreeView_GetSelection(tree);
							vector<int> *ids = Tree_GetIDs(tree, hItem);
							if (ids->size() > 1)
								if (ShowPopup(hwndDlg, 1) == ID_UNGROUP)
									UngroupSelectedItems(tree);
						}
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
			TCHAR name[512];
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
		case WM_DESTROY:
		{
			HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);
			HTREEITEM hItem = TreeView_GetRoot(tree);
			while (hItem)
			{
				delete Tree_GetIDs(tree, hItem);
				hItem = TreeView_GetNextSibling(tree, hItem);
			}

			break;
		}
	}

	return 0;
}
