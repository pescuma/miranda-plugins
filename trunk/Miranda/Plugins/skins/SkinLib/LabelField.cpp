#include "globals.h"
#include "LabelField.h"
#include "LabelFieldState.h"


LabelField::LabelField(const char *name, HWND hwnd) : ControlField(name, hwnd)
{
}

LabelField::~LabelField()
{
}

FieldType LabelField::getType() const
{
	return CONTROL_LABEL;
}

FieldState * LabelField::createState()
{
	return new LabelFieldState(this);
}