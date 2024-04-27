#pragma once
#ifndef _D3DHELPER_UPLOADBUFFER_H
#define _D3DHELPER_UPLOADBUFFER_H
#include "D3DBase.h"

namespace D3DHelper
{
/// @brief 上传堆辅助类
    class UploadBuffer
    {
    public:
        UploadBuffer();

        UploadBuffer(ID3D12Device *Device, UINT ElementByteSize, UINT ElementCount);
        UploadBuffer &operator=(const UploadBuffer &) = delete;
        ~UploadBuffer();
		
        void Init(ID3D12Device* Device, UINT ByteSize, UINT Count, D3D12_RESOURCE_DESC* ResDesc, D3D12_RESOURCE_STATES ResInitState);
        void Init(ID3D12Device *Device, UINT ElementByteSize, UINT ElementCount);
        ID3D12Resource *Resource() const;
        void CopyData(int ElementIndex, const void *Data, const UINT ByteSize);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;
        BYTE *pBufferBegin = NULL;
        UINT nElementByteSize;
    };
};

#endif