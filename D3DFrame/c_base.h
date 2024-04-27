#ifndef _C_BASE_H
#define _C_BASE_H
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <windows.h>

#ifdef __cplusplus
#define EXPORT extern "C" __declspec(dllexport) 
#else
#define EXPORT __declspec(dllexport)
#endif

typedef void* UNDEF_TYPE;
#define C_BASE_POINTER_BYTES_SIZE sizeof(void*)
#if defined(_M_X86)
#define __WIN32__
#define ADDRESS UINT
#else
#define __WIN64__
#define ADDRESS unsigned long long
#endif
#endif

#ifdef WIN_SAL
#include <sal.h>
#endif