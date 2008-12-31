#ifndef __SKINNED_DIALOG_H__
# define __SKINNED_DIALOG_H__

#include <windows.h>
#include <tchar.h>
#include "tstring.h"

#include "Dialog.h"
#include "ButtonField.h"
#include "EditField.h"
#include "IconField.h"
#include "ImageField.h"
#include "LabelField.h"
#include "TextField.h"

#include "DialogState.h"
#include "BorderState.h"
#include "ButtonFieldState.h"
#include "EditFieldState.h"
#include "FontState.h"
#include "IconFieldState.h"
#include "ImageFieldState.h"
#include "LabelFieldState.h"
#include "TextFieldState.h"

class V8Script;
class SkinOptions;

typedef void (*MessageCallback)(void *param, const TCHAR *err);



class SkinnedDialog
{
public:
	SkinnedDialog(const char *name);
	~SkinnedDialog();

	const TCHAR * getFilename() const;
	void setFilename(const char *skin, const TCHAR *filename);

	bool addField(Field *field);
	Field * getField(const char *name) const;
	Field * getField(unsigned int pos) const;
	int getFieldCount() const;

	const Size & getSize() const;
	void setSize(const Size &size);

	/// Return the cached state. Do not free the result. 
	/// Each call to this method can potentially create the state, so don't cache it.
	DialogState * getState();

	/// Create a state based on the script passed in text. the caller have to free the DialogState *
	DialogState * createState(const TCHAR *text, MessageCallback errorCallback = NULL, void *errorCallbackParam = NULL);

	void setErrorCallback(MessageCallback cb, void *param = NULL);
	void setTraceCallback(MessageCallback cb, void *param = NULL);

private:
	Dialog dlg;
	std::string skin;
	std::tstring filename;
	__time64_t fileChangedTime;
	V8Script *script;
	DialogState *state;
	SkinOptions *opts;

	MessageCallback errorCallback;
	void *errorCallbackParam;
	MessageCallback traceCallback;
	void *traceCallbackParam;

	void releaseCompiledScript();
	void releaseState();
	bool fileChanged();
	void readFile(std::tstring &ret);

	void trace(TCHAR *msg, ...);

	void onFieldChange(const Field *field);

	static void staticOnFieldChange(void *param, const Field *field);
};




#endif // __SKINNED_DIALOG_H__
