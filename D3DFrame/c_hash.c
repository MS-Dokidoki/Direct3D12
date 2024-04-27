#include "c_hash.h"

DWORD bkdrhashW(LPWSTR wstr)
{
	DWORD seed = 31;
	DWORD hash = 0;
	wchar_t* c = wstr;
	
	while(*c)
	{
		hash = hash * seed + (*c++);
	}
	return hash;
}

DWORD bkdrhashA(LPSTR str)
{
    DWORD seed = 31;
	DWORD hash = 0;
	char* c = str;
	
	while(*c)
	{
		hash = hash * seed + (*c++);
	}
	
	return hash;
}