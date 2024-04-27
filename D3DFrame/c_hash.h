#ifndef _C_HASH_H
#define _C_HASH_H
#include "c_base.h"

// 128bits 16bytes
typedef struct MD5_16
{
	DWORD _0;
	DWORD _1;
	DWORD _2;
	DWORD _3;
}C_HASH_MD5_16;

DWORD bkdrhashW(LPWSTR wstr);
DWORD bkdrhashA(LPSTR str);


#endif