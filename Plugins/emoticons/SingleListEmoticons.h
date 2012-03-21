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

	SingleListEmoticons(HWND hwnd, EmoticonSelectionData *ssd);
	virtual ~SingleListEmoticons();

	virtual void CreateToolTips();

	virtual void SetSelection(HWND hwnd, POINT p);

	virtual void OnUp(HWND hwnd);
	virtual void OnDown(HWND hwnd);
	virtual void OnLeft(HWND hwnd);
	virtual void OnRight(HWND hwnd);

	virtual void Draw(HDC hdc);

protected:

	void Load();

	RECT GetEmoticonRect(int line, int col);

	void GetMaxEmoticonSize();

	int GetEmoticonCountRoundedByCols();

};
