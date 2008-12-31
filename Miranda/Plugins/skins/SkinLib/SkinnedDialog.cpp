#include "globals.h"
#include "SkinnedDialog.h"

#include <sys/stat.h>
#include "V8Script.h"
#include "utf8_helpers.h"
#include "SkinOptions.h"


SkinnedDialog::SkinnedDialog(const char *name) 
		: dlg(name), fileChangedTime(0), 
		  script(NULL), state(NULL), opts(NULL),
		  errorCallback(NULL), errorCallbackParam(NULL),
		  traceCallback(NULL), traceCallbackParam(NULL)
{
}

SkinnedDialog::~SkinnedDialog()
{
	releaseState();
	releaseCompiledScript();
}

const TCHAR * SkinnedDialog::getFilename() const
{
	return filename.c_str();
}

void SkinnedDialog::setFilename(const char *skin, const TCHAR *filename)
{
	this->skin = skin;

	if (this->filename == filename)
		return;

	this->filename = filename;
	releaseState();
	releaseCompiledScript();
}

bool SkinnedDialog::addField(Field *field)
{
	if (dlg.addField(field))
	{
		releaseCompiledScript();
		releaseState();
		field->setOnChangeCallback(SkinnedDialog::staticOnFieldChange, this);
		return true;
	}
	else
		return false;
}

Field * SkinnedDialog::getField(const char *name) const
{
	return dlg.getField(name);
}

Field * SkinnedDialog::getField(unsigned int pos) const
{
	if (pos >= dlg.fields.size())
		return NULL;
	return dlg.fields[pos];
}

int SkinnedDialog::getFieldCount() const
{
	return dlg.fields.size();
}

const Size & SkinnedDialog::getSize() const
{
	return dlg.getSize();
}

void SkinnedDialog::setSize(const Size &size)
{
	if (dlg.getSize() == size)
		return;

	dlg.setSize(size);
	releaseState();
}

DialogState * SkinnedDialog::getState()
{	
	bool changed = fileChanged();
	if (state != NULL && !changed)
		return state;

	releaseState();

	if (filename.size() <= 0)
		return NULL;

	if (changed || script == NULL)
	{
		releaseCompiledScript();

		struct _stat st = {0};
		if (_tstat(filename.c_str(), &st) != 0)
			return NULL;

		std::tstring text;
		readFile(text);
		if (text.size() <= 0)
			return NULL;

		script = new V8Script();
		script->setExceptionCallback(errorCallback, errorCallbackParam);

		if (!script->compile(text.c_str(), &dlg))
		{
			releaseCompiledScript();
			return NULL;
		}

		opts = script->createOptions(&dlg);
		if (opts == NULL)
		{
			releaseCompiledScript();
			return NULL;
		}

		fileChangedTime = st.st_mtime;
	}

	state = dlg.createState();
	if (!script->run(state, opts))
	{
		releaseState();
		return NULL;
	}

	return state;
}

void SkinnedDialog::releaseCompiledScript()
{
	delete script;
	script = NULL;

	delete opts;
	opts = NULL;
}

void SkinnedDialog::releaseState()
{
	delete state;
	state = NULL;
}

DialogState * SkinnedDialog::createState(const TCHAR *text, MessageCallback errorCallback, void *errorCallbackParam)
{
	V8Script script;
	script.setExceptionCallback(errorCallback, errorCallbackParam);

	if (!script.compile(text, &dlg))
		return NULL;

	DialogState *state = dlg.createState();
	if (!script.run(state, opts))
	{
		delete state;
		return NULL;
	}

	return state;
}


bool SkinnedDialog::fileChanged()
{
	if (filename.size() <= 0)
		return false;

	struct _stat st = {0};
	if (_tstat(filename.c_str(), &st) != 0)
		return false;

	return st.st_mtime > fileChangedTime;
}

void SkinnedDialog::readFile(std::tstring &ret)
{
	FILE* file = _tfopen(filename.c_str(), _T("rb"));
	if (file == NULL) 
		return;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) 
	{
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);

	ret = Utf8ToTchar(chars);

	delete[] chars;
}

void SkinnedDialog::onFieldChange(const Field *field)
{
	releaseState();
}


void SkinnedDialog::staticOnFieldChange(void *param, const Field *field)
{
	_ASSERT(param != NULL);
	_ASSERT(field != NULL);

	SkinnedDialog *skdlg = (SkinnedDialog *) param;
	skdlg->onFieldChange(field);
}

void SkinnedDialog::setErrorCallback(MessageCallback cb, void *param /*= NULL*/)
{
	errorCallback = cb;
	errorCallbackParam = param;
}

void SkinnedDialog::setTraceCallback(MessageCallback cb, void *param /*= NULL*/)
{
	traceCallback = cb;
	traceCallbackParam = param;
}

void SkinnedDialog::trace(TCHAR *msg, ...)
{
	if (traceCallback == NULL)
		return;

	TCHAR buff[1024];
	memset(buff, 0, sizeof(buff));

	va_list args;
	va_start(args, msg);

	_vsntprintf(buff, MAX_REGS(buff) - 1, msg, args);
	buff[MAX_REGS(buff) - 1] = 0;

	va_end(args);

	traceCallback(traceCallbackParam, buff);
}