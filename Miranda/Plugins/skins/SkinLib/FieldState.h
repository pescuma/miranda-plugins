#ifndef __FIELD_STATE_H__
# define __FIELD_STATE_H__

#include "Field.h"
#include "BorderState.h"

class DialogState;

enum HORIZONTAL_ALIGN
{
	LEFT = 1,
	CENTER,
	RIGHT
};


class FieldState
{
public:
	virtual ~FieldState();

	virtual Field * getField() const;
	virtual DialogState * getDialog() const;

	virtual Size getPreferedSize() const = 0;

	// Used inside script

	virtual int getX() const;
	virtual void setX(int x);

	virtual int getY() const;
	virtual void setY(int y);

	virtual int getWidth() const;
	virtual void setWidth(int width);

	virtual int getHeight() const;
	virtual void setHeight(int height);

	virtual int getLeft() const;
	virtual void setLeft(int left);

	virtual int getTop() const;
	virtual void setTop(int top);

	virtual int getRight() const;
	virtual void setRight(int right);

	virtual int getBottom() const;
	virtual void setBottom(int bottom);

	virtual bool isVisible() const;
	virtual void setVisible(bool visible);

	virtual bool isEnabled() const;

	virtual const TCHAR * getToolTip() const;
	virtual void setToolTip(const TCHAR *tooltip);

	virtual BorderState * getBorders();
	virtual const BorderState * getBorders() const;

	virtual bool isEmpty() const = 0;

	// Results

	virtual RECT getInsideRect() const;
	virtual RECT getRect() const;

protected:
	FieldState(DialogState *dialog, Field *field);

	Field *field;
	DialogState *dialog;

	Size size;
	Position pos;
	int usingX;
	int usingY;
	bool visible;
	BorderState borders;
	bool tooltipSet;
	std::tstring tooltip;

	int getHorizontalBorders() const;
	int getVerticalBorders() const;

	friend class Field;
};


#endif  // __FIELD_STATE_H__
