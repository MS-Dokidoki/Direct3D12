#ifndef _D3DHELPER_H
#define _D3DHELPER_H
#include "D3DBase.h"
#define DXEXCEPTION_MAXSTRING 256
#define DXEXCEPTION_MAXSTRINGEX 512
#include <string>
#include <unordered_map>

class DXException
{
public:
    DXException(HRESULT);
    DXException(HRESULT, const char*, unsigned int);
    DXException(HRESULT, ID3DBlob**, const char*, unsigned int);
    ~DXException();

    TCHAR* ToString();
private:
    TCHAR e[DXEXCEPTION_MAXSTRINGEX];
};

#define ThrowIfFailed(hr) if(FAILED(hr)){throw DXException(hr, __FILE__, __LINE__);}
#define ThrowIfFailedEx(hr, error) if(FAILED(hr)){throw DXException(hr, error, __FILE__, __LINE__);}

#define D3DHelper_CalcConstantBufferBytesSize(byteSize) ((byteSize + 255)& ~255)

namespace D3DHelper
{
    UINT CalcConstantBufferBytesSize(UINT bytesSize);
    
    Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPCWSTR lpShaderFile, const D3D_SHADER_MACRO* pDefines, const char* pEntryPoint, const char* pTarget);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, const void* pData, UINT uiByteSize, ID3D12Resource** pUploader);

    struct SubmeshGeometry
    {
        UINT uiIndexCount;
        UINT uiStartIndexLocation;
        INT iBaseVertexLocation;

        DirectX::BoundingBox Bounds;
    };

    struct MeshGeometry
    {
        std::string Name;

        Microsoft::WRL::ComPtr<ID3DBlob> pCPUVertexBuffer;
        Microsoft::WRL::ComPtr<ID3DBlob> pCPUIndexBuffer;

        Microsoft::WRL::ComPtr<ID3D12Resource> pGPUVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> pGPUIndexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderIndexBuffer;

        UINT uiVertexByteStride;
        UINT uiVertexBufferByteSize;
        UINT uiIndexBufferByteSize;
        DXGI_FORMAT emIndexFormat = DXGI_FORMAT_R16_UINT;

        std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

        D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
        {
            D3D12_VERTEX_BUFFER_VIEW view;
            view.BufferLocation = pGPUVertexBuffer->GetGPUVirtualAddress();
            view.SizeInBytes = uiVertexBufferByteSize;
            view.StrideInBytes = uiVertexByteStride;
            return view;
        }

        D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
        {
            D3D12_INDEX_BUFFER_VIEW view;
            view.BufferLocation = pGPUIndexBuffer->GetGPUVirtualAddress();
            view.Format = emIndexFormat;
            view.SizeInBytes = uiIndexBufferByteSize;
            return view;
        }

        void DisposeUploader()
        {
            pUploaderVertexBuffer = nullptr;
            pUploaderIndexBuffer = nullptr;
        }
    };

    class UploadBuffer
    {
    public:
        UploadBuffer();
        UploadBuffer(ID3D12Device* pDevice, UINT uiElementByteSize, UINT uiElementCount);
        UploadBuffer(const UploadBuffer&) = delete;
        UploadBuffer& operator=(const UploadBuffer&) = delete;
        ~UploadBuffer();

        void Init(ID3D12Device* pDevice, UINT uiElementByteSize, UINT uiElementCount);
        ID3D12Resource* Resource() const;
        void CopyData(int iElementIndex, const void* pData, const UINT uiByteSize);
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;
        BYTE* pBufferBegin = NULL;
        UINT uiElementByteSize;
    };
};

#endif