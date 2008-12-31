#ifndef __DIALOG_STATE_H__
# define __DIALOG_STATE_H__

#include "Dialog.h"
#include "FieldState.h"
#include "BorderState.h"


/// This have to be deleted before the owning dialog
/// It is responsible for freeing the FieldStates
class DialogState
{
public:
	~DialogState();

	Dialog * getDialog() const;

	std::vector<FieldState *> fields;
	FieldState * getField(const char *name) const;

	int getWidth() const;
	void setWidth(int width);

	int getHeight() const;
	void setHeight(int height);

	BorderState * getBorders();
	const BorderState * getBorders() const;

private:
	DialogState(Dialog *dialog);

	Dialog *dialog;

	Size size;
	BorderState borders;

	int getHorizontalBorders() const;
	int getVerticalBorders() const;

	friend class Dialog;
};


#endif // __DIALOG_STATE_H__