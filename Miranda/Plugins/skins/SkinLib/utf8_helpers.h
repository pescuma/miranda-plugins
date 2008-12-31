#ifndef __UTF8_HELPERS_H__
# define __UTF8_HELPERS_H__

#include <windows.h>
#include "scope.h"


class TcharToUtf8
{
public:
	TcharToUtf8(const char *str) : utf8(NULL)
	{
		int size = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
		if (size <= 0)
			throw _T("Could not convert string to WCHAR");

		scope<WCHAR *> tmp = (WCHAR *) malloc(size * sizeof(WCHAR));
		if (tmp == NULL)
			throw _T("malloc returned NULL");

		MultiByteToWideChar(CP_ACP, 0, str, -1, tmp, size);

		init(tmp);
	}


	TcharToUtf8(const WCHAR *str) : utf8(NULL)
	{
		init(str);
	}


	~TcharToUtf8()
	{
		if (utf8 != NULL)
			free(utf8);
	}

	operator char *()
	{
		return utf8;
	}

private:
	char *utf8;

	void init(const WCHAR *str)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
		if (size <= 0)
			throw _T("Could not convert string to UTF8");

		utf8 = (char *) malloc(size);
		if (utf8 == NULL)
			throw _T("malloc returned NULL");

		WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8, size, NULL, NULL);
	}
};



class Utf8ToTchar
{
public:
	Utf8ToTchar(const char *str) : tchar(NULL)
	{
		if (str == NULL)
			return;

		int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
		if (size <= 0)
			throw _T("Could not convert string to WCHAR");

		scope<WCHAR *> tmp = (WCHAR *) malloc(size * sizeof(WCHAR));
		if (tmp == NULL)
			throw _T("malloc returned NULL");

		MultiByteToWideChar(CP_UTF8, 0, str, -1, tmp, size);

#ifdef UNICODE

		tchar = tmp.detach();

#else

		size = WideCharToMultiByte(CP_ACP, 0, tmp, -1, NULL, 0, NULL, NULL);
		if (size <= 0)
			throw _T("Could not convert string to ACP");

		tchar = (TCHAR *) malloc(size);
		if (tchar == NULL)
			throw _T("malloc returned NULL");

		WideCharToMultiByte(CP_ACP, 0, tmp, -1, tchar, size, NULL, NULL);

#endif
	}


	~Utf8ToTchar()
	{
		if (tchar != NULL)
			free(tchar);
	}

	TCHAR *detach()
	{
		TCHAR *ret = tchar;
		tchar = NULL;
		return ret;
	}

	operator TCHAR *()
	{
		return tchar;
	}

private:
	TCHAR *tchar;
};


#endif // __UTF8_HELPERS_H__