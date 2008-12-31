#ifndef __BUTTON_FIELD_STATE_H__
# define __BUTTON_FIELD_STATE_H__

#include "ControlFieldState.h"


class ButtonFieldState : public ControlFieldState
{
public:
	virtual ~ButtonFieldState();

	virtual Size getPreferedSize() const;

private:
	ButtonFieldState(ControlField *field);

	friend class ButtonField;
};


#endif // __BUTTON_FIELD_STATE_H__
