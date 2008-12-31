#include "globals.h"
#include "ButtonField.h"
#include "ButtonFieldState.h"


ButtonField::ButtonField(const char *name, HWND hwnd) : ControlField(name, hwnd)
{
}

ButtonField::~ButtonField()
{
}

FieldType ButtonField::getType() const
{
	return CONTROL_BUTTON;
}

FieldState * ButtonField::createState()
{
	return new ButtonFieldState(this);
}