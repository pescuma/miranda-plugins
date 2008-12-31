#ifndef __DIALOG_H__
# define __DIALOG_H__

#include <vector>
#include "Field.h"

class DialogState;


/// It is responsible for freeing the Fields
class Dialog
{
public:
	Dialog(const char *name);
	~Dialog();

	const char * getName() const;

	std::vector<Field *> fields;
	bool addField(Field *field);
	Field * getField(const char *name) const;

	const Size & getSize() const;
	void setSize(const Size &size);

	DialogState * createState();

private:
	const std::string name;
	Size size;
};


#endif // __DIALOG_H__
