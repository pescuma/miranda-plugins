#include "commons.h"


GroupListEmoticons::GroupListEmoticons(HWND hwnd, EmoticonSelectionData *ssd)
{
	this->hwnd = hwnd;
	this->ssd = ssd;
	Load();
}

GroupListEmoticons::~GroupListEmoticons()
{
	DeleteObject(groupFont);
	DeleteObject(emoticonFont);
}


HFONT mir_font_get(TCHAR *group, TCHAR *name, COLORREF *color)
{
	LOGFONT lf = {0};
	FontIDT font = {0};
	font.cbSize = sizeof(font);
	lstrcpyn(font.group, group, MAX_REGS(font.group));
	lstrcpyn(font.name, name, MAX_REGS(font.name));
	COLORREF tmp = (COLORREF) CallService(MS_FONT_GETT, (WPARAM) &font, (LPARAM) &lf);
	if (color != NULL)
		*color = tmp;
	return CreateFontIndirect(&lf);
}

COLORREF mir_color_get(TCHAR *group, TCHAR *name)
{
	ColourIDT cid = {0};
	cid.cbSize = sizeof(cid);
	lstrcpyn(cid.group, group, MAX_REGS(cid.group));
	lstrcpyn(cid.name, name, MAX_REGS(cid.name));
	return (COLORREF) CallService(MS_COLOUR_GETT, (WPARAM) &cid, 0);
}

HFONT GroupListEmoticons::GetFont(HDC hdc)
{
	return emoticonFont;
}

void GroupListEmoticons::ReleaseFont(HFONT hFont)
{
}


RECT GroupListEmoticons::CalcRect(TCHAR *txt, HFONT hFont)
{
	HDC hdc = GetDC(hwnd);
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	RECT rc = { 0, 0, 0xFFFF, 0xFFFF };
	DrawText(hdc, txt, lstrlen(txt), &rc, DT_CALCRECT | DT_NOPREFIX);

	SelectObject(hdc, oldFont);

	ReleaseDC(hwnd, hdc);
	return rc;
}


void GroupListEmoticons::DrawEmoticonText(HDC hdc, TCHAR *txt, RECT rc, HFONT hFont)
{
	HFONT oldFont = (HFONT) SelectObject(hdc, hFont);

	DrawText(hdc, txt, lstrlen(txt), &rc, DT_NOPREFIX);

	SelectObject(hdc, oldFont);
}


void GroupListEmoticons::Load()
{
	selection = -1;

	// Load fonts
	groupFont = mir_font_get(_T("Emoticons"), _T("Group Name"), &groupColor);
	emoticonFont = mir_font_get(_T("Emoticons"), _T("Emoticons Text"), &emoticonColor);
	groupBkgColor = mir_color_get(_T("Emoticons"), _T("Group Background"));
	ssd->background = mir_color_get(_T("Emoticons"), _T("Emoticons Background"));

	num_groups = CountGroups();
	groups = (Group *) malloc(num_groups * sizeof(Group));

	char *current_group = ssd->module->emoticons[0]->group;

	int current_id = -1;
	int i;
	for(i = 0; i < ssd->module->emoticons.getCount(); i++)
	{
		Emoticon *e = ssd->module->emoticons[i];

		if (stricmp(e->group, current_group) != 0 || i == 0)
		{
			if (i != 0)
				groups[current_id].end = i - 1;

			current_group = e->group;
			current_id++;

			SetGroupName(groups[current_id], current_group);
			groups[current_id].start = i;
		}
	}
	groups[current_id].end = i - 1;

	// First calc the width
	window.width = 0;
	for(i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		group.count = group.end - group.start + 1;
		GetMaxEmoticonSize(group);
		group.cols = GetNumOfCols(group.count);

		if (group.name[0] != _T('\0'))
		{
			RECT rc = CalcRect(group.name, groupFont);
			window.width = max(window.width, rc.right - rc.left + 2 * BORDER + 1);
		}

		window.width = max(window.width, group.max_width * group.cols + (group.cols + 1) * BORDER);
	}

	// Now calc the height
	window.height = 0;
	for(i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		group.top = window.height;

		// Recalc the num of cols
		int w = window.width - BORDER;
		group.cols = w / (group.max_width + BORDER);

		// If there is space left, put it into the emoticons
		group.max_width += (w - group.cols * (group.max_width + BORDER)) / group.cols;

		group.lines = GetNumOfLines(group.count, group.cols);

		if (group.name[0] != '\0')
		{
			RECT rc = CalcRect(group.name, groupFont);
			window.height += HeightWithBorders(rc);
		}

		window.height += group.max_height * group.lines + (group.lines + 1) * BORDER;
	}
}


int GroupListEmoticons::CountGroups()
{
	int groups = 1;
	char *current_group = ssd->module->emoticons[0]->group;
	for(int i = 1; i < ssd->module->emoticons.getCount(); i++)
	{
		Emoticon *e = ssd->module->emoticons[i];

		if (stricmp(e->group, current_group) != 0)
		{
			current_group = e->group;
			groups++;
		}
	}
	return groups;
}


void GroupListEmoticons::SetGroupName(Group &group, char *name)
{
	TCHAR *tmp = mir_a2t(name);
	group.name = mir_tstrdup(TranslateTS(tmp));
	mir_free(tmp);
}


void GroupListEmoticons::GetMaxEmoticonSize(Group &group)
{
	group.max_height = 4;
	group.max_width = 4;

	for(int i = group.start; i <= group.end; i++)
	{
		Emoticon *e = ssd->module->emoticons[i];

		int height, width;
		GetEmoticonSize(e, width, height);

		group.max_height = max(group.max_height, height);
		group.max_width = max(group.max_width, width);
	}
}


RECT GroupListEmoticons::GetEmoticonRect(Group &group, int index)
{
	int pos = index - group.start;
	int line = pos / group.cols;
	int col = pos % group.cols;

	RECT rc;
	rc.left = BORDER + col * (group.max_width + BORDER);
	rc.top = BORDER + line * (group.max_height + BORDER);
	rc.right = rc.left + group.max_width;
	rc.bottom = rc.top + group.max_height;
	return rc;
}


void GroupListEmoticons::CreateToolTips()
{
	int top = 0;

	for(int i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		
		if (group.name[0] != '\0')
		{
			RECT rc = CalcRect(group.name, groupFont);
			top += HeightWithBorders(rc);
		}

		for (int j = group.start; j <= group.end; j++)
		{
			Emoticon *e = ssd->module->emoticons[j];

			RECT rc = GetEmoticonRect(group, j);
			rc.top += top;
			rc.bottom += top;
			CreateEmoticonToolTip(e, rc);
		}

		top += group.max_height * group.lines + (group.lines + 1) * BORDER;
	}
}

int GroupListEmoticons::HeightWithBorders(RECT rc)
{
	return rc.bottom - rc.top + 2 * BORDER + 1;
}

int GroupListEmoticons::GetIndex(Group &group, int line, int col)
{
	return line * group.cols + col + group.start;
}

void GroupListEmoticons::SetSelection(HWND hwnd, POINT p)
{
	// first find group 
	int i;
	for(i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		if (group.top > p.y)
			break;
	}
	i--;

	Group &group = groups[i];

	p.y -= group.top;
	if (group.name[0] != '\0')
	{
		RECT rc = CalcRect(group.name, groupFont);
		p.y -= HeightWithBorders(rc);
	}

	int col;
	if (p.x % (BORDER + group.max_width) < BORDER)
		col = -1;
	else
		col = p.x / (BORDER + group.max_width);

	int line;
	if (p.y % (BORDER + group.max_height) < BORDER)
		line = -1;
	else
		line = p.y / (BORDER + group.max_height);

	int index = GetIndex(group, line, col);

	if (col >= 0 && line >= 0 && index <= group.end)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
	else
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, -1);
	}
}

int GroupListEmoticons::GetGroupByEmoticonIndex(int index)
{
	for(int i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		if (index >= group.start && index <= group.end)
			return i;
	}
	return 0;
}

void GroupListEmoticons::OnUp(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, ssd->module->emoticons.getCount() - 1);
	}
	else
	{
		int group_index = GetGroupByEmoticonIndex(selection);
		Group *group = &groups[group_index];

		int line = (selection - group->start) / group->cols;
		int col = (selection - group->start) % group->cols;

		line--;
		int index = GetIndex(*group, line, col);

		if (index >= group->start && index <= group->end)
		{
			EmoticonsSelectionLayout::SetSelection(hwnd, index);
		}
		else
		{
			group_index--;
			if (group_index < 0)
			{
				group_index = num_groups - 1;
				col--;
			}
			group = &groups[group_index];

			line = group->lines - 1;
			if (group_index == num_groups - 1 && col < 0)
				col = group->cols - 1;
			else
				col = max(0, min(col, group->cols - 1));

			index = GetIndex(*group, line, col);
			if (index < group->start || index > group->end)
				index = GetIndex(*group, line-1, col);

			EmoticonsSelectionLayout::SetSelection(hwnd, index);
		}
	}
}

void GroupListEmoticons::OnDown(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, 0);
	}
	else
	{
		int group_index = GetGroupByEmoticonIndex(selection);
		Group *group = &groups[group_index];

		int line = (selection - group->start) / group->cols;
		int col = (selection - group->start) % group->cols;

		line++;
		int index = GetIndex(*group, line, col);

		if (index >= group->start && index <= group->end)
		{
			EmoticonsSelectionLayout::SetSelection(hwnd, index);
		}
		else
		{
			group_index++;
			if (group_index >= num_groups)
			{
				group_index = 0;
				col++;
			}
			group = &groups[group_index];

			line = 0;
			if (group_index != 0)
				col = min(col, group->cols - 1);
			else if (col >= group->cols)
				col = 0;

			index = GetIndex(*group, line, col);
			EmoticonsSelectionLayout::SetSelection(hwnd, index);
		}
	}
}

void GroupListEmoticons::OnLeft(HWND hwnd)
{
	if (selection < 0)
	{
		EmoticonsSelectionLayout::SetSelection(hwnd, ssd->module->emoticons.getCount() - 1);
	}
	else
	{
		int index = (selection - 1) % ssd->module->emoticons.getCount();
		if (index < 0)
			index += ssd->module->emoticons.getCount();
		EmoticonsSelectionLayout::SetSelection(hwnd, index);
	}
}

void GroupListEmoticons::OnRight(HWND hwnd)
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

void GroupListEmoticons::Draw(HDC hdc)
{
	EraseBackground(hdc);

	RECT client_rc;
	GetClientRect(hwnd, &client_rc);

	COLORREF old_text_color = SetTextColor(hdc, emoticonColor);

	int top = 0;

	for(int i = 0; i < num_groups; i++)
	{
		Group &group = groups[i];
		
		if (group.name[0] != '\0')
		{
			RECT rc = CalcRect(group.name, groupFont);
			rc.top += top + BORDER;
			rc.bottom += top + BORDER;

			int width = rc.right - rc.left;
			rc.left = client_rc.left + (client_rc.right - client_rc.left - width) / 2;
			rc.right = rc.left + width;

			RECT title_rc = client_rc;
			title_rc.top = rc.top - BORDER;
			title_rc.bottom = rc.bottom + BORDER + 1;
			HBRUSH hB = CreateSolidBrush(groupBkgColor);
			FillRect(hdc, &title_rc, hB);
			DeleteObject(hB);

			SetTextColor(hdc, groupColor);

			DrawEmoticonText(hdc, group.name, rc, groupFont);

			SetTextColor(hdc, emoticonColor);

			top += HeightWithBorders(rc);
		}

		for (int j = group.start; j <= group.end; j++)
		{
			RECT rc = GetEmoticonRect(group, j);
			rc.top += top;
			rc.bottom += top;

			DrawEmoticon(hdc, j, rc);
		}

		top += group.max_height * group.lines + (group.lines + 1) * BORDER;
	}

	SetTextColor(hdc, old_text_color);
}
