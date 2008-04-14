#include "commons.h"




HBITMAP CreateBitmap32(int cx, int cy)
{
   BITMAPINFO RGB32BitsBITMAPINFO; 
    UINT * ptPixels;
    HBITMAP DirectBitmap;

    ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
    RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    RGB32BitsBITMAPINFO.bmiHeader.biWidth=cx;//bm.bmWidth;
    RGB32BitsBITMAPINFO.bmiHeader.biHeight=cy;//bm.bmHeight;
    RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
    RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;

    DirectBitmap = CreateDIBSection(NULL, 
                                    (BITMAPINFO *)&RGB32BitsBITMAPINFO, 
                                    DIB_RGB_COLORS,
                                    (void **)&ptPixels, 
                                    NULL, 0);
    return DirectBitmap;
}


void AssertInsideScreen(RECT &rc)
{
	// Make sure it is inside screen
	if (IsWinVer98Plus()) {
		static BOOL loaded = FALSE;
		static HMONITOR (WINAPI *MyMonitorFromRect)(LPCRECT,DWORD) = NULL;
		static BOOL (WINAPI *MyGetMonitorInfo)(HMONITOR,LPMONITORINFO) = NULL;

		if (!loaded) {
			HMODULE hUser32 = GetModuleHandleA("user32");
			if (hUser32) {
				MyMonitorFromRect = (HMONITOR(WINAPI*)(LPCRECT,DWORD))GetProcAddress(hUser32,"MonitorFromRect");
				MyGetMonitorInfo = (BOOL(WINAPI*)(HMONITOR,LPMONITORINFO))GetProcAddress(hUser32,"GetMonitorInfoA");
				if (MyGetMonitorInfo == NULL)
					MyGetMonitorInfo = (BOOL(WINAPI*)(HMONITOR,LPMONITORINFO))GetProcAddress(hUser32,"GetMonitorInfo");
			}
			loaded = TRUE;
		}

		if (MyMonitorFromRect != NULL && MyGetMonitorInfo != NULL) {
			HMONITOR hMonitor;
			MONITORINFO mi;

			hMonitor = MyMonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
			mi.cbSize = sizeof(mi);
			MyGetMonitorInfo(hMonitor, &mi);

			if (rc.bottom > mi.rcWork.bottom)
				OffsetRect(&rc, 0, mi.rcWork.bottom - rc.bottom);
			if (rc.bottom < mi.rcWork.top)
				OffsetRect(&rc, 0, mi.rcWork.top - rc.top);
			if (rc.top > mi.rcWork.bottom)
				OffsetRect(&rc, 0, mi.rcWork.bottom - rc.bottom);
			if (rc.top < mi.rcWork.top)
				OffsetRect(&rc, 0, mi.rcWork.top - rc.top);
			if (rc.right > mi.rcWork.right)
				OffsetRect(&rc, mi.rcWork.right - rc.right, 0);
			if (rc.right < mi.rcWork.left)
				OffsetRect(&rc, mi.rcWork.left - rc.left, 0);
			if (rc.left > mi.rcWork.right)
				OffsetRect(&rc, mi.rcWork.right - rc.right, 0);
			if (rc.left < mi.rcWork.left)
				OffsetRect(&rc, mi.rcWork.left - rc.left, 0);
		}
	}
}


DWORD ConvertServiceParam(EmoticonsSelectionLayout *layout, char *param)
{
	DWORD ret;
	if (param == NULL)
		ret = 0;
	else if (stricmp("hContact", param) == 0)
		ret = (DWORD) layout->ssd->hContact;
	else
		ret = atoi(param);
	return ret;
}


INT_PTR CALLBACK EmoticonSeletionDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	switch(msg) 
	{
	case WM_INITDIALOG: 
		{
			EmoticonSelectionData *ssd = (EmoticonSelectionData *) lParam;

			// Get background
			ssd->background = RGB(255, 255, 255);
			if (ssd->hwndTarget != NULL)
			{
				ssd->background = SendMessage(ssd->hwndTarget, EM_SETBKGNDCOLOR, 0, ssd->background);
				SendMessage(ssd->hwndTarget, EM_SETBKGNDCOLOR, 0, ssd->background);
			}

			// Create positioning object
			EmoticonsSelectionLayout *layout = new GroupListEmoticons();
			layout->ssd = ssd;
			layout->hwnd = hwnd;
			SetWindowLong(hwnd, GWL_USERDATA, (LONG) layout);

			layout->Load();

			// Calc position

			int x = ssd->xPosition;
			int y = ssd->yPosition;
			switch (ssd->Direction) 
			{
			case 1: 
				x -= layout->window.width;
				break;
			case 2:
				x -= layout->window.width;
				y -= layout->window.height;
				break;
			case 3:
				y -= layout->window.height;
				break;
			}

			RECT rc = { x, y, x + layout->window.width + 2, y + layout->window.height + 2 };
			AssertInsideScreen(rc);
			SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);

			layout->CreateToolTips();

			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.hwndTrack = hwnd;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);

			return TRUE;
		}

	case WM_PAINT:
		{
			RECT r;
			if (GetUpdateRect(hwnd, &r, FALSE)) 
			{
				EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);

				PAINTSTRUCT ps;

				HDC hdc_orig = BeginPaint(hwnd, &ps);

				RECT rc;
				GetClientRect(hwnd, &rc);

				// Create double buffer
				HDC hdc = CreateCompatibleDC(hdc_orig);
				HBITMAP hBmp = CreateBitmap32(rc.right, rc.bottom);
				SelectObject(hdc, hBmp);

				SetBkMode(hdc, TRANSPARENT);

				layout->Draw(hdc);

				// Copy buffer to screen
				BitBlt(hdc_orig, rc.left, rc.top, rc.right - rc.left, 
					rc.bottom - rc.top, hdc, rc.left, rc.top, SRCCOPY);

				DeleteDC(hdc);
				DeleteObject(hBmp);

				EndPaint(hwnd, &ps);
			}

			return TRUE;
		}

	case WM_MOUSELEAVE:
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER;
			tme.hwndTrack = hwnd;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
		}
	case WM_NCMOUSEMOVE:
		{
			EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);
			layout->SetSelection(hwnd, -1);
			break;
		}

	case WM_MOUSEHOVER:
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
		}
	case WM_MOUSEMOVE:
		{
			EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);

			POINT p;
			p.x = LOWORD(lParam); 
			p.y = HIWORD(lParam); 

			layout->SetSelection(hwnd, p);

			break;
		}

	case WM_GETDLGCODE:
		{
			if (lParam != NULL)
			{
				static DWORD last_time = 0;

				MSG *msg = (MSG* ) lParam;
				if (msg->message == WM_KEYDOWN && msg->time != last_time)
				{
					last_time = msg->time;
					EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);

					if (msg->wParam == VK_UP)
					{
						layout->OnUp(hwnd);
					}
					else if (msg->wParam == VK_DOWN)
					{
						layout->OnDown(hwnd);
					}
					else if (msg->wParam == VK_LEFT)
					{
						layout->OnLeft(hwnd);
					}
					else if (msg->wParam == VK_RIGHT)
					{
						layout->OnRight(hwnd);
					}
					else if (msg->wParam == VK_HOME)
					{
						layout->SetSelection(hwnd, 0);
					}
					else if (msg->wParam == VK_END)
					{
						layout->SetSelection(hwnd, layout->ssd->module->emoticons.getCount() - 1);
					}
				}
			}

			return DLGC_WANTALLKEYS;
		}
		
	    case WM_ACTIVATE:
		{
			if (wParam == WA_INACTIVE) 
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) 
			{
				case IDOK:
					PostMessage(hwnd, WM_LBUTTONUP, 0, 0);
					break;

				case IDCANCEL:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					break;
			}
			break;
		}

		case WM_LBUTTONUP:
		{
			EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);
			if (layout->selection >= 0 && layout->ssd->hwndTarget != NULL)
			{
				Emoticon *e = layout->ssd->module->emoticons[layout->selection];

				if (e->service[0] != NULL)
				{
					CallProtoService(layout->ssd->module->name, e->service[0],
									 ConvertServiceParam(layout, e->service[1]), 
									 ConvertServiceParam(layout, e->service[2]));
				}
				else if (opts.only_replace_isolated)
				{
					TCHAR tmp[64];
					mir_sntprintf(tmp, MAX_REGS(tmp), _T(" %s "), e->texts[0]);
					SendMessage(layout->ssd->hwndTarget, layout->ssd->targetMessage, layout->ssd->targetWParam, (LPARAM) tmp);
				}
				else
					SendMessage(layout->ssd->hwndTarget, layout->ssd->targetMessage, layout->ssd->targetWParam, (LPARAM) e->texts[0]);
			}

			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}

		case WM_CLOSE:
		{
			EmoticonsSelectionLayout *layout = (EmoticonsSelectionLayout *) GetWindowLong(hwnd, GWL_USERDATA);
			SetWindowLong(hwnd, GWL_USERDATA, NULL);

			layout->DestroyToolTips();

			DestroyWindow(hwnd);

			SetFocus(layout->ssd->hwndTarget);
			delete layout->ssd;
			delete layout;
			break;
		}
	}

	return FALSE;
}

int ShowSelectionService(WPARAM wParam, LPARAM lParam)
{
    SMADD_SHOWSEL3 *sss = (SMADD_SHOWSEL3 *)lParam;
	if (sss == NULL || sss->cbSize < sizeof(SMADD_SHOWSEL3)) 
		return FALSE;

	const char *proto = NULL;
	HANDLE hContact = GetRealContact(sss->hContact);
	if (hContact != NULL)
		proto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if (proto == NULL)
		proto = sss->Protocolname;
	if (proto == NULL)
		return FALSE;

	Module *m = GetModule(proto);
	if (m == NULL)
		return FALSE;
	else if (m->emoticons.getCount() <= 0)
		return FALSE;

	EmoticonSelectionData * ssd = new EmoticonSelectionData();
	ssd->module = m;
	ssd->hContact = hContact;

	ssd->xPosition = sss->xPosition;
	ssd->yPosition = sss->yPosition;
	ssd->Direction = sss->Direction;

	ssd->hwndTarget = sss->hwndTarget;
	ssd->targetMessage = sss->targetMessage;
	ssd->targetWParam = sss->targetWParam;

	CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_EMOTICON_SELECTION), sss->hwndParent, 
					  EmoticonSeletionDlgProc, (LPARAM) ssd);

    return TRUE;
}

