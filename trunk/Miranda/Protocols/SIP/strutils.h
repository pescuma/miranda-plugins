/* 
Copyright (C) 2009 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#pragma once
#ifndef __STRUTILS_H__
#define __STRUTILS_H__


#define TcharToSip TcharToUtf8

class SipToTchar
{
public:
	SipToTchar(const pj_str_t &str) : tchar(NULL)
	{
		if (str.ptr == NULL)
			return;

		int size = MultiByteToWideChar(CP_UTF8, 0, str.ptr, str.slen, NULL, 0);
		if (size < 0)
			throw _T("Could not convert string to WCHAR");
		size++;

		WCHAR *tmp = (WCHAR *) mir_alloc(size * sizeof(WCHAR));
		if (tmp == NULL)
			throw _T("mir_alloc returned NULL");

		MultiByteToWideChar(CP_UTF8, 0, str.ptr, str.slen, tmp, size);
		tmp[size-1] = 0;

#ifdef UNICODE

		tchar = tmp;

#else

		size = WideCharToMultiByte(CP_ACP, 0, tmp, -1, NULL, 0, NULL, NULL);
		if (size <= 0)
		{
			mir_free(tmp);
			throw _T("Could not convert string to ACP");
		}

		tchar = (TCHAR *) mir_alloc(size * sizeof(char));
		if (tchar == NULL)
		{
			mir_free(tmp);
			throw _T("mir_alloc returned NULL");
		}

		WideCharToMultiByte(CP_ACP, 0, tmp, -1, tchar, size, NULL, NULL);

		mir_free(tmp);

#endif
	}

	~SipToTchar()
	{
		if (tchar != NULL)
			mir_free(tchar);
	}

	TCHAR *detach()
	{
		TCHAR *ret = tchar;
		tchar = NULL;
		return ret;
	}

	TCHAR * get() const
	{
		return tchar;
	}

	operator TCHAR *() const
	{
		return tchar;
	}

	const TCHAR & operator[](int pos) const
	{
		return tchar[pos];
	}

private:
	TCHAR *tchar;
};


static pj_str_t pj_str(const char *str)
{
	pj_str_t ret;
	pj_cstr(&ret, str);
	return ret;
}


static char * mir_pjstrdup(const pj_str_t *from)
{
	char *ret = (char *) mir_alloc(from->slen + 1);
	strncpy(ret, from->ptr, from->slen);
	ret[from->slen] = 0;
	return ret;
}


static int FirstGtZero(int n1, int n2)
{
	if (n1 > 0)
		return n1;
	return n2;
}


static TCHAR *CleanupSip(TCHAR *str)
{
	if (_tcsnicmp(_T("sip:"), str, 4) == 0)
		return &str[4];
	else
		return str;
}


static const TCHAR *CleanupSip(const TCHAR *str)
{
	if (_tcsnicmp(_T("sip:"), str, 4) == 0)
		return &str[4];
	else
		return str;
}


static void CleanupNumber(TCHAR *out, int outSize, const TCHAR *number)
{
	int pos = 0;
	int len = lstrlen(number);
	for(int i = 0; i < len && pos < outSize - 1; ++i)
	{
		TCHAR c = number[i];

		if (i == 0 && c == _T('+'))
			out[pos++] = c;
		else if (c >= _T('0') && c <= _T('9'))
			out[pos++] = c;
	}
	out[pos] = 0;
}


static void RemoveLtGt(TCHAR *str)
{
	int len = lstrlen(str);
	if (str[0] == _T('<') && str[len-1] == _T('>'))
	{
		str[len-1] = 0;
		memmove(str, &str[1], len * sizeof(TCHAR));
	}
}


static bool IsValidDTMF(TCHAR c)
{
	if (c >= _T('A') && c <= _T('D'))
		return true;
	if (c >= _T('0') && c <= _T('9'))
		return true;
	if (c == _T('#') || c == _T('*'))
		return true;

	return false;
}


static const pj_str_t pj_cstr(const char *s)
{
	pj_str_t str;
    str.ptr = (char*)s;
    str.slen = s ? (pj_ssize_t)strlen(s) : 0;
    return str;
}


#endif // __STRUTILS_H__
