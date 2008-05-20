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


#include "commons.h"



// Prototypes /////////////////////////////////////////////////////////////////////////////////////

HANDLE hOptHook = NULL;

Options opts;

static BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK ChatDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


BOOL AllowProtocol(const char *proto)
{
	if ((CallProtoService(proto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_IM) == 0)
		return FALSE;

	return TRUE;
}


BOOL ChatAllowProtocol(const char *proto)
{
	if ((CallProtoService(proto, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_CHAT) == 0)
		return FALSE;

	return TRUE;
}


static OptPageControl optionsControls[] = { 
	{ &opts.filename_pattern,		CONTROL_TEXT,			IDC_FILENAME,		"FilenamePattern", (DWORD) _T("Log\\%group%\\%contact%.msgs") },
	{ &opts.ident_multiline_msgs,	CONTROL_CHECKBOX,		IDC_IDENT_MULTILINE,"IdentMultilineMsgs", TRUE },
	{ NULL,							CONTROL_PROTOCOL_LIST,	IDC_PROTOCOLS,		"Enable%s", TRUE, (int)AllowProtocol }
};


static OptPageControl chatControls[] = { 
	{ &opts.chat_filename_pattern,		CONTROL_TEXT,			IDC_FILENAME,		"ChatFilenamePattern", (DWORD) _T("Log\\Group Chats\\%session%.msgs") },
	{ &opts.chat_ident_multiline_msgs,	CONTROL_CHECKBOX,		IDC_IDENT_MULTILINE,"ChatIdentMultilineMsgs", TRUE },
	{ NULL,								CONTROL_PROTOCOL_LIST,	IDC_PROTOCOLS,		"ChatEnable%s", TRUE, (int)ChatAllowProtocol }
};



// Functions //////////////////////////////////////////////////////////////////////////////////////


int InitOptionsCallback(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};
    odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.ptszGroup = _T("History");
	odp.ptszTitle = _T("Disk Log");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS);
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;

	odp.ptszTab = _T("Messages");
	odp.pfnDlgProc = OptionsDlgProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.ptszTab = _T("Group Chat");
	odp.pfnDlgProc = ChatDlgProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}


void LoadOptions()
{
	LoadOpts(optionsControls, MAX_REGS(optionsControls), MODULE_NAME);
	LoadOpts(chatControls, MAX_REGS(chatControls), MODULE_NAME);
}


void InitOptions()
{
	LoadOptions();

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

	if (!ret) return ret;

	lpRect->left = pt.x;
	lpRect->top = pt.y;


	pt.x = lpRect->right;
	pt.y = lpRect->bottom;

	ret = ScreenToClient(hWnd, &pt);

	lpRect->right = pt.x;
	lpRect->bottom = pt.y;

	return ret;
}


static void GetTextMetric(HFONT hFont, TEXTMETRIC *tm)
{
	HDC hdc = GetDC(NULL);
	HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);
	GetTextMetrics(hdc, tm);
	SelectObject(hdc, hOldFont);
	ReleaseDC(NULL, hdc);
}


static BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	static int avaiable = 0;
	static int total = 0;
	static int current = 0;
	static int lineHeigth = 0;
	static int eventTypeCount = 0;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			RECT rc;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_EVENT_TYPES), &rc);

			POINT pt = { rc.left, rc.bottom + 5 };
			ScreenToClient(hwndDlg, &pt);
			int origY = pt.y;

			GetClientRect(hwndDlg, &rc);

			HFONT hFont = (HFONT) SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			TEXTMETRIC font;
			GetTextMetric(hFont, &font);

			int height = max(font.tmHeight, 16) + 4;
			int width = rc.right - rc.left - 50;

			lineHeigth = height;

			// Create all items
			int id = IDC_EVENT_TYPES + 100;
			eventTypeCount = CallService(MS_HISTORYEVENTS_GET_COUNT, 0, 0);
			for (int k = 0; k < eventTypeCount; k++)
			{
				const HISTORY_EVENT_HANDLER *heh = (const HISTORY_EVENT_HANDLER *) CallService(MS_HISTORYEVENTS_GET_EVENT, k, -1);

				int x = pt.x + 20;

				// Event type

				HWND chk = CreateWindow(_T("BUTTON"), TranslateT(""), 
							WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX | BS_AUTOCHECKBOX, 
						    x, pt.y, 20, height, hwndDlg, (HMENU) id, hInst, NULL);
				SendMessage(chk, WM_SETFONT, (WPARAM) hFont, FALSE);
				SendMessage(chk, BM_SETCHECK, IsEventEnabled(heh->eventType) ? BST_CHECKED : BST_UNCHECKED, 0);
				x += 20;
				id ++;

				HWND icon = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_ICON | SS_CENTERIMAGE, 
                        x, pt.y + (height - 16) / 2, 16, 16, hwndDlg, NULL, hInst, NULL);
				x += 20;

				SendMessage(icon, STM_SETICON, (WPARAM) CallService(MS_HISTORYEVENTS_GET_ICON, heh->eventType, TRUE), 0);

				HWND tmp = CreateWindowA("STATIC", heh->description, WS_CHILD | WS_VISIBLE, 
                        x, pt.y + (height - font.tmHeight) / 2, width - (x - pt.x), font.tmHeight, 
						hwndDlg, NULL, hInst, NULL);
				SendMessage(tmp, WM_SETFONT, (WPARAM) hFont, FALSE);

				pt.y += height;
			}

			avaiable = rc.bottom - rc.top;
			total = pt.y;
			current = 0;

			SCROLLINFO si; 
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
			si.nMin   = 0; 
			si.nMax   = total; 
			si.nPage  = avaiable; 
			si.nPos   = current; 
			SetScrollInfo(hwndDlg, SB_VERT, &si, TRUE); 

			break;
		}

		case WM_VSCROLL: 
		{ 
			int yDelta;     // yDelta = new_pos - current_pos 
			int yNewPos;    // new position 
 
			switch (LOWORD(wParam)) 
			{ 
				case SB_PAGEUP: 
					yNewPos = current - avaiable / 2; 
					break;  
				case SB_PAGEDOWN: 
					yNewPos = current + avaiable / 2; 
					break; 
				case SB_LINEUP: 
					yNewPos = current - lineHeigth; 
					break; 
				case SB_LINEDOWN: 
					yNewPos = current + lineHeigth; 
					break; 
				case SB_THUMBPOSITION: 
					yNewPos = HIWORD(wParam); 
					break; 
				case SB_THUMBTRACK:
					yNewPos = HIWORD(wParam); 
					break;
				default: 
					yNewPos = current; 
			} 

			yNewPos = min(total - avaiable, max(0, yNewPos)); 
 
			if (yNewPos == current) 
				break; 
 
			yDelta = yNewPos - current; 
			current = yNewPos; 
 
			// Scroll the window. (The system repaints most of the 
			// client area when ScrollWindowEx is called; however, it is 
			// necessary to call UpdateWindow in order to repaint the 
			// rectangle of pixels that were invalidated.) 
 
			ScrollWindowEx(hwndDlg, 0, -yDelta, (CONST RECT *) NULL, 
				(CONST RECT *) NULL, (HRGN) NULL, (LPRECT) NULL, 
				SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN); 
			InvalidateRect(hwndDlg, NULL, FALSE);
			//UpdateWindow(hwndDlg); 
 
			// Reset the scroll bar. 
 
			SCROLLINFO si; 
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_POS; 
			si.nPos   = current; 
			SetScrollInfo(hwndDlg, SB_VERT, &si, TRUE); 

			break; 
		}

		case WM_COMMAND:
		{
			if ((HWND) lParam != GetFocus())
				break;

			int id = LOWORD(wParam);
			if (id >= IDC_EVENT_TYPES + 100 && id < IDC_EVENT_TYPES + 100 + eventTypeCount)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;
			if (lpnmhdr->idFrom != 0 || lpnmhdr->code != PSN_APPLY)
				break;

			// Create all items
			int id = IDC_EVENT_TYPES + 100;
			for (int k = 0; k < eventTypeCount; k++)
			{
				const HISTORY_EVENT_HANDLER *heh = (const HISTORY_EVENT_HANDLER *) CallService(MS_HISTORYEVENTS_GET_EVENT, k, -1);
				SetEventEnabled(heh->eventType, IsDlgButtonChecked(hwndDlg, id));
				id ++;
			}

			break;
		}

	}

	return SaveOptsDlgProc(optionsControls, MAX_REGS(optionsControls), MODULE_NAME, hwndDlg, msg, wParam, lParam);
}



struct
{
	int type;
	TCHAR *name;
} 
CHAT_EVENTS[] = 
{
	{ GC_EVENT_MESSAGE, _T("Message") },
	{ GC_EVENT_ACTION, _T("Action") },
	{ GC_EVENT_JOIN, _T("User joined") },
	{ GC_EVENT_PART, _T("User left") },
	{ GC_EVENT_QUIT, _T("User disconnected") },
	{ GC_EVENT_KICK, _T("User kicked") },
	{ GC_EVENT_NICK, _T("Nickname change") },
	{ GC_EVENT_NOTICE, _T("Notice") },
	{ GC_EVENT_TOPIC, _T("Topic change") },
	{ GC_EVENT_INFORMATION, _T("Information message") },
	{ GC_EVENT_ADDSTATUS, _T("Status enabled") },
	{ GC_EVENT_REMOVESTATUS, _T("Status disabled") }
};

static BOOL CALLBACK ChatDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	static int avaiable = 0;
	static int total = 0;
	static int current = 0;
	static int lineHeigth = 0;
	static int eventTypeCount = 0;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			RECT rc;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_EVENT_TYPES), &rc);

			POINT pt = { rc.left, rc.bottom + 5 };
			ScreenToClient(hwndDlg, &pt);
			int origY = pt.y;

			GetClientRect(hwndDlg, &rc);

			HFONT hFont = (HFONT) SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			TEXTMETRIC font;
			GetTextMetric(hFont, &font);

			int height = max(font.tmHeight, 16) + 4;
			int width = rc.right - rc.left - 50;

			lineHeigth = height;

			// Create all items
			int id = IDC_EVENT_TYPES + 100;
			eventTypeCount = MAX_REGS(CHAT_EVENTS);
			for (int k = 0; k < eventTypeCount; k++)
			{
				int x = pt.x + 20;

				// Event type

				HWND chk = CreateWindow(_T("BUTTON"), TranslateTS(CHAT_EVENTS[k].name), 
							WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX | BS_AUTOCHECKBOX, 
						    x, pt.y, width - (x - pt.x), height, hwndDlg, (HMENU) id, hInst, NULL);
				SendMessage(chk, WM_SETFONT, (WPARAM) hFont, FALSE);
				SendMessage(chk, BM_SETCHECK, IsChatEventEnabled(CHAT_EVENTS[k].type) ? BST_CHECKED : BST_UNCHECKED, 0);
				id ++;

				pt.y += height;
			}

			avaiable = rc.bottom - rc.top;
			total = pt.y;
			current = 0;

			SCROLLINFO si; 
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
			si.nMin   = 0; 
			si.nMax   = total; 
			si.nPage  = avaiable; 
			si.nPos   = current; 
			SetScrollInfo(hwndDlg, SB_VERT, &si, TRUE); 

			break;
		}

		case WM_VSCROLL: 
		{ 
			int yDelta;     // yDelta = new_pos - current_pos 
			int yNewPos;    // new position 
 
			switch (LOWORD(wParam)) 
			{ 
				case SB_PAGEUP: 
					yNewPos = current - avaiable / 2; 
					break;  
				case SB_PAGEDOWN: 
					yNewPos = current + avaiable / 2; 
					break; 
				case SB_LINEUP: 
					yNewPos = current - lineHeigth; 
					break; 
				case SB_LINEDOWN: 
					yNewPos = current + lineHeigth; 
					break; 
				case SB_THUMBPOSITION: 
					yNewPos = HIWORD(wParam); 
					break; 
				case SB_THUMBTRACK:
					yNewPos = HIWORD(wParam); 
					break;
				default: 
					yNewPos = current; 
			} 

			yNewPos = min(total - avaiable, max(0, yNewPos)); 
 
			if (yNewPos == current) 
				break; 
 
			yDelta = yNewPos - current; 
			current = yNewPos; 
 
			// Scroll the window. (The system repaints most of the 
			// client area when ScrollWindowEx is called; however, it is 
			// necessary to call UpdateWindow in order to repaint the 
			// rectangle of pixels that were invalidated.) 
 
			ScrollWindowEx(hwndDlg, 0, -yDelta, (CONST RECT *) NULL, 
				(CONST RECT *) NULL, (HRGN) NULL, (LPRECT) NULL, 
				SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN); 
			InvalidateRect(hwndDlg, NULL, FALSE);
			//UpdateWindow(hwndDlg); 
 
			// Reset the scroll bar. 
 
			SCROLLINFO si; 
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_POS; 
			si.nPos   = current; 
			SetScrollInfo(hwndDlg, SB_VERT, &si, TRUE); 

			break; 
		}

		case WM_COMMAND:
		{
			if ((HWND) lParam != GetFocus())
				break;

			int id = LOWORD(wParam);
			if (id >= IDC_EVENT_TYPES + 100 && id < IDC_EVENT_TYPES + 100 + eventTypeCount)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR lpnmhdr = (LPNMHDR)lParam;
			if (lpnmhdr->idFrom != 0 || lpnmhdr->code != PSN_APPLY)
				break;

			// Create all items
			int id = IDC_EVENT_TYPES + 100;
			for (int k = 0; k < eventTypeCount; k++)
			{
				SetChatEventEnabled(CHAT_EVENTS[k].type, IsDlgButtonChecked(hwndDlg, id));
				id ++;
			}

			break;
		}

	}

	return SaveOptsDlgProc(chatControls, MAX_REGS(chatControls), MODULE_NAME, hwndDlg, msg, wParam, lParam);
}
