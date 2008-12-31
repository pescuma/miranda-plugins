#include "globals.h"
#include "BorderState.h"

BorderState::BorderState(int aLeft, int aRight, int aTop, int aBottom) : left(aLeft), right(aRight), 
																			top(aTop), bottom(aBottom)
{
}

BorderState::~BorderState()
{
}

int BorderState::getLeft() const
{
	return left;
}

void BorderState::setLeft(int left)
{
	this->left = left;
}

int BorderState::getRight() const
{
	return right;
}

void BorderState::setRight(int right)
{
	this->right = right;
}

int BorderState::getTop() const
{
	return top;
}

void BorderState::setTop(int top)
{
	this->top = top;
}

int BorderState::getBottom() const
{
	return bottom;
}

void BorderState::setBottom(int bottom)
{
	this->bottom = bottom;
}

void BorderState::setAll(int border)
{
	left = border;
	right = border; 
	top = border;
	bottom = border;
}