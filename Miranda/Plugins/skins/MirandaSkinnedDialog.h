#ifndef __MIRANDA_SKINNED_DIALOG_H__
# define __MIRANDA_SKINNED_DIALOG_H__

#include "SkinLib\SkinnedDialog.h"

class SkinOption;


class MirandaSkinnedDialog : public SkinnedDialog
{
public:
	MirandaSkinnedDialog(const char *name, const char *module);
	virtual ~MirandaSkinnedDialog();

	virtual const char * getModule() const;
	virtual const TCHAR * getSkinName();

	virtual bool finishedConfiguring();


private:
	std::string module;
	std::tstring skinName;

	void updateFilename();

	void loadFromDB(SkinOption *opt);
	void storeToDB(const SkinOption *opt);

	bool getSettting(const char *name, bool defVal);
	void setSettting(const char *name, bool val);
	int getSettting(const char *name, int defVal);
	void setSettting(const char *name, int val);
	void getSettting(const char *name, const WCHAR *defVal, std::wstring &ret);
	void setSettting(const char *name, const WCHAR *val);
	void getSettting(const char *name, const char *defVal, std::string &ret);
	void setSettting(const char *name, const char *val);

	inline void getSettingName(char *setting, const char * name);

	void onOptionChange(const SkinOption *opt);

	static void staticOnOptionChange(void *param, const SkinOption *opt);
};


#endif // __MIRANDA_SKINNED_DIALOG_H__
