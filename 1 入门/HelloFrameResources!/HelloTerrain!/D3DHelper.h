/**
 * ��ͷ�ļ�������һЩ������ DirectX 3D �����Ĺ���
 * ��Ҫ��Ϊ���²���:
 * * ͨ�ò���(D3DHelper)
 * * ��Ⱦ�ò���(D3DHelper::Render)
 * * �����岿��(D3DHelper::Geometry)
 * ...
 * 
 * ��������:
 * ������������������ͬʱ���ݱ��������������ͽ���������ǰ׺���й滮��
 * 
 * - ��������������:
 * * һ�������ָ�������ڵ���ʱ����
 * * ���������ָ�ṹ������еĳ�Ա������ͬʱ��������ʽ������ʵ�ʲ���Ҳ����ʹ�øù淶
 * 
 * - ����ǰ׺:
 * ** һ�����ǰ׺
 * * f:     float ������
 * * i:     int ����
 * * b:     byte �ֽ�
 * * p(lp): (long) pointer (��)ָ�룬Ҳ���Դ������� 
 * 
 * ** �������ǰ׺
 * * n:     number ��������, �����״θ�ֵ���䱣��ֵ��֮���������
 * * c:     count ����
 * * i:     int   ����, ��ֵ�ڳ��������в��ϸı�
 * * ix:    index ����
 * * em:    enum ö��
 * * p(lp): (long) pointer (��)ָ��, Ҳ���Դ�������
 * * vecX:  vector ����, X ����ά��
 * * mat:   matrix ����
 * 
 * ** ���ڳ�Ա�ṹ��������Ա�����:
 * ��������ṹ���������ý���������
 * 
 * - �������:
 * ** ������
 * ���� ��ǰ׺-������-������... ����ʽ��������, �������ж�Ϊ��д��ĸ 
 * ��: DXEXCEPTION_MAXSTRING ��ǰ׺Ϊ DXEXCEPTION, �����ú���Ϊ DxException �����ģ�����Ϊ MaxString, �����ú���һ���쳣���ַ���󻺳������ֽ�����
 * 
 * ** ����
 * ���ݺ��������ý�������������ÿ�����ʵ�����ĸ��д��
 * ��������֮���ơ�
 * ��: CreateDefaultBuffer ����, Create �����ú���Ϊ����/���ɺ����������ɶ���Ϊ Direct3D �е� Default Buffer(Ĭ�϶�)
 * 
 * 
 * 
*/

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
    /// @param nByteSize    ���ݵ��ֽ���
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
        void CopyData(int nElementIndex, const void* pData, const UINT nByteSize);
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
            UINT nStartIndexLocation;
            INT nBaseVertexLocation;

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
            D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
            {
                D3D12_VERTEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUVertexBuffer->GetGPUVirtualAddress();
                view.SizeInBytes = nVertexBufferByteSize;
                view.StrideInBytes = nVertexByteStride;
                return view;
            }

            /// @brief ��ȡ����������ͼ
            /// @return
            D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
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
            DirectX::XMFLOAT4X4 matWorld;

            // ʹ�����ʶ(Dirty Flag)����ʾ������������Ѿ������仯
            // ����ÿ��֡��Դ����һ������������ �������Ǳ����ÿһ��֡��Դ�е����ݽ��и���
            // ���޸ı�ʶʱ��Ӧ��ʹ�ñ�ʶΪ��ǰ�����֡��Դ�������� iFrameDirty = ֡��Դ����
            // ��ȷ��ÿһ��֡��Դ�еĳ������ݵõ�����
            int iFramesDirty;

            // �����ݱ�ʶ��ǰ��Ⱦ���Ӧ��֡��Դ�ĳ�������������
            UINT nCBObjectIndex;

            // ����Ⱦ�������Ƶļ����塣һ������������ж����Ⱦ��
            MeshGeometry* pGeo;

            // ͼԪ��������
            D3D12_PRIMITIVE_TOPOLOGY emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            // ��Ⱦ�ò���
            UINT nIndexCount;                // ��������������
            UINT nStartIndexLocation;   // ����λ����ʼֵ
            int nBaseVertexLocation;    // ����λ�û�ֵ
        };

        /// @brief ֡��Դ�ṹ��
        struct FrameResource
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;   // ���������
            D3DHelper::UploadBuffer CBScene;            // ��������
            D3DHelper::UploadBuffer CBObject;           // ��������
            D3DHelper::UploadBuffer CBVertex;           // ��������
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

            /// @brief ��ʼ��֡��Դ�Ķ�̬���㻺����
            /// @param pDevice D3D12 �豸ָ��
            /// @param nBytesSize ��̬���㻺�������ݿ��С
            /// @param nCount ��̬���㻺�������ݿ�����
            void InitDynamicVertexBuffer(ID3D12Device* pDevice, UINT nBytesSize, UINT nCount)
            {
                CBVertex.Init(pDevice, nBytesSize, nCount);
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
            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ�����
            /// @param bottomRadius ����뾶
            /// @param topRadius    ����뾶
            /// @param height       �߶�
            /// @param nSliceCount  ��Ƭ����
            /// @param nStackCount  �ѵ�����
            /// @return 
            static Mesh CreateCylinder(float bottomRadius, float topRadius, float height, UINT nSliceCount, UINT nStackCount);

            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ�����
            /// @param radius       ����뾶
            /// @param nSliceCount  ��Ƭ����
            /// @param nStackCount  �ѵ�����
            /// @return 
            static Mesh CreateSphere(float radius, UINT nSliceCount, UINT nStackCount);

            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ� x-z դ��ƽ��
            /// @param fHorizonalLength ���򳤶�( x �᷽��)
            /// @param fVerticalLength ���򳤶�( z �᷽��)
            /// @param nHoriGridCount ����դ������
            /// @param nVertGridCount ����դ������
            /// @return 
            static Mesh CreateGrid(float fHorizonalLength, float fVerticalLength, UINT nHoriGridCount, UINT nVertGridCount);
        };
    };

    
};

#endif