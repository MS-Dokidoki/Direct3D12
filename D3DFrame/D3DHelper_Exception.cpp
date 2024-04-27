#include "D3DHelper_Exception.h"

D3DHelper_Exception::D3DHelper_Exception(){}

D3DHelper_Exception::D3DHelper_Exception(LPWSTR str)
{
    lstrcpynW(Buffer, str, D3DHELPER_EXCEPTION_BUFFER_MAX);
}

LPWSTR D3DHelper_Exception::ToString() const
{
    return (LPWSTR)Buffer;
}

DirectX_Exception::DirectX_Exception(HRESULT hr, const char* file, int line)
{
    wsprintfW(Buffer, L"DirectX_Exception: \nHRESULT: %ld \nFile: %S \nLine: %d", hr, file, line);
}

DirectX_Exception::DirectX_Exception(HRESULT hr, const char* file, int line, ID3DBlob** extra)
{
    if(*extra)
        wsprintfW(Buffer, L"DirectX_Exception: \nHRESULT: %ld \nFile: %S \nLine: %d \nExtra: %S", hr, file, line, (char*)(*extra)->GetBufferPointer());
    else
        DirectX_Exception(hr, file, line);
}
