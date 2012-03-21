#include "commons.h"

SingleListEmoticons::SingleListEmoticons(HWND hwnd, EmoticonSelectionData *ssd)
{
	this->hwnd = hwnd;
	this->ssd = ssd;
	Load();
}


SingleListEmoticons::~SingleListEmoticons()
{
}


void SingleListEmoticons::GetMaxEmoticonSize()
{
	emoticons.max_height = 4;
	emoticons.max_width = 4;

	for(int i = 0; i < emoticons.count; i++)
	{
		Emoticon *e = ssd->module->emoticons[i];

		if (e->service[0] != NULL && !ProtoServiceExists(ssd->proto, e->service[0]))
			continue;

		int height, width;
		GetEmoticonSize(e, width, height);

		emoticons.max_height = max(emoticons.max_height, height);
		emoticons.max_width = max(emoticons.max_width, width);
	}
}

int SingleListEmoticons::GetEmoticonCountRoundedByCols() 
{
	int roundCount = ssd->module->emoticons.getCount();
	if (roundCount % emoticons.cols != 0)
		roundCount += emoticons.cols - (roundCount % emoticons.cols);
	return roundCount;
}

void SingleListEmoticons::CreateToolTips()
{
	for(int line = 0; line < emoticons.lines; line++)
	{
		for(int col = 0; col < emoticons.cols; col++)
		{
			int index = line * emoticons.cols + col;
			if (index >= emoticons.count)
				break;

			Emoticon *e = ssd->module->emoticons[index];

			if (e->service[0] != NULL && !ProtoServiceExists(ssd->proto, e->service[0]))
				continue;

			CreateEmoticonToolTip(e, GetEmoticonRect(line, col));
		}
	}
}

void SingleListEmoticons::Load()
{
	selection = -1;

	emoticons.count = ssd->module->emoticons.getCount();

	GetMaxEmoticonSize();

	emoticons.cols = GetNumOfCols(emoticons.count);
	emoticons.lines = GetNumOfLines(emoticons.count, emoticons.cols);

	window.width = emoticons.max_width * emoticons.cols + (emoticons.cols + 1) * BORDER + 1;
	window.height = emoticons.max_height * emoticons.lines + (emoticons.lines + 1) * BORDER + 1;
}

void SingleListEmoticons::SetSelection(HWND hwnd, POINT p)
{
	int col;
	if (p.x % (BORDER + emoticons.max_width) < BORDER)
		col = -1;
	else
		col = p.x / (BORDER + emoticons.max_width);

	int line;
	if (p.y % (BORDER + emoticons.max_height) < BORDER)
		line = -1;
	else
		line = p.y / (BORDER + emoticons.max_height);

	int index = line * emoticons.cols + col;

	if (col >= 0 && line >= 0 && index < ssd->module->emoticons.getCount())
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
	else
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, -1);
	}
}

void SingleListEmoticons::OnUp(HWND hwnd)
{
	if (selection < 0)
	{
//		int index = GetEmoticonCountRoundedByCols() - 1;
//		if (index >= ssd->module->emoticons.getCount())
//			index -= emoticons.cols;
//
//		EmoticonsSelectionLayout::SetSelection(hwnd, index);
		EmoticonsSelectionLayout::SetSelection(hwnd, ssd->module->emoticons.getCount() - 1);
	}
	else
	{
		int roundCount = GetEmoticonCountRoundedByCols();

		int index = (selection - emoticons.cols) % roundCount;
		if (index < 0)
			index += roundCount - 1;
		if (index >= ssd->module->emoticons.getCount())
			index -= emoticons.cols;

		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
}

void SingleListEmoticons::OnDown(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, 0);
	}
	else
	{
		int roundCount = GetEmoticonCountRoundedByCols();

		int index = selection + emoticons.cols;
		if (index >= roundCount)
		{
			index = (index + 1) % roundCount;
			if (index >= ssd->module->emoticons.getCount())
				index = (index + emoticons.cols) % roundCount;
		}
		else if (index >= ssd->module->emoticons.getCount())
			index = (index + emoticons.cols + 1) % roundCount % emoticons.cols;

		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
}

void SingleListEmoticons::OnLeft(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, emoticons.cols - 1);
	}
	else
	{
		int index = (selection - 1) % ssd->module->emoticons.getCount();
		if (index < 0)
			index += ssd->module->emoticons.getCount();
		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
}

void SingleListEmoticons::OnRight(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, 0);
	}
	else
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, (selection + 1) % ssd->module->emoticons.getCount());
	}
}

RECT SingleListEmoticons::GetEmoticonRect(int line, int col)
{
	RECT rc;
	rc.left = BORDER + col * (emoticons.max_width + BORDER);
	rc.top = BORDER + line * (emoticons.max_height + BORDER);
	rc.right = rc.left + emoticons.max_width;
	rc.bottom = rc.top + emoticons.max_height;
	return rc;
}

void SingleListEmoticons::Draw(HDC hdc)
{
	EraseBackground(hdc);

	for(int line = 0; line < emoticons.lines; line++)
	{
		for(int col = 0; col < emoticons.cols; col++)
		{
			int index = line * emoticons.cols + col;
			if (index >= emoticons.count)
				break;

			Emoticon *e = ssd->module->emoticons[index];
			if (e->service[0] != NULL && !ProtoServiceExists(ssd->proto, e->service[0]))
				continue;

			DrawEmoticon(hdc, index, GetEmoticonRect(line, col));
		}
	}
}
