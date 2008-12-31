#ifndef __ICON_FIELD_STATE_H__
# define __ICON_FIELD_STATE_H__

#include "IconField.h"
#include "FieldState.h"


class IconFieldState : public FieldState
{
public:
	virtual ~IconFieldState();

	virtual IconField * getField() const;

	virtual Size getPreferedSize() const;

	virtual HICON getIcon() const;

private:
	IconFieldState(IconField *field);

	friend class IconField;
};


#endif // __ICON_FIELD_STATE_H__