class GroupListEmoticons : public EmoticonsSelectionLayout
{
public:
	struct Group {
		TCHAR *name;
		int start;
		int end;
		int count;
		int max_height;
		int max_width;
		int lines;
		int cols;
		int top;
	};

	struct {
		int line;
		int col;
	} sel;

	int num_groups;
	Group *groups;

	HFONT groupFont;
	HFONT emoticonFont;
	COLORREF groupBkgColor;
	COLORREF emoticonColor;
	COLORREF groupColor;

	GroupListEmoticons(HWND hwnd, EmoticonSelectionData *ssd);
	virtual ~GroupListEmoticons();

	virtual void CreateToolTips();

	virtual void SetSelection(HWND hwnd, POINT p);

	int GetIndex(Group &group, int col, int line);
	int GetGroupByEmoticonIndex(int index);
	int HeightWithBorders(RECT rc);

	virtual void OnUp(HWND hwnd);
	virtual void OnDown(HWND hwnd);
	virtual void OnLeft(HWND hwnd);
	virtual void OnRight(HWND hwnd);

	virtual void Draw(HDC hdc);

protected:

	void Load();

	RECT CalcRect(TCHAR *txt, HFONT hFont);
	void DrawEmoticonText(HDC hdc, TCHAR *txt, RECT rc, HFONT hFont);

	virtual HFONT GetFont(HDC hdc);
	virtual void ReleaseFont(HFONT hFont);

	void SetGroupName(Group &group, char *name);

	int CountGroups();

	void GetMaxEmoticonSize(Group &group);

	RECT GetEmoticonRect(Group &group, int index);
};
