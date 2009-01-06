#include "globals.h"
#include "V8Script.h"

#include <utf8_helpers.h>

using namespace v8;



V8Script::V8Script() : exceptionCallback(NULL), exceptionCallbackParam(NULL)
{
}

V8Script::~V8Script()
{
	dispose();
}

static Handle<Value> IsEmptyCallback(const Arguments& args)
{
	if (args.Length() < 1) 
		return Undefined();
	
	HandleScope scope;

	for(int i = 0; i < args.Length(); i++)
	{
		Local<Value> arg = args[0];

		if (arg.IsEmpty() || arg->IsNull() || arg->IsUndefined())
		{
			return Boolean::New(true);
		}
		else if (arg->IsObject())
		{
			Local<Object> self = Local<Object>::Cast(arg);
			if (self->InternalFieldCount() < 1)
				continue;

			Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
			FieldState *field = (FieldState *) wrap->Value();
			if (field == NULL)
				continue;

			if (field->isEmpty())
				return Boolean::New(true);
		}
		else if (arg->IsString())
		{
			Local<String> str = Local<String>::Cast(arg);
			if (str->Length() <= 0)
				return Boolean::New(true);
		}
	}

	return Boolean::New(false);
}

static Handle<Value> RGBCallback(const Arguments& args)
{
	if (args.Length() != 3) 
		return Undefined();

	COLORREF color = RGB(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value());
	return Int32::New(color);
}

static Handle<Value> AlertCallback(const Arguments& args)
{
	Local<External> wrap = Local<External>::Cast(args.Data());
	if (wrap.IsEmpty())
		return Int32::New(-1);

	Dialog *dialog = (Dialog *) wrap->Value();
	if (dialog == NULL)
		return Int32::New(-1);

	if (args.Length() < 1) 
		return Int32::New(-1);

	Local<Value> arg = args[0];
	if (!arg->IsString())
		return Int32::New(-1);

	Local<String> str = Local<String>::Cast(arg);
	String::Utf8Value utf8_value(str);

	std::tstring title;
	title = CharToTchar(dialog->getName());
	title += _T(" - Skin alert");
	MessageBox(NULL, Utf8ToTchar(*utf8_value), title.c_str(), MB_OK);

	return Int32::New(0);
}

bool V8Script::compile(const TCHAR *source, Dialog *dlg)
{
	dispose();

	HandleScope handle_scope;

	Handle<ObjectTemplate> global = ObjectTemplate::New();
	
	global->Set(String::New("IsEmpty"), FunctionTemplate::New(&IsEmptyCallback));
	global->Set(String::New("RGB"), FunctionTemplate::New(&RGBCallback));
	global->Set(String::New("alert"), FunctionTemplate::New(&AlertCallback, External::New(dlg)));

	global->Set(String::New("NUMBER"), String::New("NUMBER"));
	global->Set(String::New("CHECKBOX"), String::New("CHECKBOX"));
	global->Set(String::New("TEXT"), String::New("TEXT"));
	global->Set(String::New("LEFT"), String::New("LEFT"));
	global->Set(String::New("CENTER"), String::New("CENTER"));
	global->Set(String::New("RIGHT"), String::New("RIGHT"));

	context = Context::New(NULL, global);

	Context::Scope context_scope(context);

	context->Global()->Set(String::New("window"), wrappers.createDialogWrapper(), ReadOnly);
	context->Global()->Set(String::New("opts"), wrappers.createOptionsWrapper(), ReadOnly);
	for(unsigned int i = 0; i < dlg->getFieldCount(); i++)
	{
		Field *field = dlg->getField(i);
		context->Global()->Set(String::New(field->getName()), wrappers.createWrapper(field->getType()), ReadOnly);
	}
	wrappers.clearTemplates();

	TryCatch try_catch;
	Local<Script> tmpScript = Script::Compile(String::New((const V8_TCHAR *) source), String::New(dlg->getName()));
	if (tmpScript.IsEmpty()) 
	{
		reportException(&try_catch);
		dispose();
		return false;
	}

	script = Persistent<Script>::New(tmpScript);

	return true;
}

void V8Script::dispose()
{
	script.Dispose();
	script.Clear();

	context.Dispose();
	context.Clear();
}

bool V8Script::isValid()
{
	return !context.IsEmpty() && !script.IsEmpty();
}

static Handle<Object> get(Local<Object> obj, const char *field)
{
	return Handle<Object>::Cast(obj->Get(String::New(field)));
}

Handle<Function> V8Script::getConfigureFunction(Dialog *dlg)
{
	DialogState *state = dlg->createState();

	HandleScope handle_scope;

	Context::Scope context_scope(context);

	// Run once to get function
	Local<Object> global = context->Global();
	wrappers.fillWrapper(get(global, "window"), state);
	wrappers.fillWrapper(get(global, "opts"), (SkinOptions *) NULL, false);
	for(unsigned int i = 0; i < state->fields.size(); i++)
	{
		FieldState *field = state->fields[i];
		wrappers.fillWrapper(get(global, field->getField()->getName()), field);
	}

	// I don't care for the catch here
	TryCatch try_catch;
	script->Run();

	delete state;

	Handle<Value> configure_val = context->Global()->Get(String::New("configure"));
	if (configure_val.IsEmpty() || !configure_val->IsFunction()) 
		return Handle<Function>();

	Handle<Function> configure = Handle<Function>::Cast(configure_val);

	return handle_scope.Close(configure);
}

std::pair<SkinOptions *,DialogState *> V8Script::configure(Dialog *dlg)
{
	if (!isValid())
		return std::pair<SkinOptions *,DialogState *>(NULL, NULL);

	SkinOptions *opts = new SkinOptions();
	DialogState *state = dlg->createState();

	HandleScope handle_scope;

	Context::Scope context_scope(context);

	Handle<Function> configure = getConfigureFunction(dlg);
	if (configure.IsEmpty())
		return std::pair<SkinOptions *,DialogState *>(opts, state);

	Local<Object> global = context->Global();
	wrappers.fillWrapper(get(global, "opts"), opts, true);
	wrappers.fillWrapper(get(global, "window"), state);
	for(unsigned int i = 0; i < state->fields.size(); i++)
	{
		FieldState *field = state->fields[i];
		wrappers.fillWrapper(get(global, field->getField()->getName()), field);
	}

	TryCatch try_catch;
	Handle<Value> result = configure->Call(global, 0, NULL);
	if (result.IsEmpty()) 
	{
		reportException(&try_catch);
		delete opts;
		delete state;
		return std::pair<SkinOptions *,DialogState *>(NULL, NULL);;
	} 

	return std::pair<SkinOptions *,DialogState *>(opts, state);
}

bool V8Script::run(DialogState * state, SkinOptions *opts)
{
	if (!isValid())
		return false;

	HandleScope handle_scope;

	Context::Scope context_scope(context);

	Local<Object> global = context->Global();

	wrappers.fillWrapper(get(global, "window"), state);
	wrappers.fillWrapper(get(global, "opts"), opts, false);
	for(unsigned int i = 0; i < state->fields.size(); i++)
	{
		FieldState *field = state->fields[i];
		wrappers.fillWrapper(get(global, field->getField()->getName()), field);
	}

	TryCatch try_catch;
	Handle<Value> result = script->Run();
	if (result.IsEmpty()) 
	{
		reportException(&try_catch);
		return false;
	}

	return true;
}

void V8Script::setExceptionCallback(ExceptionCallback cb, void *param)
{
	exceptionCallback = cb;
	exceptionCallbackParam = param;
}

void V8Script::reportException(v8::TryCatch *try_catch)
{
	std::string err;
	char tmp[1024];

	HandleScope handle_scope;
	String::Utf8Value exception(try_catch->Exception());
	Handle<Message> message = try_catch->Message();
	if (message.IsEmpty()) 
	{
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		_snprintf(tmp, 1024, "%s\n", *exception);
		err += tmp;
	}
	else 
	{
		// Print (filename):(line number): (message).
		String::Utf8Value filename(message->GetScriptResourceName());
		int linenum = message->GetLineNumber();
		_snprintf(tmp, 1024, "%s:%i: %s\n", *filename, linenum, *exception);
		err += tmp;

		// Print line of source code.
		String::Utf8Value sourceline(message->GetSourceLine());
		_snprintf(tmp, 1024, "%s\n", *sourceline);
		err += tmp;

		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			err += " ";
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			err += "^";
		}
		err += "\n";
	}

	Utf8ToTchar tcharErr(err.c_str());

	OutputDebugString(tcharErr);

	if (exceptionCallback != NULL)
		exceptionCallback(exceptionCallbackParam, tcharErr);
}
