#include "globals.h"
#include "DialogState_v8_wrapper.h"
#include <v8.h>
#include "DialogState.h"

using namespace v8;


#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif


static Handle<Value> Get_DialogState_width(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	DialogState *tmp = (DialogState *) wrap->Value();
	return Int32::New(tmp->getWidth());
}

static void Set_DialogState_width(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	DialogState *tmp = (DialogState *) wrap->Value();
	tmp->setWidth(value->Int32Value());
}


static Handle<Value> Get_DialogState_height(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	DialogState *tmp = (DialogState *) wrap->Value();
	return Int32::New(tmp->getHeight());
}

static void Set_DialogState_height(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	DialogState *tmp = (DialogState *) wrap->Value();
	tmp->setHeight(value->Int32Value());
}


void AddDialogStateAcessors(Handle<ObjectTemplate> &templ)
{
	templ->SetAccessor(String::New("width"), Get_DialogState_width, Set_DialogState_width);
	templ->SetAccessor(String::New("height"), Get_DialogState_height, Set_DialogState_height);
}
