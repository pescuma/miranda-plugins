#ifndef __V8_WRAPPERS_H__
# define __V8_WRAPPERS_H__

#include "V8Templates.h"
#include "SkinOptions.h"


class V8Wrappers : public V8Templates
{
public:
	virtual v8::Handle<v8::Object> newState(FieldType type);
	virtual void fillState(v8::Handle<v8::Object> obj, FieldState *state);

	virtual v8::Handle<v8::ObjectTemplate> getOptionsTemplate();
	virtual v8::Handle<v8::Object> newOptions();
	virtual void fillOptions(v8::Handle<v8::Object> v8Obj, SkinOptions *obj, bool configure);

private:
	v8::Persistent<v8::ObjectTemplate> optionsTemplate;

protected:
	virtual void addGlobalTemplateFields(v8::Handle<v8::ObjectTemplate> &templ);

	virtual void addSkinOptionTemplateFields(v8::Handle<v8::ObjectTemplate> &templ);

};



#endif // __V8_WRAPPERS_H__
