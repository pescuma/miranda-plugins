#define MIN_COLS 6
#define MAX_LINES 6
#define MAX_COLS 12
#define BORDER 3


struct EmoticonSelectionData
{
	HANDLE hContact;
	Module *module;
	const char *proto;
	COLORREF background;

    int xPosition;
    int yPosition;
    int Direction;
    HWND hwndTarget;
    UINT targetMessage;
    LPARAM targetWParam;
};


class EmoticonsSelectionLayout
{
public:
	EmoticonSelectionData *ssd;
	HWND hwnd;
	int selection;

	struct {
		int width;
		int height;
	} window;

	EmoticonsSelectionLayout() {}
	virtual ~EmoticonsSelectionLayout() {}

	virtual void OnUp(HWND hwnd) = 0;
	virtual void OnDown(HWND hwnd) = 0;
	virtual void OnLeft(HWND hwnd) = 0;
	virtual void OnRight(HWND hwnd) = 0;

	virtual void Draw(HDC hdc) = 0;

	virtual void SetSelection(HWND hwnd, POINT p) = 0;

	void SetSelection(HWND hwnd, int sel);
	
	virtual void CreateToolTips() = 0;

	void DestroyToolTips();

protected:

	virtual HFONT GetFont(HDC hdc);
	virtual void ReleaseFont(HFONT hFont);

	RECT CalcRect(TCHAR *txt);
#ifdef UNICODE
	RECT CalcRect(char *txt);
#endif

	void GetEmoticonSize(Emoticon *e, int &width, int &height);

	int GetNumOfCols(int num_emotes);
	int GetNumOfLines(int num_emotes, int cols);

	void CreateEmoticonToolTip(Emoticon *e, RECT fr);

	void EraseBackground(HDC hdc);

	void DrawEmoticon(HDC hdc, int index, RECT rc);
	void DrawEmoticonText(HDC hdc, TCHAR *txt, RECT rc);
#ifdef UNICODE
	void DrawEmoticonText(HDC hdc, char *txt, RECT rc);
#endif

};
