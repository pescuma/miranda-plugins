#ifndef __V8_WRAPPERS_H__
# define __V8_WRAPPERS_H__

#include <v8.h>
#include <map>
#include "DialogState.h"
#include "TextFieldState.h"
#include "ImageFieldState.h"
#include "IconFieldState.h"
#include "LabelFieldState.h"
#include "ButtonFieldState.h"
#include "EditFieldState.h"
#include "SkinOptions.h"

#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif

class V8Wrappers
{
public:
	v8::Handle<v8::Object> createDialogWrapper();
	v8::Handle<v8::Object> createOptionsWrapper();
	v8::Handle<v8::Object> createOptionWrapper();
	v8::Handle<v8::Object> createWrapper(FieldType type);

	v8::Handle<v8::Object> fillWrapper(v8::Handle<v8::Object> obj, FieldState *state);
	v8::Handle<v8::Object> fillWrapper(v8::Handle<v8::Object> obj, DialogState *state);
	v8::Handle<v8::Object> fillWrapper(v8::Handle<v8::Object> obj, SkinOptions *opts, bool configure);
	v8::Handle<v8::Object> fillWrapper(v8::Handle<v8::Object> obj, SkinOption *opt);

	void clearTemplates();

private:
	std::map<int, v8::Handle<v8::ObjectTemplate>> templs;

	void createDialogTemplate();
	void createOptionsTemplate();
	void createOptionTemplate();
	void createTemplateFor(FieldType type);

	void createTextTemplate();
	void createImageTemplate();
	void createIconTemplate();
	void createLabelTemplate();
	void createButtonTemplate();
	void createEditTemplate();
	void createFontTemplate();
	void createBorderTemplate();

	v8::Handle<v8::Object> createTextWrapper();
	v8::Handle<v8::Object> createImageWrapper();
	v8::Handle<v8::Object> createIconWrapper();
	v8::Handle<v8::Object> createLabelWrapper();
	v8::Handle<v8::Object> createButtonWrapper();
	v8::Handle<v8::Object> createEditWrapper();

	v8::Handle<v8::Object> fillTextWrapper(v8::Handle<v8::Object> obj, TextFieldState *state);
	v8::Handle<v8::Object> fillImageWrapper(v8::Handle<v8::Object> obj, ImageFieldState *state);
	v8::Handle<v8::Object> fillIconWrapper(v8::Handle<v8::Object> obj, IconFieldState *state);
	v8::Handle<v8::Object> fillLabelWrapper(v8::Handle<v8::Object> obj, LabelFieldState *state);
	v8::Handle<v8::Object> fillButtonWrapper(v8::Handle<v8::Object> obj, ButtonFieldState *state);
	v8::Handle<v8::Object> fillEditWrapper(v8::Handle<v8::Object> obj, EditFieldState *state);

	v8::Handle<v8::Object> newInstance(int type);

};



#endif // __V8_WRAPPERS_H__
