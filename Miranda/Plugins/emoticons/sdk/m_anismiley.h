#ifndef m_anismiley_h__
#define m_anismiley_h__


#define IASF_UNICODE 1
#ifdef UNICODE
#define IASF_TCHAR IASF_UNICODE
#else
#define IASF_TCHAR 0
#endif

typedef struct _tagINSERTANISMILEY
{
    size_t  cbSize;
    HWND    hWnd;
    union
    {
        char *    szFilename;
        wchar_t * wcFilename;
        TCHAR *   tcsFilename;
    };
    COLORREF    dwBackColor;
    int         nHeight;
    DWORD       dwFlags;
    union
    {
        char *    szText;
        wchar_t * wcText;
        TCHAR *   tcsText;
    };
}INSERTANISMILEY;
#define MS_INSERTANISMILEY "mAnimator/InsertSmiley"

#if defined ServiceExists && defined CallService && !defined InsertAnimatedSmiley

#ifdef _cplusplus
extern "C" {
#endif
static BOOL InsertAnimatedSmiley(HWND _hwnd, TCHAR * _szFilename, COLORREF _dwBack, int _nHeight, TCHAR * _szText)
    {
        static int bServiceExists=-1;        
        INSERTANISMILEY ias={0};
        if (bServiceExists==-1)
            bServiceExists=ServiceExists(MS_INSERTANISMILEY);        
        if (!bServiceExists) return FALSE;
        ias.cbSize=sizeof(INSERTANISMILEY);
        ias.hWnd=_hwnd;
        ias.tcsFilename=_szFilename;
        ias.dwFlags=IASF_TCHAR;
        ias.nHeight=_nHeight;
        ias.dwBackColor=_dwBack;
        ias.tcsText=_szText;
        return (BOOL) CallService(MS_INSERTANISMILEY,(WPARAM)&ias, 0);
    };
#ifdef _cplusplus
};
#endif
#endif // defined ServiceExists && defined CallService


#if !defined(NM_FIREVIEWCHANGE)

/**
	NM_FIREVIEWCHANGE is WM_NOTIFY Message for notify parent of host window about smiley are going to be repaint

	The proposed action is next: Owner of RichEdit windows received NM_FIREVIEWCHANGE through WM_NOTIFY
	twice first time before painting|invalidating (FVCN_PREFIRE) and second time - after (FVCN_POSTFIRE).
	The Owner window may change any values of received FVCNDATA_NMHDR structure in order to raise needed action.
	For example it may substitute FVCA_INVALIDATE to FVCA_CUSTOMDRAW event to force painting on self offscreen context.
	
	It can be:
	FVCA_CUSTOMDRAW - in this case you need to provide valid HDC to draw on and valid RECT of smiley
	FVCA_INVALIDATE - to invalidate specified rect of window 
	FVCA_NONE		- skip any action. But be aware - animation will be stopped till next repainting of smiley.
	FVCA_SENDVIEWCHANGE - to notify richedit ole about object changed. Be aware Richedit will fully reconstruct itself

	Another point is moment of received smiley rect - it is only valid if FVCA_DRAW is initially set, 
	and it is PROBABLY valid if FVCA_INVALIDATE is set. And it most probably invalid in case of FVCA_SENDVIEWCHANGE.
	The smiley position is relative last full paint HDC. Usually it is relative to top-left corner of host 
	richedit (NOT it client area) in windows coordinates. 

*/

// Type of Event one of
#define FVCN_PREFIRE        1
#define FVCN_POSTFIRE       2

// Action of event are going to be done
#define FVCA_NONE           0
#define FVCA_DRAW           1   // do not modify hdc in case of _DRAW, Use _CUSTOMDRAW
#define FVCA_CUSTOMDRAW     2
#define FVCA_INVALIDATE     3
#define FVCA_SENDVIEWCHANGE 4
#define FVCA_SKIPDRAW		5

// Extended NMHDR structure for WM_NOTIFY
typedef struct  
{   
    //NMHDR structure
    HWND hwndFrom;              // Window of smiley host
    UINT idFrom;                // ignored
    UINT code;                  // NM_FIREVIEWCHANGE
    
    size_t      cbSize;
    BYTE        bEvent;         // FVCN_ value - pre- or post- painting
    BYTE        bAction;        // FVCA_ keys
    HDC         hDC;            // Canvas to draw on
    RECT        rcRect;         // Valid/should be in case of FVCA_DRAW
    COLORREF    clrBackground;  // color to fill background if fTransparent is not set
    BOOL        fTransparent;   // if need to fill back color
    LPARAM      lParam;         // used by host window PreFire and PostFire event
} FVCNDATA_NMHDR;

// Code of WM_NOTIFY message (code)
#define NM_FIREVIEWCHANGE   NM_FIRST+1;

#endif

#endif // m_anismiley_h__