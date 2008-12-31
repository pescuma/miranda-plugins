#include "globals.h"
#include "V8Script.h"

#include "utf8_helpers.h"

using namespace v8;



V8Script::V8Script() : exceptionCallback(NULL), exceptionCallbackParam(NULL)
{
}

V8Script::~V8Script()
{
	dispose();
}

bool V8Script::compile(const TCHAR *source, Dialog *dlg)
{
	dispose();

	HandleScope handle_scope;

	context = Context::New();

	Context::Scope context_scope(context);

	context->Global()->Set(String::New("window"), wrappers.createDialogWrapper(), ReadOnly);
	context->Global()->Set(String::New("opts"), wrappers.createOptionsWrapper(), ReadOnly);
	for(unsigned int i = 0; i < dlg->fields.size(); i++)
	{
		Field *field = dlg->fields[i];
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

Handle<Function> V8Script::getOptionsFunction(Dialog *dlg)
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

	Handle<Value> options_val = context->Global()->Get(String::New("options"));
	if (options_val.IsEmpty() || !options_val->IsFunction()) 
		return Handle<Function>();

	Handle<Function> options = Handle<Function>::Cast(options_val);

	return handle_scope.Close(options);
}

SkinOptions * V8Script::createOptions(Dialog *dlg)
{
	if (!isValid())
		return NULL;

	SkinOptions *opts = new SkinOptions();

	HandleScope handle_scope;

	Context::Scope context_scope(context);

	Handle<Function> options = getOptionsFunction(dlg);
	if (options.IsEmpty())
		return opts;

	Local<Object> global = context->Global();
	wrappers.fillWrapper(get(global, "opts"), opts, true);

	global->Set(String::New("NUMBER"), String::New("NUMBER"));
	global->Set(String::New("CHECKBOX"), String::New("CHECKBOX"));
	global->Set(String::New("TEXT"), String::New("TEXT"));

	TryCatch try_catch;
	Handle<Value> result = options->Call(global, 0, NULL);
	if (result.IsEmpty()) 
	{
		reportException(&try_catch);
		delete opts;
		return NULL;
	} 

	return opts;
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
