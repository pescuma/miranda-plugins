#include "globals.h"
#include "DialogState.h"


DialogState::DialogState(Dialog *aDialog) : dialog(aDialog), size(-1,-1), borders(0,0,0,0)
{
}

DialogState::~DialogState()
{
	for(unsigned int i = 0; i < fields.size(); i++) 
		delete fields[i];

	fields.clear();
}

Dialog * DialogState::getDialog() const
{
	return dialog;
}

FieldState * DialogState::getField(const char *name) const
{
	if (name == NULL || name[0] == 0)
		return NULL;

	for(unsigned int i = 0; i < fields.size(); i++) 
	{
		FieldState *field = fields[i];
		if (strcmp(name, field->getField()->getName()) == 0)
			return field;
	}

	return NULL;
}

int DialogState::getWidth() const
{
	if (size.x >= 0)
		return size.x - getHorizontalBorders();

	return dialog->getSize().x - getHorizontalBorders();
}

void DialogState::setWidth(int width)
{
	size.x = max(0, width) + getHorizontalBorders();
}

int DialogState::getHeight() const
{
	if (size.y >= 0)
		return size.y - getVerticalBorders();

	return dialog->getSize().y - getVerticalBorders();
}

void DialogState::setHeight(int height)
{
	size.y = max(0, height) + getVerticalBorders();
}

BorderState * DialogState::getBorders()
{
	return &borders;
}

const BorderState * DialogState::getBorders() const
{
	return &borders;
}

int DialogState::getHorizontalBorders() const
{
	return borders.getLeft() + borders.getRight();
}

int DialogState::getVerticalBorders() const
{
	return borders.getTop() + borders.getBottom();
}