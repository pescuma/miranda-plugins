#ifndef __FIELD_STATE_H__
# define __FIELD_STATE_H__

#include "Field.h"


class FieldState
{
public:
	virtual ~FieldState();

	virtual Field * getField() const;

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

protected:
	FieldState(Field *field);

	Field *field;

	Size size;
	Position pos;
	int usingX;
	int usingY;
	bool visible;

	friend class Field;
};


#endif  // __FIELD_STATE_H__
