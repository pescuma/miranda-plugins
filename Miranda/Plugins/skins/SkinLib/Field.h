#ifndef __FIELD_H__
# define __FIELD_H__

#include <windows.h>
#include <tchar.h>
#include "tstring.h"
#include "Size.h"
#include "Position.h"



enum FieldType
{
	SIMPLE_TEXT = 1,
	SIMPLE_IMAGE,
	SIMPLE_ICON,
	CONTROL_LABEL,
	CONTROL_BUTTON,
	CONTROL_EDIT,
	USER_DEFINED = 0x100
};

class Field;
class FieldState;

typedef void (*FieldCallback)(void *param, const Field *field);


class Field
{
public:
	Field(const char *name);
	virtual ~Field();

	virtual const char * getName() const;
	virtual FieldType getType() const = 0;

	virtual FieldState * createState() = 0;

	virtual void setOnChangeCallback(FieldCallback cb, void *param = NULL);

protected:
	void fireOnChange() const;

private:
	const std::string name;

	FieldCallback onChangeCallback;
	void *onChangeCallbackParam;
};




#endif // __FIELD_H__