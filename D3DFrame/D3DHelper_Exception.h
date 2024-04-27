#pragma once
#ifndef _D3DHELPER_EXCEPTION_H
#define _D3DHELPER_EXCEPTION_H
#include "D3DBase.h"
#define D3DHELPER_EXCEPTION_BUFFER_MAX 512

class D3DHelper_Exception
{
public:
    D3DHelper_Exception();
    D3DHelper_Exception(LPWSTR str);
    LPWSTR ToString() const;
protected:
    WCHAR Buffer[D3DHELPER_EXCEPTION_BUFFER_MAX];
};

class DirectX_Exception : public D3DHelper_Exception
{
public:
    DirectX_Exception(HRESULT hr, const char* file, int line);
    DirectX_Exception(HRESULT hr, const char* file, int line, ID3DBlob** extra);

};

#define ThrowIfFailed(hr)                          \
    if (FAILED(hr))                                \
    {                                              \
        throw DirectX_Exception(hr, __FILE__, __LINE__); \
    }
#define ThrowIfFailedEx(hr, error)                        \
    if (FAILED(hr))                                       \
    {                                                     \
        throw DirectX_Exception(hr, __FILE__, __LINE__, error); \
    }
#endif