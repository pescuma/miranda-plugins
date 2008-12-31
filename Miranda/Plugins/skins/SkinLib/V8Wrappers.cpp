#include "globals.h"
#include "V8Wrappers.h"

#include "FieldState_v8_wrapper.h"
#include "ControlFieldState_v8_wrapper.h"
#include "TextFieldState_v8_wrapper.h"
#include "FontState_v8_wrapper.h"
#include "DialogState_v8_wrapper.h"
#include "BorderState_v8_wrapper.h"
#include "SkinOption_v8_wrapper.h"
#include "utf8_helpers.h"


#define OPTION (USER_DEFINED-5)
#define OPTIONS (USER_DEFINED-4)
#define BORDER (USER_DEFINED-3)
#define FONT (USER_DEFINED-2)
#define DIALOG (USER_DEFINED-1)


using namespace v8;


void V8Wrappers::createTemplateFor(FieldType type)
{
	switch(type)
	{
		case SIMPLE_TEXT:
			createTextTemplate();
			break;
		case SIMPLE_IMAGE:
			createImageTemplate();
			break;
		case SIMPLE_ICON:
			createIconTemplate();
			break;
		case CONTROL_LABEL:
			createLabelTemplate();
			break;
		case CONTROL_BUTTON:
			createButtonTemplate();
			break;
		case CONTROL_EDIT:
			createEditTemplate();
			break;
	}
}


static Handle<Value> Get_DialogState_borders(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	return self->GetInternalField(1);
}


static void Set_DialogState_borders(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	DialogState *tmp = (DialogState *) wrap->Value();
	tmp->getBorders()->setAll(value->Int32Value());
}


void V8Wrappers::createDialogTemplate()
{
	if (templs.find(DIALOG) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(2);
	AddDialogStateAcessors(templ);
	templ->SetAccessor(String::New("borders"), Get_DialogState_borders, Set_DialogState_borders);
	templs[DIALOG] = templ;

	createBorderTemplate();
}


void V8Wrappers::createTextTemplate()
{	
	if (templs.find(SIMPLE_TEXT) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	AddTextFieldStateAcessors(templ);
	templs[SIMPLE_TEXT] = templ;

	createFontTemplate();
}

void V8Wrappers::createImageTemplate()
{
	if (templs.find(SIMPLE_IMAGE) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	templs[SIMPLE_IMAGE] = templ;
}

void V8Wrappers::createIconTemplate()
{
	if (templs.find(SIMPLE_ICON) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	templs[SIMPLE_ICON] = templ;
}

void V8Wrappers::createLabelTemplate()
{
	if (templs.find(CONTROL_LABEL) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	AddControlFieldStateAcessors(templ);
	templs[CONTROL_LABEL] = templ;

	createFontTemplate();
}

void V8Wrappers::createButtonTemplate()
{
	if (templs.find(CONTROL_BUTTON) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	AddControlFieldStateAcessors(templ);
	templs[CONTROL_BUTTON] = templ;

	createFontTemplate();
}

void V8Wrappers::createEditTemplate()
{
	if (templs.find(CONTROL_EDIT) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFieldStateAcessors(templ);
	AddControlFieldStateAcessors(templ);
	templs[CONTROL_EDIT] = templ;

	createFontTemplate();
}

void V8Wrappers::createFontTemplate()
{
	if (templs.find(FONT) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddFontStateAcessors(templ);
	templs[FONT] = templ;
}


void V8Wrappers::createBorderTemplate()
{
	if (templs.find(BORDER) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddBorderStateAcessors(templ);
	templs[BORDER] = templ;
}


Handle<Object> V8Wrappers::createWrapper(FieldType type)
{
	createTemplateFor(type);

	switch(type)
	{
		case SIMPLE_TEXT:
			return createTextWrapper();
			break;
		case SIMPLE_IMAGE:
			return createImageWrapper();
			break;
		case SIMPLE_ICON:
			return createIconWrapper();
			break;
		case CONTROL_LABEL:
			return createLabelWrapper();
			break;
		case CONTROL_BUTTON:
			return createButtonWrapper();
			break;
		case CONTROL_EDIT:
			return createEditWrapper();
			break;
	}
	throw "Unknown type";
}

Handle<Object> V8Wrappers::fillWrapper(Handle<Object> obj, FieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	switch(state->getField()->getType())
	{
	case SIMPLE_TEXT:
		return fillTextWrapper(obj, (TextFieldState *) state);
		break;
	case SIMPLE_IMAGE:
		return fillImageWrapper(obj, (ImageFieldState *) state);
		break;
	case SIMPLE_ICON:
		return fillIconWrapper(obj, (IconFieldState *) state);
		break;
	case CONTROL_LABEL:
		return fillLabelWrapper(obj, (LabelFieldState *) state);
		break;
	case CONTROL_BUTTON:
		return fillButtonWrapper(obj, (ButtonFieldState *) state);
		break;
	case CONTROL_EDIT:
		return fillEditWrapper(obj, (EditFieldState *) state);
		break;
	}
	throw "Unknown type";
}

Handle<Object> V8Wrappers::createDialogWrapper()
{
	createDialogTemplate();

	Handle<Object> obj = newInstance(DIALOG);
	Handle<Object> borders = newInstance(BORDER);
	obj->SetInternalField(1, borders);

	return obj;
}

Handle<Object> V8Wrappers::fillWrapper(Handle<Object> obj, DialogState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	Handle<Object> borders = Handle<Object>::Cast(obj->GetInternalField(1));
	borders->SetInternalField(0, External::New(state->getBorders()));

	return obj;
}

Handle<Object> V8Wrappers::newInstance(int type)
{
	if (templs.find(type) == templs.end())
		throw "Unknown template";

	return templs[type]->NewInstance();
}

Handle<Object> V8Wrappers::createTextWrapper()
{
	Handle<Object> obj = newInstance(SIMPLE_TEXT);

	Handle<Object> font = newInstance(FONT);
	obj->Set(String::New("font"), font, ReadOnly);

	return obj;
}

Handle<Object> V8Wrappers::fillTextWrapper(Handle<Object> obj, TextFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	Handle<Object> font = Handle<Object>::Cast(obj->Get(String::New("font")));
	font->SetInternalField(0, External::New(state->getFont()));

	return obj;
}

Handle<Object> V8Wrappers::createIconWrapper()
{
	return newInstance(SIMPLE_ICON);
}

Handle<Object> V8Wrappers::fillIconWrapper(Handle<Object> obj, IconFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	return obj;
}

Handle<Object> V8Wrappers::createImageWrapper()
{
	return newInstance(SIMPLE_IMAGE);
}

Handle<Object> V8Wrappers::fillImageWrapper(Handle<Object> obj, ImageFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	return obj;
}

Handle<Object> V8Wrappers::createLabelWrapper()
{
	Handle<Object> obj = newInstance(CONTROL_LABEL);

	Handle<Object> font = newInstance(FONT);
	obj->Set(String::New("font"), font, ReadOnly);

	return obj;
}

Handle<Object> V8Wrappers::fillLabelWrapper(Handle<Object> obj, LabelFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	Handle<Object> font = Handle<Object>::Cast(obj->Get(String::New("font")));
	font->SetInternalField(0, External::New(state->getFont()));

	return obj;
}

Handle<Object> V8Wrappers::createButtonWrapper()
{
	Handle<Object> obj = newInstance(CONTROL_BUTTON);

	Handle<Object> font = newInstance(FONT);
	obj->Set(String::New("font"), font, ReadOnly);

	return obj;
}

Handle<Object> V8Wrappers::fillButtonWrapper(Handle<Object> obj, ButtonFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	Handle<Object> font = Handle<Object>::Cast(obj->Get(String::New("font")));
	font->SetInternalField(0, External::New(state->getFont()));

	return obj;
}

Handle<Object> V8Wrappers::createEditWrapper()
{
	Handle<Object> obj = newInstance(CONTROL_EDIT);

	Handle<Object> font = newInstance(FONT);
	obj->Set(String::New("font"), font, ReadOnly);

	return obj;
}

Handle<Object> V8Wrappers::fillEditWrapper(Handle<Object> obj, EditFieldState *state)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(state));

	Handle<Object> font = Handle<Object>::Cast(obj->Get(String::New("font")));
	font->SetInternalField(0, External::New(state->getFont()));

	return obj;
}

void V8Wrappers::clearTemplates()
{
	templs.clear();
}


static Handle<Value> Get_Options_Fields(Local<String> aName, const AccessorInfo &info)
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	SkinOptions *opts = (SkinOptions *) wrap->Value();
	if (opts == NULL)
		return Undefined();

	String::AsciiValue name(aName);
	if (name.length() <= 0)
		return Undefined();

	bool configure = self->GetInternalField(1)->BooleanValue();
	if (configure)
	{
		SkinOption * opt = opts->getOption(*name);

		if (opt == NULL)
		{
			opt = new SkinOption(*name);
			opts->addOption(opt);
		}

		V8Wrappers wrappers;
		return wrappers.fillWrapper(wrappers.createOptionWrapper(), opt);
	}
	else
	{
		SkinOption * opt = opts->getOption(*name);
		if (opt == NULL)
			return Undefined();

		switch (opt->getType())
		{
			case CHECKBOX:	return Boolean::New(opt->getValueCheckbox());
			case NUMBER:	return Int32::New(opt->getValueNumber());
			case TEXT:		return String::New((const V8_TCHAR *) opt->getValueText());
		}

		return Undefined();
	}
}

void V8Wrappers::createOptionsTemplate()
{
	if (templs.find(OPTIONS) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(2);
	templ->SetNamedPropertyHandler(&Get_Options_Fields);
	templs[OPTIONS] = templ;
}

Handle<Object> V8Wrappers::createOptionsWrapper()
{
	createOptionsTemplate();

	return newInstance(OPTIONS);
}

static Handle<Value> Get_SkinOption_value(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	SkinOption *opt = (SkinOption *) wrap->Value();
	switch (opt->getType())
	{
		case CHECKBOX:	return Boolean::New(opt->getValueCheckbox());
		case NUMBER:	return Int32::New(opt->getValueNumber());
		case TEXT:		return String::New((const V8_TCHAR *) opt->getValueText());
	}
	return Undefined();
}

static void Set_SkinOption_value(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	SkinOption *opt = (SkinOption *) wrap->Value();
	switch (opt->getType())
	{
		case CHECKBOX:	
			opt->setValueCheckbox(value->BooleanValue());
			break;
		case NUMBER:
			opt->setValueNumber(value->Int32Value());
			break;
		case TEXT:		
			opt->setValueText(Utf8ToTchar(*String::Utf8Value(value)));
			break;
	}
}

void V8Wrappers::createOptionTemplate()
{
	if (templs.find(OPTION) != templs.end())
		return;

	Handle<ObjectTemplate> templ = ObjectTemplate::New();
	templ->SetInternalFieldCount(1);
	AddSkinOptionAcessors(templ);
	templ->SetAccessor(String::New("value"), Get_SkinOption_value, Set_SkinOption_value);
	templs[OPTION] = templ;
}

Handle<Object> V8Wrappers::createOptionWrapper()
{
	createOptionTemplate();

	return newInstance(OPTION);
}

Handle<Object> V8Wrappers::fillWrapper(Handle<Object> obj, SkinOptions *opts, bool configure)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(opts));
	obj->SetInternalField(1, Boolean::New(configure));

	return obj;
}

Handle<Object> V8Wrappers::fillWrapper(Handle<Object> obj, SkinOption *opt)
{
	if (obj.IsEmpty())
		throw "Empty object";

	obj->SetInternalField(0, External::New(opt));

	return obj;
}