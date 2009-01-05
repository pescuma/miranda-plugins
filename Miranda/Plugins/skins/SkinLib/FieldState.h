#ifndef __FIELD_STATE_H__
# define __FIELD_STATE_H__

#include "Field.h"

class DialogState;

enum HORIZONTAL_ALIGN
{
	LEFT = 1,
	RIGHT,
	CENTER
};


class FieldState
{
public:
	virtual ~FieldState();

	virtual Field * getField() const;
	virtual DialogState * getDialog() const;

	virtual Size getPreferedSize() const = 0;

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
	virtual void setBottom(int botom);

	virtual bool isVisible() const;
	virtual void setVisible(bool visible);

	virtual bool isEmpty() const = 0;

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

	friend class Field;
};


#endif  // __FIELD_STATE_H__
