#ifndef __V8_SCRIPT_H__
# define __V8_SCRIPT_H__

#include <v8.h>
#include "V8Wrappers.h"

typedef void (*ExceptionCallback)(void *param, const TCHAR *err);


class V8Script
{
public:
	V8Script();
	~V8Script();

	bool compile(const TCHAR *source, Dialog *dlg);
	void dispose();

	bool isValid();

	SkinOptions * createOptions(Dialog *dlg);

	bool run(DialogState * state, SkinOptions *opts);

	void setExceptionCallback(ExceptionCallback cb, void *param = NULL);

private:
	V8Wrappers wrappers;
	v8::Persistent<v8::Context> context;
	v8::Persistent<v8::Script> script;

	ExceptionCallback exceptionCallback;
	void *exceptionCallbackParam;

	v8::Handle<v8::Function> getOptionsFunction(Dialog *dlg);
	void reportException(v8::TryCatch *try_catch);
};



#endif // __V8_SCRIPT_H__
