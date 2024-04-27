#include "D3DHelper.h"

DXException::DXException(HRESULT hr)
{
    wsprintf(e, TEXT("hr: %ld"), hr);
}

DXException::DXException(HRESULT hr, const char* file, unsigned int line)
{
    TCHAR buffer[DXEXCEPTION_MAXSTRING] = { 0 };

#if defined(UNICODE)
    MultiByteToWideChar(CP_ACP, 0, file, -1, buffer, DXEXCEPTION_MAXSTRING);
#endif
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d"), hr, buffer, line);
}

DXException::~DXException(){}

TCHAR* DXException::ToString()
{
    return e;
}