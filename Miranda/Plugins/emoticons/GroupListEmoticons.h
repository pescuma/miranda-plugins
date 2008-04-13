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

	virtual void Load();

	void SetGroupName(Group &group, char *name);

	int CountGroups();

	void GetMaxEmoticonSize(Group &group);

	RECT GetEmoticonRect(Group &group, int index);

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
};
