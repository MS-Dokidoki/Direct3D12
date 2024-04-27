#ifndef _D3DHELPER_H
#define _D3DHELPER_H
#include "D3DBase.h"
#define DXEXCEPTION_MAXSTRING MAX_PATH

class DXException
{
public:
    DXException(HRESULT);
    DXException(HRESULT, const char*, unsigned int);
    ~DXException();

    TCHAR* ToString();
private:
    TCHAR e[DXEXCEPTION_MAXSTRING];
};

#define ThrowIfFailed(hr) if(FAILED(hr)){throw DXException(hr);}
#define ThrowIfFailedEx(hr) if(FAILED(hr)){throw DXException(hr, __FILE__, __LINE__);}

namespace D3DHelper
{
    UINT CalcConstantBufferBytesSize(UINT bytesSize);
};

#endif