#pragma once
#ifndef _D3DHELPER_RESOURCE_H
#define _D3DHELPER_RESOURCE_H
#include "D3DBase.h"

namespace D3DHelper
{
	namespace Resource
	{
		struct Texture
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploader;

            std::string Name;
            std::wstring FileName;
        };
		
        /// @brief 渲染数据描述结构体
        struct SubmeshGeometry
        {
            UINT nIndexCount;						// 顶点索引数量
            UINT nStartIndexLocation;				// 顶点索引基值
            UINT nBaseVertexLocation;				// 顶点基值

            DirectX::BoundingBox Bounds;			// 几何体碰撞盒数据
        };

        /// @brief 渲染数据结构体
        struct MeshGeometry
        {
            std::string Name;

            Microsoft::WRL::ComPtr<ID3DBlob> pCPUVertexBuffer; // 顶点数据副本
            Microsoft::WRL::ComPtr<ID3DBlob> pCPUIndexBuffer;  // 索引数据副本

            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUVertexBuffer;      // 顶点数据默认堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pGPUIndexBuffer;       // 索引数据默认堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderVertexBuffer; // 顶点数据上传堆
            Microsoft::WRL::ComPtr<ID3D12Resource> pUploaderIndexBuffer;  // 索引数据上传堆

            UINT nVertexByteStride;                           // 顶点属性步长
            UINT nVertexBufferByteSize;                       // 顶点数据字节数
            UINT nIndexBufferByteSize;                        // 索引数据字节数
            DXGI_FORMAT emIndexFormat = DXGI_FORMAT_R16_UINT; // 索引数据类型

            std::unordered_map<std::string, SubmeshGeometry> DrawArgs; // 几何体列表

            /// @brief 获取顶点缓冲视图
            /// @return
            D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
            {
                D3D12_VERTEX_BUFFER_VIEW view;
                view.BufferLocation = pGPUVertexBuffer->GetGPUVirtualAddress();
                view.SizeInBytes = nVertexBufferByteSize;
                view.StrideInBytes = nVertexByteStride;
                return view;
            }

            /// @brief 获取索引缓冲视图
            /// @return
            D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
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
		
	};
};

#endif