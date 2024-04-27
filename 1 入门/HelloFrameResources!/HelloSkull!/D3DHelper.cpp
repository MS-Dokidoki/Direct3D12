#include "D3DHelper.h"

/************************/
/** Exception */
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

DXException::DXException(HRESULT hr, ID3DBlob** error, const char* file, unsigned int line)
{
    TCHAR buffer[DXEXCEPTION_MAXSTRINGEX] = {0};
    UINT errorBegin;

#ifdef UNICODE
    errorBegin = MultiByteToWideChar(CP_ACP, 0, file, -1, buffer, DXEXCEPTION_MAXSTRINGEX);
    if(*error)
    {
        errorBegin += 2;
        MultiByteToWideChar(CP_ACP, 0, (char*)(*error)->GetBufferPointer(), -1, &buffer[errorBegin], DXEXCEPTION_MAXSTRINGEX - errorBegin);
    }
#endif
    
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d\nlog: %s"), hr, buffer, line, &buffer[errorBegin]);
}

DXException::~DXException(){}

TCHAR* DXException::ToString()
{
    return e;
}

/****************************************/
/** D3DHelper */
using namespace Microsoft::WRL;
using namespace D3DHelper;

Microsoft::WRL::ComPtr<ID3DBlob> D3DHelper::CompileShader(LPCWSTR lpShaderFile, const D3D_SHADER_MACRO* pDefines, const char* pEntryPoint, const char* pTarget)
{
    UINT uiCompileFlags = 0;
#ifdef _DEBUG
    uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> pByteCode;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3DCompileFromFile(lpShaderFile, pDefines, NULL, pEntryPoint, pTarget, uiCompileFlags, 0, &pByteCode, &pError), &pError);

    return pByteCode;
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3DHelper::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, 
    const void* pData, UINT nByteSize, ID3D12Resource** pUploader)
{
    ComPtr<ID3D12Resource> pDefaultBuffer;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nByteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&pDefaultBuffer)
    ));

    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nByteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(pUploader)
    ));

    void* pBegin;
    (*pUploader)->Map(0, NULL, &pBegin);
    memcpy(pBegin, pData, nByteSize);
    (*pUploader)->Unmap(0, NULL);

    pComList->CopyResource(pDefaultBuffer.Get(), *pUploader);
    pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pDefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    return pDefaultBuffer;
}

UploadBuffer::UploadBuffer()
{

}

UploadBuffer::UploadBuffer(ID3D12Device* pDevice, UINT nElementByteSize, UINT nElementsCount)
{
    Init(pDevice, nElementByteSize, nElementsCount);
}

UploadBuffer::~UploadBuffer()
{
    if(pBufferBegin)
        pUploadBuffer->Unmap(0, NULL);
    pBufferBegin = NULL;
}
void UploadBuffer::Init(ID3D12Device* pDevice, UINT nElementByteSize, UINT nElementsCount)
{
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nElementByteSize * nElementsCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pUploadBuffer)
    ));
    ThrowIfFailed(pUploadBuffer->Map(0, NULL, (void**)&pBufferBegin));
}

ID3D12Resource* UploadBuffer::Resource() const
{
    return pUploadBuffer.Get();
}

void UploadBuffer::CopyData(int nElementIndex, const void* pData, const UINT nByteSize)
{
    memcpy(&pBufferBegin[nElementIndex * nElementByteSize], pData, nByteSize);
}