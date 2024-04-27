#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <unordered_map>

typedef HANDLE BASE_HANDLE;
typedef char BASE_BITSIGN;

#define BASE_MALLOC(byte) malloc(byte)
#define BASE_MFREE(p) free(p)
#define BASE_MZERO(p, byte) memset(p, 0, byte)

#define BASE_IsDigit(c) (c >= '0' && c <= '9')
#define BASE_IsUnmeaning(c) (c == ' ' || c == '\0' || c == '\r' || c == '\n' || c == '\b' || c == '\t')

#define BASE_IsDigitW(c) (c >= L'0' && c <= L'9')
#define BASE_IsUnmeaningW(c) (c == L' ' || c == L'\0' || c == L'\r' || c == L'\n' || c == L'\b' || c == L'\t')

// 11011101 | 00001000
// 11111111 & 11101111
// 00011111 & 00000001 
#define BASE_SetSignTrue(var, bit) (var | (1 << bit))
#define BASE_SetSignFalse(var, bit) (var & !(1 << bit))
#define BASE_SetSign(var, bit, t) t? BASE_SetSignTrue(var, bit): BASE_SetSignFalse(var, bit)
#define BASE_GetSign(var, bit) ((var >> bit) & 1)