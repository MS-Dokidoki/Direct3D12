#pragma once
#ifndef _D3DHELPER_H
#define _D3DHELPER_H

#include "D3DBase.h"
#define DXEXCEPTION_MAXSTRING 256
#define DXEXCEPTION_MAXSTRINGEX 512
#include <string>
#include <unordered_map>
#include <vector>

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

/*************************/
//* c: count; p(lp): pointer; n: number; 
//* f: float; i: int; ui: unsigned int;

namespace D3DHelper
{
/**** ͨ�ò��� */

    /// @brief ���� HLSL ��ɫ��
    /// @param lpszShaderFile �������ļ�����
    /// @param lpszDefines     �궨���ַ���
    /// @param lpszEntryPoint  ��ɫ����ڵ�
    /// @param lpszTarget      ��ɫ���汾
    /// @return             ��ɫ�������ƴ���
    Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPCWSTR lpszShaderFile, const D3D_SHADER_MACRO* lpszDefines, const char* lpszEntryPoint, const char* lpszTarget);
    
    /// @brief ����Ĭ�϶Ѹ�������
    /// @param pDevice      D3D12 �豸
    /// @param pComList     �����б�
    /// @param pData        �ϴ�������
    /// @param uiByteSize   ���ݵ��ֽ���
    /// @param pUploader    δ��ʼ�����ϴ��� 
    /// @return             Ĭ�϶�
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, const void* pData, UINT nByteSize, ID3D12Resource** pUploader);

    /// @brief �ϴ��Ѹ�����
    class UploadBuffer
    {
    public:
        UploadBuffer();

        UploadBuffer(ID3D12Device* pDevice, UINT nElementByteSize, UINT nElementCount);
        UploadBuffer(const UploadBuffer&) = delete;
        UploadBuffer& operator=(const UploadBuffer&) = delete;
        ~UploadBuffer();

        void Init(ID3D12Device* pDevice, UINT nElementByteSize, UINT nElementCount);
        ID3D12Resource* Resource() const;
        void CopyData(int iElementIndex, const void* pData, const UINT nByteSize);
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer;
        BYTE* pBufferBegin = NULL;
        UINT nElementByteSize;
    };
    
/******* ��Ⱦ�ò���*/
    namespace Render
    {
        /// @brief ��Ⱦ���������ṹ��
        struct SubmeshGeometry
        {
            UINT nIndexCount;
            UINT iStartIndexLocation;
            INT iBaseVertexLocation;

            DirectX::BoundingBox Bounds;
        };

        /// @brief ��Ⱦ���ݽṹ��
        struct MeshGeometry
        {
            std::string Name;

            Microsoft::WRL::ComPtr<ID3DBlob> pCPUVertexBuffer;              // �������ݸ���
            Microsoft::WRL::ComPtr<ID3DBlob> pCPUIndexBuffer;               // �������ݸ���

            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUVertexBuffer;        // ��������Ĭ�϶�
            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUIndexBuffer;         // ��������Ĭ�϶�
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderVertexBuffer;   // ���������ϴ���
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderIndexBuffer;    // ���������ϴ���

            UINT nVertexByteStride;                            // �������Բ���
            UINT nVertexBufferByteSize;                        // ���������ֽ���
            UINT nIndexBufferByteSize;                         // ���������ֽ���
            DXGI_FORMAT emIndexFormat = DXGI_FORMAT_R16_UINT;  // ������������

            std::unordered_map<std::string, SubmeshGeometry> DrawArgs;      // ���ƶ���

            /// @brief ��ȡ���㻺����ͼ
            /// @return
            D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
            {
                D3D12_VERTEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUVertexBuffer->GetGPUVirtualAddress();
                view.SizeInBytes = nVertexBufferByteSize;
                view.StrideInBytes = nVertexByteStride;
                return view;
            }

            /// @brief ��ȡ����������ͼ
            /// @return
            D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
            {
                D3D12_INDEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUIndexBuffer->GetGPUVirtualAddress();
                view.Format = emIndexFormat;
                view.SizeInBytes = nIndexBufferByteSize;
                return view;
            }

            /// @brief �ͷ��ϴ��ѡ��ϴ�������Ĭ�϶Ѻ󼴿ɵ���
            void DisposeUploader()
            {
                pUploaderVertexBuffer = nullptr;
                pUploaderIndexBuffer = nullptr;
            }
        };

        /// @brief ������������
        struct SceneConstant
        {
            DirectX::XMFLOAT4X4 matView;
            DirectX::XMFLOAT4X4 matInvView;
            DirectX::XMFLOAT4X4 matProj;
            DirectX::XMFLOAT4X4 matInvProj;
            DirectX::XMFLOAT4X4 matViewProj;
            DirectX::XMFLOAT4X4 matInvViewProj;
            DirectX::XMFLOAT3 vec3EyePos;
            DirectX::XMFLOAT2 vec2RenderTargetSize;
            DirectX::XMFLOAT2 vec2InvRenderTargetSize;
            float fPerObjectPad1;
            float fNearZ;
            float fFarZ;
            float fTotalTime;
            float fDeltaTime;
            /*
            cbuffer Scene: register(b1)
            {
                float4x4 matView;
                float4x4 matInvView;
                float4x4 matProj;
                float4x4 matInvProj;
                float4x4 matViewProj;
                float4x4 matInvViewProj;
                float3 vec3EyePos;
                float2 vec2RenderTargetSize;
                float2 vec2InvRenderTargetSize;
                float fPerObjectPad1;
                float fNearZ;
                float fFarZ;
                float fTotalTime;
                float fDeltaTime;
            };
            */
        };

        /// @brief ����������
        struct ObjectConstant
        {
            DirectX::XMFLOAT4X4 matWorld;
        };

        /// @brief ��Ⱦ�������ṹ��
        struct RenderItem
        {
            // ������Ⱦ����ֲ��ռ����������ռ�ľ���
            // �������˶����������ռ��еĳ���λ�úʹ�С
            DirectX::XMFLOAT4X4 World;

            // ʹ�����ʶ(Dirty Flag)����ʾ������������Ѿ������仯
            // ����ÿ��֡��Դ����һ������������ �������Ǳ����ÿһ��֡��Դ�е����ݽ��и���
            // ���޸ı�ʶʱ��Ӧ��ʹ�ñ�ʶΪ��ǰ�����֡��Դ�������� iFrameDirty = ֡��Դ����
            // ��ȷ��ÿһ��֡��Դ�еĳ������ݵõ�����
            int iFramesDirty;

            // �����ݱ�ʶ��ǰ��Ⱦ���Ӧ��֡��Դ�ĳ�������������
            UINT iCBObjectIndex;

            // ����Ⱦ�������Ƶļ����塣һ������������ж����Ⱦ��
            MeshGeometry* pGeo;

            // ͼԪ��������
            D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            // ��Ⱦ�ò���
            UINT cIndex;                // ��������������
            UINT iStartIndexLocation;   // ����λ����ʼֵ
            int iBaseVertexLocation;    // ����λ�û�ֵ
        };

        /// @brief ֡��Դ�ṹ��
        struct FrameResource
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;   // ���������
            D3DHelper::UploadBuffer CBScene;            // ��������
            D3DHelper::UploadBuffer CBObject;           // ��������
            UINT iFence;
            
            FrameResource() : iFence(0)
            {}

            /// @brief ��ʼ��֡��Դ�ĳ���������
            /// @param pDevice D3D12 �豸ָ��
            /// @param nSceneCount �����������ݿ������
            /// @param nObjectCount ���������ݿ������
            void InitConstantBuffer(ID3D12Device* pDevice, UINT nSceneCount, UINT nObjectCount)
            {
                CBScene.Init(pDevice, D3DHelper_CalcConstantBufferBytesSize(sizeof(SceneConstant)), nSceneCount);
                CBObject.Init(pDevice, D3DHelper_CalcConstantBufferBytesSize(sizeof(ObjectConstant)), nObjectCount);
            }
        };

    };
    
/***** �����岿��*/
    namespace Geometry
    {
        /// @brief �����嶥�����ݽṹ��
        struct Vertex
        {
            Vertex(){}
            Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) :
            vec3Position(px, py, pz), vec3Normal(nx, ny, nz), vec3TangentU(tx, ty, tz), vec2TexCoords(u, v){}
            Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangentU, DirectX::XMFLOAT2 texCoords) :
            vec3Position(position), vec3Normal(normal), vec3TangentU(tangentU), vec2TexCoords(texCoords){}
            DirectX::XMFLOAT3 vec3Position;     // ��������
            DirectX::XMFLOAT3 vec3Normal;       // ���㷨��
            DirectX::XMFLOAT3 vec3TangentU;     // ��������
            DirectX::XMFLOAT2 vec2TexCoords;    // ��������
        };

        /// @brief �������������ݽṹ�塣��ǰ��֧�� UINT16 ������ʽ
        struct Mesh
        {
            std::vector<Vertex> vertices;
            std::vector<UINT16> indices;
        };

        /// @brief �������������ɾ�̬�� 
        class GeometryGenerator
        {
        public:
            /// @brief ����һ���Ծֲ�����ϵԭ��Ϊ���ĵ���״��������������
            /// @param rBottom  ����뾶
            /// @param rTop     ����뾶
            /// @param height   ����߶�
            /// @param cSlice   ��Ƭ����; ��ָ���뾶�£�����͵����Լ���������ΪԲ�ģ���Բ������ƽ���и������������
            /// @param cStack   �ѵ�����; ��ָ���߶��£�����������汻�����и������
            /// @return ��������
            static Mesh CreateCylinder(float rBottom, float rTop, float height, UINT cSlice, UINT cStack);

            static Mesh CreateCube();
        };
    };

    
};

#endif