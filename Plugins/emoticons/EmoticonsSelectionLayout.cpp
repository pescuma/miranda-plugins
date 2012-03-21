#include "commons.h"

void EmoticonsSelectionLayout::SetSelection(HWND hwnd, int sel)
{
	if (sel < 0)
		sel = -1;
	if (sel >= ssd->module->emoticons.getCount())
		sel = -1;
	if (sel != selection)
	{
		InvalidateRect(hwnd, NULL, FALSE);
		selection = sel;
	}
}


void EmoticonsSelectionLayout::DestroyToolTips()
{
	for(int i = 0; i < ssd->module->emoticons.getCount(); i++)
	{
		Emoticon *e = ssd->module->emoticons[i];

		if (e->tt != NULL)
		{
			DestroyWindow(e->tt);
			e->tt = NULL;
		}
	}
}


HFONT EmoticonsSelectionLayout::GetFont(HDC hdc)
{
	HFONT hFont;

	if (ssd->hwndTarget != NULL)
	{
		CHARFORMAT2 cf;
		ZeroMemory(&cf, sizeof(cf));
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_FACE | CFM_ITALIC | CFM_CHARSET | CFM_FACE | CFM_WEIGHT | CFM_SIZE;
		SendMessage(ssd->hwndTarget, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

		LOGFONT lf = {0};
		lf.lfHeight = -MulDiv(cf.yHeight / 20, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		lf.lfWeight = cf.wWeight;
		lf.lfItalic = (cf.dwEffects & CFE_ITALIC) == CFE_ITALIC;
		lf.lfCharSet = cf.bCharSet;
		lf.lfPitchAndFamily = cf.bPitchAndFamily;
		lstrcpyn(lf.lfFaceName, cf.szFaceName, MAX_REGS(lf.lfFaceName));

		hFont = CreateFontIndirect(&lf);
	}
	else
	{
		hFont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);
	}

	return hFont;
}

void EmoticonsSelectionLayout::ReleaseFont(HFONT hFont)
{
	if (ssd->hwndTarget != NULL)
		DeleteObject(hFont);
}

RECT EmoticonsSelectionLayout::CalcRect(TCHAR *txt)
{
	HDC hdc = GetDC(hwnd);
	HFONT hFont = GetFont(hdc);
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	RECT rc = { 0, 0, 0xFFFF, 0xFFFF };
	DrawText(hdc, txt, lstrlen(txt), &rc, DT_CALCRECT | DT_NOPREFIX);

	SelectObject(hdc, oldFont);
	ReleaseFont(hFont);

	ReleaseDC(hwnd, hdc);
	return rc;
}

#ifdef UNICODE

RECT EmoticonsSelectionLayout::CalcRect(char *txt)
{
	HDC hdc = GetDC(hwnd);
	HFONT hFont = GetFont(hdc);
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	RECT rc = { 0, 0, 0xFFFF, 0xFFFF };
	DrawTextA(hdc, txt, strlen(txt), &rc, DT_CALCRECT | DT_NOPREFIX);

	SelectObject(hdc, oldFont);
	ReleaseFont(hFont);

	ReleaseDC(hwnd, hdc);
	return rc;
}

#endif

void EmoticonsSelectionLayout::GetEmoticonSize(Emoticon *e, int &width, int &height)
{
	width = 0;
	height = 0;

	if (e->img != NULL)
		e->img->Load(height, width);

	if (e->img == NULL || e->img->img == NULL)
	{
		if (e->texts.getCount() > 0)
		{
			RECT rc = CalcRect(e->texts[0]);
			height = rc.bottom - rc.top + 1;
			width = rc.right - rc.left + 1;
		}
		else
		{
			RECT rc = CalcRect(e->description);
			height = rc.bottom - rc.top + 1;
			width = rc.right - rc.left + 1;
		}
	}
}

int EmoticonsSelectionLayout::GetNumOfCols(int num_emotes)
{
	int cols = num_emotes / MAX_LINES;
	if (num_emotes % MAX_LINES != 0)
		cols++;

	if (cols < MAX_COLS)
	{
		cols = max(cols, MIN_COLS);
		for(int i = 2; i > -3; i--)
		{
			if (num_emotes % (cols+i) == 0)
			{
				cols += i;
				return cols;
			}
		}
	}

	if (num_emotes % cols <= max(2, cols /3))
		cols++;

	return min(max(MIN_COLS, min(cols, MAX_COLS)), num_emotes);
}

int EmoticonsSelectionLayout::GetNumOfLines(int num_emotes, int cols)
{
	int lines = num_emotes / cols;
	if (num_emotes % cols != 0)
		lines++;
	return lines;
}



HWND CreateTooltip(HWND hwnd, RECT &rect, TCHAR *text)
{
          // struct specifying control classes to register
    INITCOMMONCONTROLSEX iccex; 
    HWND hwndTT;                 // handle to the ToolTip control
          // struct specifying info about tool in ToolTip control
    TOOLINFO ti;
    unsigned int uid = 0;       // for ti initialization

	// Load the ToolTip class from the DLL.
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC  = ICC_BAR_CLASSES;

    if(!InitCommonControlsEx(&iccex))
       return NULL;

    /* CREATE A TOOLTIP WINDOW */
    hwndTT = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwnd,
        NULL,
        hInst,
        NULL
        );

	/* Gives problem with mToolTip
    SetWindowPos(hwndTT,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	*/

    /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hwnd;
    ti.hinst = hInst;
    ti.uId = uid;
    ti.lpszText = text;
        // ToolTip control will cover the whole window
    ti.rect.left = rect.left;    
    ti.rect.top = rect.top;
    ti.rect.right = rect.right;
    ti.rect.bottom = rect.bottom;

    /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
	SendMessage(hwndTT, TTM_SETDELAYTIME, (WPARAM) (DWORD) TTDT_AUTOPOP, (LPARAM) MAKELONG(24 * 60 * 60 * 1000, 0));	

	return hwndTT;
} 


void EmoticonsSelectionLayout::CreateEmoticonToolTip(Emoticon *e, RECT fr)
{
	Buffer<TCHAR> tt;
	if (e->description[0] != _T('\0'))
	{
		tt += _T(" ");
		tt += e->description;
		tt.translate();
		tt += _T(" ");
	}

	for(int k = 0; k < e->texts.getCount(); k++)
	{
		tt += _T(" ");
		tt += e->texts[k];
		tt += _T(" ");
	}
	tt.pack();

	e->tt = CreateTooltip(hwnd, fr, tt.str);
}

void EmoticonsSelectionLayout::EraseBackground(HDC hdc)
{
	RECT rc;
	GetClientRect(hwnd, &rc);

	HBRUSH hB = CreateSolidBrush(ssd->background);
	FillRect(hdc, &rc, hB);
	DeleteObject(hB);
}

void EmoticonsSelectionLayout::DrawEmoticonText(HDC hdc, TCHAR *txt, RECT rc)
{
	HFONT hFont = GetFont(hdc);
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	DrawText(hdc, txt, lstrlen(txt), &rc, DT_NOPREFIX);

	SelectObject(hdc, oldFont);
	ReleaseFont(hFont);
}

#ifdef UNICODE

void EmoticonsSelectionLayout::DrawEmoticonText(HDC hdc, char *txt, RECT rc)
{
	HFONT hFont = GetFont(hdc);
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	DrawTextA(hdc, txt, strlen(txt), &rc, DT_NOPREFIX);

	SelectObject(hdc, oldFont);
	ReleaseFont(hFont);
}

#endif

void EmoticonsSelectionLayout::DrawEmoticon(HDC hdc, int index, RECT fullRc)
{	
	Emoticon *e = ssd->module->emoticons[index];

	int width, height;
	GetEmoticonSize(e, width, height);

	RECT rc = fullRc;
	rc.left += (rc.right - rc.left - width) / 2;
	rc.top += (rc.bottom - rc.top - height) / 2;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	if (e->img == NULL || e->img->img == NULL)
	{
		if (e->texts.getCount() > 0)
		{
			DrawEmoticonText(hdc, e->texts[0], rc);
		}
		else
		{
			DrawEmoticonText(hdc, e->description, rc);
		}
	}
	else
	{
		HDC hdc_img = CreateCompatibleDC(hdc);
		HBITMAP old_bmp = (HBITMAP) SelectObject(hdc_img, e->img->img);

		if (e->img->transparent)
		{
			BLENDFUNCTION bf = {0};
			bf.SourceConstantAlpha = 255;
			bf.AlphaFormat = AC_SRC_ALPHA;
			AlphaBlend(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				hdc_img, 0, 0, rc.right - rc.left, rc.bottom - rc.top, bf);
		}
		else
		{
			BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				hdc_img, 0, 0, SRCCOPY);
		}

		SelectObject(hdc_img, old_bmp);
		DeleteDC(hdc_img);
	}

	if (selection == index)
	{
		rc = fullRc;
		rc.left -= 2;
		rc.right += 2;
		rc.top -= 2;
		rc.bottom += 2;
		FrameRect(hdc, &rc, (HBRUSH) GetStockObject(GRAY_BRUSH));
	}
}
