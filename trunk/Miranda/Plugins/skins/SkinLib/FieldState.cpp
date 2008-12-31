#include "globals.h"
#include "FieldState.h"

#define START		1<<0
#define LEN			1<<1
#define END			1<<2
#define USING_MASK	0xFF
#define LAST_SHIFT	8
#define SET(_FIELD_, _ITEM_)		_FIELD_ = (((_FIELD_ | _ITEM_) & USING_MASK) | (_ITEM_ << LAST_SHIFT))
#define LAST_SET(_FIELD_)			(_FIELD_ >> LAST_SHIFT)


FieldState::FieldState(Field *aField) : field(aField), size(-1, -1), pos(0, 0), 
										usingX(0), usingY(0), visible(true)
{
}

FieldState::~FieldState()
{
}

Field * FieldState::getField() const
{
	return field;
}

int FieldState::getX() const
{
	return pos.x;
}

void FieldState::setX(int x)
{
	if (usingX & END)
	{
		int diff = x - getX();
		size.x = max(0, getWidth() - diff);
	}

	pos.x = x;

	SET(usingX, START);
}

int FieldState::getY() const
{
	return pos.y;
}

void FieldState::setY(int y)
{
	if (usingY & END)
	{
		int diff = y - getY();
		size.y = max(0, getHeight() - diff);
	}

	pos.y = y;

	SET(usingY, START);
}

int FieldState::getWidth() const
{
	if (size.x >= 0)
		return size.x;

	return getPreferedSize().x;
}

void FieldState::setWidth(int width)
{
	width = max(0, width);

	if (LAST_SET(usingX) == END)
	{
		int diff = width - getWidth();
		pos.x = getX() - diff;
	}

	size.x = width;

	usingX |= LEN;
}

int FieldState::getHeight() const
{
	if (size.y >= 0)
		return size.y;

	return getPreferedSize().y;
}

void FieldState::setHeight(int height)
{
	height = max(0, height);

	if (LAST_SET(usingY) == END)
	{
		int diff = height - getHeight();
		pos.y = getY() - diff;
	}

	size.y = height;

	usingY |= LEN;
}

bool FieldState::isVisible() const
{
	return visible;
}

void FieldState::setVisible(bool visible)
{
	this->visible = visible;
}

int FieldState::getLeft() const
{
	return getX();
}

void FieldState::setLeft(int left)
{
	setX(left);
}

int FieldState::getTop() const
{
	return getY();
}

void FieldState::setTop(int top)
{
	setY(top);
}

int FieldState::getRight() const
{
	return getX() + getWidth();
}

void FieldState::setRight(int right)
{
	if (usingX & START)
	{
		size.x = max(0, right - getX());
	}
	else
	{
		pos.x = right - getWidth();
	}

	SET(usingX, END);
}

int FieldState::getBottom() const
{
	return getY() + getHeight();
}

void FieldState::setBottom(int botom)
{
	if (usingY & START)
	{
		size.y = max(0, botom - getY());
	}
	else
	{
		pos.y = botom - getHeight();
	}

	SET(usingY, END);
}