#include "D3DHelper.h"

using namespace Microsoft::WRL;
using namespace D3DHelper;

Microsoft::WRL::ComPtr<ID3DBlob> D3DHelper::CompileShader(LPCWSTR lpShaderFile, const D3D_SHADER_MACRO* pDefines, const char* pEntryPoint, const char* pTarget)
{
    UINT uiCompileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> pByteCode;
    ComPtr<ID3DBlob> pError;
    HRESULT hr = D3DCompileFromFile(lpShaderFile, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, pEntryPoint, pTarget, uiCompileFlags, 0, &pByteCode, &pError);

    if(pError)
        OutputDebugStringA((char*)pError->GetBufferPointer());
    ThrowIfFailedEx(hr, &pError);
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

Microsoft::WRL::ComPtr<ID3D12Resource> D3DHelper::LoadDDSFromFile(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, LPCWSTR lpszFileName, ID3D12Resource** pUploader)
{
    ComPtr<ID3D12Resource> pResource;
    std::unique_ptr<UINT8[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    
    ThrowIfFailed(DirectX::LoadDDSTextureFromFile(pDevice, lpszFileName, &pResource, ddsData, subresources));

    UINT64 nBytesSize = GetRequiredIntermediateSize(pResource.Get(), 0, subresources.size());

    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nBytesSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(pUploader)
    ));

    UpdateSubresources(pComList, pResource.Get(), *pUploader, 0, 0, subresources.size(), &subresources[0]);

    pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    ));

    return pResource;
}