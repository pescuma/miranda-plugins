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

static INT_PTR CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

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
	odp.pszTab = LPGENT("General");
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

static void Tree_GetSelected(HWND tree, vector<HTREEITEM> &selected)
{
	HTREEITEM hItem = TreeView_GetRoot(tree);
	while (hItem)
	{
		if (IsSelected(tree, hItem))
			selected.push_back(hItem);
		hItem = TreeView_GetNextSibling(tree, hItem);
	}
}

static void Tree_Select(HWND tree, vector<HTREEITEM> &selected)
{
	for (unsigned int i = 0; i < selected.size(); i++)
		if (selected[i] != NULL)
			Tree_Select(tree, selected[i]);
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
				vector<HTREEITEM> selected;
				Tree_GetSelected(hwndDlg, selected);


				// Check if have to deselect it
				for (unsigned int i = 0; i < selected.size(); i++)
				{
					if (selected[i] == hti.hItem)
					{
						// Deselect it
						UnselectAll(hwndDlg);
						selected[i] = NULL;

						if (i > 0)
							hti.hItem = selected[0];

						else if (i + 1 < selected.size())
							hti.hItem = selected[i + 1];

						else
							hti.hItem = NULL;

						break;
					}
				}

				TreeView_SelectItem(hwndDlg, hti.hItem);
				Tree_Select(hwndDlg, selected);
			}
			else if (wParam & MK_SHIFT)
			{
				HTREEITEM hItem = TreeView_GetSelection(hwndDlg);
				if (hItem == NULL)
					break;

				vector<HTREEITEM> selected;
				Tree_GetSelected(hwndDlg, selected);

				TreeView_SelectItem(hwndDlg, hti.hItem);
				Tree_Select(hwndDlg, selected);
				Tree_SelectRange(hwndDlg, hItem, hti.hItem);
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

static HTREEITEM Tree_AddExtraIcon(HWND tree, BaseExtraIcon *extra, bool selected, HTREEITEM hAfter = TVI_LAST)
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
		BaseExtraIcon *extra = registeredExtraIcons[group[i] - 1];
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
	int ii = ids.at(0);
	ii = ids.at(1);
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

	for (size_t i = ids->size(); i > 0; --i)
	{
		BaseExtraIcon *extra = registeredExtraIcons[ids->at(i - 1) - 1];
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
	return registeredExtraIcons[a->at(0) - 1]->compare(registeredExtraIcons[b->at(0) - 1]);
}

static INT_PTR CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
			SetWindowLongPtr(tree, GWL_STYLE, GetWindowLong(tree, GWL_STYLE) | TVS_NOHSCROLL);

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

				HICON hIcon = IcoLib_LoadIcon(extra->getDescIcon());

				if (hIcon == NULL)
				{
					HICON hDefaultIcon = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_EMPTY), IMAGE_ICON, cx, cx,
							LR_DEFAULTCOLOR | LR_SHARED);
					ImageList_AddIcon(hImageList, hDefaultIcon);
					DestroyIcon(hDefaultIcon);
				}
				else
				{
					ImageList_AddIcon(hImageList, hIcon);
					IcoLib_ReleaseIcon(hIcon);
				}
			}
			TreeView_SetImageList(tree, hImageList, TVSIL_NORMAL);

			for (i = 0; i < extraIconsBySlot.size(); ++i)
			{
				ExtraIcon *extra = extraIconsBySlot[i];

				if (extra->getType() == EXTRAICON_TYPE_GROUP)
				{
					ExtraIconGroup *group = (ExtraIconGroup *) extra;
					vector<int> ids;
					for (unsigned int j = 0; j < group->items.size(); ++j)
						ids.push_back(group->items[j]->getID());
					Tree_AddExtraIconGroup(tree, ids, extra->isEnabled());
				}
				else
				{
					Tree_AddExtraIcon(tree, (BaseExtraIcon *) extra, extra->isEnabled());
				}
			}

			TVSORTCB sort = { 0 };
			sort.hParent = NULL;
			sort.lParam = 0;
			sort.lpfnCompare = CompareFunc;
			TreeView_SortChildrenCB(tree, &sort, 0);

			origTreeProc = (WNDPROC) SetWindowLongPtr(tree, -4, (INT_PTR)TreeProc);

			return TRUE;
		}
		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR) lParam;
			if (lpnmhdr->idFrom == 0)
			{
				if (lpnmhdr->code == (UINT) PSN_APPLY)
				{
					unsigned int i;

					HWND tree = GetDlgItem(hwndDlg, IDC_EXTRAORDER);


					// Store old slots
					int *oldSlots = new int[registeredExtraIcons.size()];
					int lastUsedSlot = -1;
					for (i = 0; i < registeredExtraIcons.size(); ++i)
					{
						if (extraIconsByHandle[i] == registeredExtraIcons[i])
							oldSlots[i] = registeredExtraIcons[i]->getSlot();
						else
							// Remove old slot for groups to re-set images
							oldSlots[i] = -1;
						lastUsedSlot = MAX(lastUsedSlot, registeredExtraIcons[i]->getSlot());
					}
					lastUsedSlot = MIN(lastUsedSlot, GetNumberOfSlots());


					// Get user data and create new groups
					vector<ExtraIconGroup *> groups;

					BYTE pos = 0;
					int firstEmptySlot = 0;
					HTREEITEM ht = TreeView_GetRoot(tree);
					TVITEM tvi = { 0 };
					tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
					tvi.stateMask = TVIS_STATEIMAGEMASK;
					while (ht)
					{
						tvi.hItem = ht;
						TreeView_GetItem(tree, &tvi);

						vector<int> *ids = (vector<int> *) tvi.lParam;
						if (ids == NULL || ids->size() < 1)
							continue; // ???

						bool enabled = ((tvi.state & INDEXTOSTATEIMAGEMASK(3)) == INDEXTOSTATEIMAGEMASK(2));
						int slot = (enabled ? firstEmptySlot++ : -1);
						if (slot >= GetNumberOfSlots())
							slot = -1;

						if (ids->size() == 1)
						{
							BaseExtraIcon *extra = registeredExtraIcons[ids->at(0) - 1];
							extra->setPosition(pos++);
							extra->setSlot(slot);
						}
						else
						{
							char name[128];
							mir_snprintf(name, MAX_REGS(name), "__group_%d", groups.size());

							ExtraIconGroup *group = new ExtraIconGroup(name);

							for (i = 0; i < ids->size(); ++i)
							{
								BaseExtraIcon *extra = registeredExtraIcons[ids->at(i) - 1];
								extra->setPosition(pos++);

								group->addExtraIcon(extra);
							}

							group->setSlot(slot);

							groups.push_back(group);
						}

						ht = TreeView_GetNextSibling(tree, ht);
					}

					// Store data
					for (i = 0; i < registeredExtraIcons.size(); ++i)
					{
						BaseExtraIcon *extra = registeredExtraIcons[i];

						char setting[512];
						mir_snprintf(setting, MAX_REGS(setting), "Position_%s", extra->getName());
						DBWriteContactSettingWord(NULL, MODULE_NAME, setting, extra->getPosition());

						mir_snprintf(setting, MAX_REGS(setting), "Slot_%s", extra->getName());
						DBWriteContactSettingWord(NULL, MODULE_NAME, setting, extra->getSlot());
					}

					CallService(MS_DB_MODULE_DELETE, 0, (LPARAM) MODULE_NAME "Groups");
					DBWriteContactSettingWord(NULL, MODULE_NAME "Groups", "Count", groups.size());
					for (i = 0; i < groups.size(); ++i)
					{
						ExtraIconGroup *group = groups[i];

						char setting[512];
						mir_snprintf(setting, MAX_REGS(setting), "%d_count", i);
						DBWriteContactSettingWord(NULL, MODULE_NAME "Groups", setting, group->items.size());

						for (unsigned int j = 0; j < group->items.size(); ++j)
						{
							BaseExtraIcon *extra = group->items[j];

							mir_snprintf(setting, MAX_REGS(setting), "%d_%d", i, j);
							DBWriteContactSettingString(NULL, MODULE_NAME "Groups", setting, extra->getName());
						}
					}

					// Clean removed slots
					for (int j = firstEmptySlot; j <= lastUsedSlot; ++j)
						RemoveExtraIcons(j);


					// Apply icons to new slots
					RebuildListsBasedOnGroups(groups);
					for (i = 0; i < extraIconsBySlot.size(); ++i)
					{
						ExtraIcon *extra = extraIconsBySlot[i];

						if (extra->getType() != EXTRAICON_TYPE_GROUP)
						{
							if (oldSlots[((BaseExtraIcon *) extra)->getID() - 1] == extra->getSlot())
								continue;
						}

						extra->applyIcons();
					}

					delete[] oldSlots;

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
							{
								GroupSelectedItems(tree);
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
							}
						}
						else if (sels == 1)
						{
							HTREEITEM hItem = TreeView_GetSelection(tree);
							vector<int> *ids = Tree_GetIDs(tree, hItem);
							if (ids->size() > 1)
							{
								if (ShowPopup(hwndDlg, 1) == ID_UNGROUP)
								{
									UngroupSelectedItems(tree);
									SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM) hwndDlg, 0);
								}
							}
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
