#include "globals.h"
#include "TextFieldState_v8_wrapper.h"
#include <v8.h>
#include "TextFieldState.h"
#include <utf8_helpers.h>

using namespace v8;


#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif


static Handle<Value> Get_TextFieldState_text(Local<String> property, const AccessorInfo &info) 
{
	HandleScope scope;
	
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return scope.Close( Undefined() );

	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	if (tmp == NULL)
		return scope.Close( Undefined() );

	return scope.Close( String::New((const V8_TCHAR *) tmp->getText()) );
}

static void Set_TextFieldState_text(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	HandleScope scope;
	
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsString())
	{
		String::Utf8Value utf8_value(value);
		tmp->setText(Utf8ToTchar(*utf8_value));
	}
}


static Handle<Value> Get_TextFieldState_hAlign(Local<String> property, const AccessorInfo &info) 
{
	HandleScope scope;
	
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return scope.Close( Undefined() );

	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	if (tmp == NULL)
		return scope.Close( Undefined() );

	switch(tmp->getHAlign())
	{
		case LEFT: return scope.Close( String::New((const V8_TCHAR *) _T("LEFT")) );
		case CENTER: return scope.Close( String::New((const V8_TCHAR *) _T("CENTER")) );
		case RIGHT: return scope.Close( String::New((const V8_TCHAR *) _T("RIGHT")) );
	}
	return scope.Close( Undefined() );
}

static void Set_TextFieldState_hAlign(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	HandleScope scope;
	
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	TextFieldState *tmp = (TextFieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsString())
	{
		String::Utf8Value utf8_value(value);
		Utf8ToTchar tval(*utf8_value);
		if ( lstrcmpi(_T("LEFT"), tval) == 0)
			tmp->setHAlign(LEFT);
		else if ( lstrcmpi(_T("CENTER"), tval) == 0)
			tmp->setHAlign(CENTER);
		else if ( lstrcmpi(_T("RIGHT"), tval) == 0)
			tmp->setHAlign(RIGHT);
	}
}


void AddTextFieldStateAcessors(Handle<ObjectTemplate> &templ)
{
	HandleScope scope;
	
	templ->SetAccessor(String::New("text"), Get_TextFieldState_text, Set_TextFieldState_text);
	templ->SetAccessor(String::New("hAlign"), Get_TextFieldState_hAlign, Set_TextFieldState_hAlign);
}
