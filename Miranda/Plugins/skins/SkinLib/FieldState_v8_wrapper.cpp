#include "globals.h"
#include "FieldState_v8_wrapper.h"
#include <v8.h>
#include "FieldState.h"

using namespace v8;


#ifdef UNICODE
# define V8_TCHAR uint16_t
#else
# define V8_TCHAR char
#endif


static Handle<Value> Get_FieldState_x(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getX());
}

static void Set_FieldState_x(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setX(value->Int32Value());
}


static Handle<Value> Get_FieldState_y(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getY());
}

static void Set_FieldState_y(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setY(value->Int32Value());
}


static Handle<Value> Get_FieldState_width(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getWidth());
}

static void Set_FieldState_width(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setWidth(value->Int32Value());
}


static Handle<Value> Get_FieldState_height(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getHeight());
}

static void Set_FieldState_height(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setHeight(value->Int32Value());
}


static Handle<Value> Get_FieldState_left(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getLeft());
}

static void Set_FieldState_left(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setLeft(value->Int32Value());
}


static Handle<Value> Get_FieldState_top(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getTop());
}

static void Set_FieldState_top(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setTop(value->Int32Value());
}


static Handle<Value> Get_FieldState_right(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getRight());
}

static void Set_FieldState_right(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setRight(value->Int32Value());
}


static Handle<Value> Get_FieldState_bottom(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Int32::New(tmp->getBottom());
}

static void Set_FieldState_bottom(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsInt32())
		tmp->setBottom(value->Int32Value());
}


static Handle<Value> Get_FieldState_visible(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Boolean::New(tmp->isVisible());
}

static void Set_FieldState_visible(Local<String> property, Local<Value> value, const AccessorInfo& info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return;

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return;

	if (!value.IsEmpty() && value->IsBoolean())
		tmp->setVisible(value->BooleanValue());
}


static Handle<Value> Get_FieldState_enabled(Local<String> property, const AccessorInfo &info) 
{
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	if (wrap.IsEmpty())
		return Undefined();

	FieldState *tmp = (FieldState *) wrap->Value();
	if (tmp == NULL)
		return Undefined();

	return Boolean::New(tmp->isEnabled());
}


void AddFieldStateAcessors(Handle<ObjectTemplate> &templ)
{
	templ->SetAccessor(String::New("x"), Get_FieldState_x, Set_FieldState_x);
	templ->SetAccessor(String::New("y"), Get_FieldState_y, Set_FieldState_y);
	templ->SetAccessor(String::New("width"), Get_FieldState_width, Set_FieldState_width);
	templ->SetAccessor(String::New("height"), Get_FieldState_height, Set_FieldState_height);
	templ->SetAccessor(String::New("left"), Get_FieldState_left, Set_FieldState_left);
	templ->SetAccessor(String::New("top"), Get_FieldState_top, Set_FieldState_top);
	templ->SetAccessor(String::New("right"), Get_FieldState_right, Set_FieldState_right);
	templ->SetAccessor(String::New("bottom"), Get_FieldState_bottom, Set_FieldState_bottom);
	templ->SetAccessor(String::New("visible"), Get_FieldState_visible, Set_FieldState_visible);
	templ->SetAccessor(String::New("enabled"), Get_FieldState_enabled, NULL, Handle<Value>(), DEFAULT, ReadOnly);
}
