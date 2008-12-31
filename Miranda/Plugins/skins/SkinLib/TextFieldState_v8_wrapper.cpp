#include "globals.h"
#include "TextFieldState_v8_wrapper.h"
#include <v8.h>
#include "TextFieldState.h"
#include "utf8_helpers.h"

using namespace v8;


#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif


static Handle<Value> Get_TextFieldState_text(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	return String::New((const V8_TCHAR *) tmp->getText());
}

static void Set_TextFieldState_text(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	String::Utf8Value utf8_value(value);
	tmp->setText(Utf8ToTchar(*utf8_value));
}


void AddTextFieldStateAcessors(Handle<ObjectTemplate> &templ)
{
	templ->SetAccessor(String::New("text"), Get_TextFieldState_text, Set_TextFieldState_text);
}
