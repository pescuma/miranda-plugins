#include "commons.h"
#include "MirandaSkinnedDialog.h"

#define SETTING_NAME_SIZE 256


MirandaSkinnedDialog::MirandaSkinnedDialog(const char *name, const char *aModule) 
		: SkinnedDialog(name), module(aModule)
{
}

MirandaSkinnedDialog::~MirandaSkinnedDialog()
{
}

const char * MirandaSkinnedDialog::getModule() const
{
	return module.c_str();
}

const TCHAR * MirandaSkinnedDialog::getSkinName()
{
	getSettting("Skin", _T(DEFAULT_SKIN_NAME), skinName);

	return skinName.c_str();
}

bool MirandaSkinnedDialog::finishedConfiguring()
{
	SkinOptions * opts = getOpts();
	if (getDefaultState() == NULL || opts == NULL)
		return false;

	for(unsigned int i = 0; i < getFieldCount(); ++i)
	{
		MirandaField *field = dynamic_cast<MirandaField *>(getField(i));
		field->configure();
	}

	for(unsigned int i = 0; i < opts->getNumOptions(); ++i)
	{
		SkinOption *opt = opts->getOption(i);
		loadFromDB(opt);
		opt->setOnChangeCallback(&staticOnOptionChange, this);
	}

	return true;
}

void MirandaSkinnedDialog::updateFilename()
{
	std::tstring filename;
	filename = skinsFolder;
	filename += _T("\\");
	filename += getSkinName();
	filename += _T("\\");
	filename += Utf8ToTchar(getName());
	filename += _T(".");
	filename += _T(SKIN_EXTENSION);

	setFilename(filename.c_str());
}

void MirandaSkinnedDialog::loadFromDB(SkinOption *opt)
{
	switch(opt->getType())
	{
		case CHECKBOX:
		{
			opt->setValueCheckbox(getSettting(opt->getName(), opt->getValueCheckbox()));
			break;
		}
		case NUMBER:
		{
			opt->setValueNumber(getSettting(opt->getName(), opt->getValueNumber()));
			break;
		}
		case TEXT:
		{
			std::tstring tmp;
			getSettting(opt->getName(), opt->getValueText(), tmp);
			opt->setValueText(tmp.c_str());
			break;
		}
	}
}

void MirandaSkinnedDialog::storeToDB(const SkinOption *opt)
{
	switch(opt->getType())
	{
		case CHECKBOX:
		{
			setSettting(opt->getName(), opt->getValueCheckbox());
			break;
		}
		case NUMBER:
		{
			setSettting(opt->getName(), opt->getValueNumber());
			break;
		}
		case TEXT:
		{
			setSettting(opt->getName(), opt->getValueText());
			break;
		}
	}
}

bool MirandaSkinnedDialog::getSettting(const char *name, bool defVal)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	return DBGetContactSettingByte(NULL, getModule(), setting, defVal ? 1 : 0) != 0;
}

void MirandaSkinnedDialog::setSettting(const char *name, bool val)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBWriteContactSettingByte(NULL, getModule(), setting, val ? 1 : 0);
}

int MirandaSkinnedDialog::getSettting(const char *name, int defVal)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	return DBGetContactSettingDword(NULL, getModule(), setting, defVal);
}

void MirandaSkinnedDialog::setSettting(const char *name, int val)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBWriteContactSettingDword(NULL, getModule(), setting, val);
}

void MirandaSkinnedDialog::getSettting(const char *name, const WCHAR *defVal, std::wstring &ret)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBVARIANT dbv = {0};
	if (DBGetContactSettingWString(NULL, getModule(), setting, &dbv))
	{
		ret = defVal;
		return;
	}

	ret = dbv.pwszVal;
	DBFreeVariant(&dbv);
}

void MirandaSkinnedDialog::setSettting(const char *name, const WCHAR *val)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBWriteContactSettingWString(NULL, getModule(), setting, val);
}

void MirandaSkinnedDialog::getSettting(const char *name, const char *defVal, std::string &ret)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBVARIANT dbv = {0};
	if (DBGetContactSettingString(NULL, getModule(), setting, &dbv))
	{
		ret = defVal;
		return;
	}

	ret = dbv.pszVal;
	DBFreeVariant(&dbv);
}

void MirandaSkinnedDialog::setSettting(const char *name, const char *val)
{
	char setting[SETTING_NAME_SIZE];
	getSettingName(setting, name);

	DBWriteContactSettingString(NULL, getModule(), setting, val);
}

void MirandaSkinnedDialog::getSettingName(char *setting, const char * name)
{
	_snprintf(setting, SETTING_NAME_SIZE, "%s_%s", getName(), name);
}

void MirandaSkinnedDialog::onOptionChange(const SkinOption *opt)
{
	storeToDB(opt);
}

void MirandaSkinnedDialog::staticOnOptionChange(void *param, const SkinOption *opt)
{
	_ASSERT(param != NULL);

	((MirandaSkinnedDialog *) param)->onOptionChange(opt);
}
