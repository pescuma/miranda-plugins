#include "globals.h"
#include "FontState_v8_wrapper.h"
#include <v8.h>
#include "FontState.h"
#include <utf8_helpers.h>

using namespace v8;


#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif


static Handle<Value> Get_FontState_face(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return String::New((const V8_TCHAR *) tmp->getFace());
}

static void Set_FontState_face(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	String::Utf8Value utf8_value(value);
	tmp->setFace(Utf8ToTchar(*utf8_value));
}


static Handle<Value> Get_FontState_size(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Int32::New(tmp->getSize());
}

static void Set_FontState_size(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setSize(value->Int32Value());
}


static Handle<Value> Get_FontState_italic(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Boolean::New(tmp->isItalic());
}

static void Set_FontState_italic(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setItalic(value->BooleanValue());
}


static Handle<Value> Get_FontState_bold(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Boolean::New(tmp->isBold());
}

static void Set_FontState_bold(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setBold(value->BooleanValue());
}


static Handle<Value> Get_FontState_underline(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Boolean::New(tmp->isUnderline());
}

static void Set_FontState_underline(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setUnderline(value->BooleanValue());
}


static Handle<Value> Get_FontState_strikeOut(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Boolean::New(tmp->isStrikeOut());
}

static void Set_FontState_strikeOut(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setStrikeOut(value->BooleanValue());
}


static Handle<Value> Get_FontState_color(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	return Int32::New(tmp->getColor());
}

static void Set_FontState_color(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	FontState *tmp = (FontState *) wrap->Value();
	tmp->setColor(value->Int32Value());
}


void AddFontStateAcessors(Handle<ObjectTemplate> &templ)
{
	templ->SetAccessor(String::New("face"), Get_FontState_face, Set_FontState_face);
	templ->SetAccessor(String::New("size"), Get_FontState_size, Set_FontState_size);
	templ->SetAccessor(String::New("italic"), Get_FontState_italic, Set_FontState_italic);
	templ->SetAccessor(String::New("bold"), Get_FontState_bold, Set_FontState_bold);
	templ->SetAccessor(String::New("underline"), Get_FontState_underline, Set_FontState_underline);
	templ->SetAccessor(String::New("strikeOut"), Get_FontState_strikeOut, Set_FontState_strikeOut);
	templ->SetAccessor(String::New("color"), Get_FontState_color, Set_FontState_color);
}
