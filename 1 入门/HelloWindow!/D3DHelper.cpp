#include "D3DHelper.h"

DXException::DXException(HRESULT hr)
{
    wsprintf(e, TEXT("hr: %ld"), hr);
}

DXException::DXException(HRESULT hr, const char* file, unsigned int line)
{
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d"), hr, file, line);
}

DXException::~DXException(){}

TCHAR* DXException::ToString()
{
    return e;
}