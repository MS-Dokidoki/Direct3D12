#pragma once
#ifndef _D3DHELPER_FRAMERESOURCE_H
#define _D3DHELPER_FRAMERESOURCE_H
#include "D3DBase.h"
#include "D3DHelper_UploadBuffer.h"

namespace D3DHelper
{
	namespace Resource
	{
        /// @brief 帧资源结构体
        struct FrameResource
        {
			enum FrameResourceTypes		// 帧资源类型
			{
				FRAME_RESOURCE_TYPE_VERTEX = 1 << 0,			// 顶点帧资源
				FRAME_RESOURCE_TYPE_MATERIAL = 1 << 1,			// 材质帧资源
                FRAME_RESOURCE_TYPE_SSAO = 1 << 2,              // Ssao 帧资源
                FRAME_RESOURCE_TYPE_SKINNED = 1 << 3            // 蒙皮帧资源
			};
            typedef UINT FRAME_RESOURCE_TYPES;

            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator; // 命令分配器
            D3DHelper::UploadBuffer CBScene;                                  // 场景数据
            D3DHelper::UploadBuffer CBObject;                                 // 对象数据
            D3DHelper::UploadBuffer CBVertex;                                 // 顶点数据
			D3DHelper::UploadBuffer CBMaterial;								  // 材质数据
			std::vector<D3DHelper::UploadBuffer> CBInstances;				  // 实例数据
            UINT iFence;

            std::unordered_map<FRAME_RESOURCE_TYPES, D3DHelper::UploadBuffer> CBOthers; 
            FrameResource() : iFence(0){}

            /// @brief 初始化帧资源的常量缓冲区
            /// @param pDevice D3D12 设备指针
            /// @param nSceneCount 场景常量数据块的数量
            /// @param nObjectCount 对象常量数据块的数量
            void InitConstantBuffer(ID3D12Device *pDevice, UINT nSceneCount, UINT nObjectCount)
            {
                CBScene.Init(pDevice, CONSTANT_VALUE::nCBSceneByteSize, nSceneCount);
                if(nObjectCount)
                    CBObject.Init(pDevice, CONSTANT_VALUE::nCBObjectByteSize, nObjectCount);
            }
			
			D3DHelper::UploadBuffer& InitOtherBuffer(FRAME_RESOURCE_TYPES type)
            {
                switch(type)
                {
                case FRAME_RESOURCE_TYPE_VERTEX:
                    return CBVertex;
                case FRAME_RESOURCE_TYPE_MATERIAL:
                    return CBMaterial;
                default:
                    return CBOthers[type];
                }
            }

            UINT InitInstanceBuffer(ID3D12Device* pDevice, UINT nTypeBytesSize, UINT nInstanceCount)
            {
                CBInstances.push_back(UploadBuffer(pDevice, nTypeBytesSize, nInstanceCount));
                return CBInstances.size() - 1;
            }
        };
	};
};

#endif