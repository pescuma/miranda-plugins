#ifndef __LABEL_FIELD_H__
# define __LABEL_FIELD_H__

#include "ControlField.h"


class LabelField : public ControlField
{
public:
	LabelField(const char *name, HWND hwnd);
	virtual ~LabelField();

	virtual FieldType getType() const;

	virtual FieldState * createState();
};



#endif // __LABEL_FIELD_H__
