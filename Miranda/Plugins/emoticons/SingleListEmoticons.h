class SingleListEmoticons : public EmoticonsSelectionLayout
{
public:
	struct {
		int count;
		int max_height;
		int max_width;
		int lines;
		int cols;
	} emoticons;

	virtual void Load();

	void GetMaxEmoticonSize();

	int GetEmoticonCountRoundedByCols();

	virtual void CreateToolTips();

	virtual void SetSelection(HWND hwnd, POINT p);

	virtual void OnUp(HWND hwnd);
	virtual void OnDown(HWND hwnd);
	virtual void OnLeft(HWND hwnd);
	virtual void OnRight(HWND hwnd);

	virtual void Draw(HDC hdc);

	RECT GetEmoticonRect(int line, int col);

};
