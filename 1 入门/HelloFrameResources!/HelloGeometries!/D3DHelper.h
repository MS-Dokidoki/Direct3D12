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
/**** 通用部分 */

    /// @brief 编译 HLSL 着色器
    /// @param lpszShaderFile 磁盘中文件名称
    /// @param lpszDefines     宏定义字符串
    /// @param lpszEntryPoint  着色器入口点
    /// @param lpszTarget      着色器版本
    /// @return             着色器二进制代码
    Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPCWSTR lpszShaderFile, const D3D_SHADER_MACRO* lpszDefines, const char* lpszEntryPoint, const char* lpszTarget);
    
    /// @brief 创建默认堆辅助函数
    /// @param pDevice      D3D12 设备
    /// @param pComList     命令列表
    /// @param pData        上传的数据
    /// @param uiByteSize   数据的字节数
    /// @param pUploader    未初始化的上传堆 
    /// @return             默认堆
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, const void* pData, UINT nByteSize, ID3D12Resource** pUploader);

    /// @brief 上传堆辅助类
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
    
/******* 渲染用部分*/
    namespace Render
    {
        /// @brief 渲染数据描述结构体
        struct SubmeshGeometry
        {
            UINT nIndexCount;
            UINT iStartIndexLocation;
            INT iBaseVertexLocation;

            DirectX::BoundingBox Bounds;
        };

        /// @brief 渲染数据结构体
        struct MeshGeometry
        {
            std::string Name;

            Microsoft::WRL::ComPtr<ID3DBlob> pCPUVertexBuffer;              // 顶点数据副本
            Microsoft::WRL::ComPtr<ID3DBlob> pCPUIndexBuffer;               // 索引数据副本

            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUVertexBuffer;        // 顶点数据默认堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUIndexBuffer;         // 索引数据默认堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderVertexBuffer;   // 顶点数据上传堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderIndexBuffer;    // 索引数据上传堆

            UINT nVertexByteStride;                            // 顶点属性步长
            UINT nVertexBufferByteSize;                        // 顶点数据字节数
            UINT nIndexBufferByteSize;                         // 索引数据字节数
            DXGI_FORMAT emIndexFormat = DXGI_FORMAT_R16_UINT;  // 索引数据类型

            std::unordered_map<std::string, SubmeshGeometry> DrawArgs;      // 绘制对象

            /// @brief 获取顶点缓冲视图
            /// @return
            D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
            {
                D3D12_VERTEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUVertexBuffer->GetGPUVirtualAddress();
                view.SizeInBytes = nVertexBufferByteSize;
                view.StrideInBytes = nVertexByteStride;
                return view;
            }

            /// @brief 获取索引缓冲视图
            /// @return
            D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
            {
                D3D12_INDEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUIndexBuffer->GetGPUVirtualAddress();
                view.Format = emIndexFormat;
                view.SizeInBytes = nIndexBufferByteSize;
                return view;
            }

            /// @brief 释放上传堆。上传数据至默认堆后即可调用
            void DisposeUploader()
            {
                pUploaderVertexBuffer = nullptr;
                pUploaderIndexBuffer = nullptr;
            }
        };

        /// @brief 场景常量数据
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

        /// @brief 对象常量数据
        struct ObjectConstant
        {
            DirectX::XMFLOAT4X4 matWorld;
        };

        /// @brief 渲染项描述结构体
        struct RenderItem
        {
            // 描述渲染对象局部空间相对于世界空间的矩阵
            // 它包含了对象的在世界空间中的朝向、位置和大小
            DirectX::XMFLOAT4X4 World;

            // 使用脏标识(Dirty Flag)来表示物体相关数据已经发生变化
            // 由于每个帧资源都有一个常量缓冲区 所以我们必须对每一个帧资源中的数据进行更新
            // 当修改标识时，应当使该标识为当前程序的帧资源数量，即 iFrameDirty = 帧资源数量
            // 以确保每一个帧资源中的常量数据得到更新
            int iFramesDirty;

            // 该数据标识当前渲染项对应于帧资源的常量缓冲区索引
            UINT iCBObjectIndex;

            // 此渲染项参与绘制的几何体。一个几何体可以有多个渲染项
            MeshGeometry* pGeo;

            // 图元拓扑类型
            D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            // 渲染用参数
            UINT cIndex;                // 顶点索引的数量
            UINT iStartIndexLocation;   // 索引位置起始值
            int iBaseVertexLocation;    // 顶点位置基值
        };

        /// @brief 帧资源结构体
        struct FrameResource
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;   // 命令分配器
            D3DHelper::UploadBuffer CBScene;            // 场景数据
            D3DHelper::UploadBuffer CBObject;           // 对象数据
            UINT iFence;
            
            FrameResource() : iFence(0)
            {}

            /// @brief 初始化帧资源的常量缓冲区
            /// @param pDevice D3D12 设备指针
            /// @param nSceneCount 场景常量数据块的数量
            /// @param nObjectCount 对象常量数据块的数量
            void InitConstantBuffer(ID3D12Device* pDevice, UINT nSceneCount, UINT nObjectCount)
            {
                CBScene.Init(pDevice, D3DHelper_CalcConstantBufferBytesSize(sizeof(SceneConstant)), nSceneCount);
                CBObject.Init(pDevice, D3DHelper_CalcConstantBufferBytesSize(sizeof(ObjectConstant)), nObjectCount);
            }
        };

    };
    
/***** 几何体部分*/
    namespace Geometry
    {
        /// @brief 几何体顶点数据结构体
        struct Vertex
        {
            Vertex(){}
            Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) :
            vec3Position(px, py, pz), vec3Normal(nx, ny, nz), vec3TangentU(tx, ty, tz), vec2TexCoords(u, v){}
            Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangentU, DirectX::XMFLOAT2 texCoords) :
            vec3Position(position), vec3Normal(normal), vec3TangentU(tangentU), vec2TexCoords(texCoords){}
            DirectX::XMFLOAT3 vec3Position;     // 顶点坐标
            DirectX::XMFLOAT3 vec3Normal;       // 顶点法线
            DirectX::XMFLOAT3 vec3TangentU;     // 切线向量
            DirectX::XMFLOAT2 vec2TexCoords;    // 纹理坐标
        };

        /// @brief 几何体网格数据结构体。当前仅支持 UINT16 索引格式
        struct Mesh
        {
            std::vector<Vertex> vertices;
            std::vector<UINT16> indices;
        };

        /// @brief 几何体网格生成静态类 
        class GeometryGenerator
        {
        public:
            /// @brief 生成一组以局部坐标系原点为中心的柱状几何体网格数据
            /// @param rBottom  底面半径
            /// @param rTop     顶面半径
            /// @param height   柱体高度
            /// @param cSlice   切片数量; 在指定半径下，顶面和底面以几何体中心为圆心，绕圆心纵向平均切割几何体的面的数量
            /// @param cStack   堆叠数量; 在指定高度下，控制柱体侧面被横向切割的数量
            /// @return 网格数据
            static Mesh CreateCylinder(float rBottom, float rTop, float height, UINT cSlice, UINT cStack);

            static Mesh CreateCube();
        };
    };

    
};

#endif