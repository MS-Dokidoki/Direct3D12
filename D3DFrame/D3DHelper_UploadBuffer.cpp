#include "D3DHelper_UploadBuffer.h"
#include "D3DHelper_Exception.h"
using namespace D3DHelper;

UploadBuffer::UploadBuffer(){}

UploadBuffer::UploadBuffer(ID3D12Device* pDevice, UINT nByteSize, UINT nCount)
{
    Init(pDevice, nByteSize, nCount);
}

UploadBuffer::~UploadBuffer()
{
    if(pBufferBegin)
        pUploadBuffer->Unmap(0, NULL);
    pBufferBegin = NULL;
}

void UploadBuffer::Init(ID3D12Device* Device, UINT ByteSize, UINT Count)
{
    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(ByteSize * Count),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pUploadBuffer)
    ));
    ThrowIfFailed(pUploadBuffer->Map(0, NULL, (void**)&pBufferBegin));

    nElementByteSize = ByteSize;
}

ID3D12Resource* UploadBuffer::Resource() const
{
    return pUploadBuffer.Get();
}

void UploadBuffer::CopyData(int nElementIndex, const void* pData, const UINT nByteSize)
{
    memcpy(&pBufferBegin[nElementIndex * nElementByteSize], pData, nByteSize);
}