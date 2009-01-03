#include "globals.h"
#include "Field.h"
#include "FieldState.h"


Field::Field(Dialog *aDlg, const char *aName) 
		: dlg(aDlg), name(aName), onChangeCallback(NULL), onChangeCallbackParam(NULL)
{
}

Field::~Field()
{
}

Dialog * Field::getDialog() const
{
	return dlg;
}

const char * Field::getName() const
{
	return name.c_str();
}

void Field::setOnChangeCallback(FieldCallback cb, void *param /*= NULL*/)
{
	onChangeCallback = cb;
	onChangeCallbackParam = param;
}

void Field::fireOnChange() const
{
	if (onChangeCallback != NULL)
		onChangeCallback(onChangeCallbackParam, this);
}
