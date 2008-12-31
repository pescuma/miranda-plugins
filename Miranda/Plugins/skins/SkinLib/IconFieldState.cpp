#include "globals.h"
#include "IconFieldState.h"

#define ICON_SIZE 16


IconFieldState::IconFieldState(IconField *field) : FieldState(field)
{
}

IconFieldState::~IconFieldState()
{
}

IconField * IconFieldState::getField() const
{
	return (IconField *) FieldState::getField();
}

Size IconFieldState::getPreferedSize() const
{
	if (getIcon() == NULL)
		return Size(0, 0);

	return Size(ICON_SIZE, ICON_SIZE);
}

HICON IconFieldState::getIcon() const
{
	return getField()->getIcon();
}
