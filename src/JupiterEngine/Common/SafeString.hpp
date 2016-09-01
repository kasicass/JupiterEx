#pragma once

#include <string.h>
#include <assert.h>

inline unsigned long LTStrLen(const char* s)
{
	if (s) return strlen(s);
	else return 0;
}

inline bool LTStrEmpty(const char* s)
{
	return !s || (s[0] == '\0');
}

inline void LTStrCpy(char *dest, const char *src, unsigned long sz)
{
	if (!dest || (sz == 0))
	{
		assert(0 && "LTStrCpy: Invalid destination buffer provided");
		return;
	}

	if (!src)
	{
		assert(0 && "LTStrCpy: Invalid source string provided");
		src = "";
	}

	if (!(sz > LTStrLen(src)))
	{
		assert(0 && "LTStrCpy: Copy source is truncated");
	}

	strncpy(dest, src, sz-1);
	dest[sz-1] = '\0';
}

inline void LTStrnCpy(char *dest, const char *src, unsigned long destBytes, unsigned long srcLen)
{
	if (destBytes == 0)
		return;

	unsigned long copyLen = (destBytes <= srcLen) ? (destBytes-1) : srcLen;

#if defined(_DEBUG)
	if (src && copyLen != srcLen)
	{
		assert(0 && "LTStrnCpy: Copy source is truncated");
	}
#endif

	strncpy(dest, src, copyLen);
	dest[copyLen] = '\0';
}