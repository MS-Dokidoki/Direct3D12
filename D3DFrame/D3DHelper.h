/******************************************************/
/*         该文件包含了用于 Direc3D 开发的工具          */
/******************************************************/

#pragma once
#ifndef _D3DHELPER_H
#define _D3DHELPER_H
#include "GameTimer.h"
#include "BaseHelper.h"
#include "D3DHelper_Exception.h"
#include "D3DHelper_GeometryGenerator.h"
#include "D3DHelper_Constant.h"
#include "D3DHelper_FrameResource.h"
#include "D3DHelper_Light.h"
#include "D3DHelper_Resource.h"
#include "D3DHelper_Math.h"
#include "D3DHelper_Animation.h"

namespace D3DHelper
{
    /// @brief 编译 HLSL 着色器
    /// @param lpszShaderFile 磁盘中文件名
    /// @param lpszDefines     宏定义字符串
    /// @param lpszEntryPoint  着色器入口
    /// @param lpszTarget      着色器版本
    /// @return                着色器二进制代码
    Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(LPCWSTR lpszShaderFile, const D3D_SHADER_MACRO *lpszDefines, const char *lpszEntryPoint, const char *lpszTarget);

    /// @brief 创建默认堆辅助函数
    /// @param pDevice      D3D12 设备
    /// @param pComList     命令列表
    /// @param pData        上传的数据
    /// @param nByteSize    数据的字节数
    /// @param pUploader    未初始化的上传堆
    /// @return             默认堆
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device *pDevice, ID3D12GraphicsCommandList *pComList, const void *pData, UINT nByteSize, ID3D12Resource **pUploader);
	
	/// @brief 加载 DDS 纹理
	/// @param pDevice 		D3D12 设备
	/// @param pComList     命令列表
	/// @param lpszFileName 磁盘文件名
	/// @param pUploader    未初始化的上传堆
	/// @return 			返回纹理资源
    Microsoft::WRL::ComPtr<ID3D12Resource> LoadDDSFromFile(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, LPCWSTR lpszFileName, ID3D12Resource** pUploader);
};

namespace D3DHelper
{
	// 渲染项实例数据
	struct InstanceData
	{
		DirectX::XMFLOAT4X4 matWorld;
		DirectX::XMFLOAT4X4 matTexTransform;
		UINT nMaterialIndex;
	};

	// 蒙皮实例数据
	// 当一个模型被多个实例使用时, 可以为不同实例创建对应的蒙皮实例数据
	struct SkinnedInstance
	{	
		Animation::SkinnedAnimation* Skinned;			// 所使用的蒙皮动画
		std::vector<DirectX::XMFLOAT4X4> matFinalTransforms;	// 存储着 UpdateSkinnedAnimation 执行后的结果 
		std::string ClipName;									// 动画片段名; 可以通过更改该值以达到切换动画片段的效果
		float fTimePos;
		
		void UpdateAnimation(float t)
		{
			assert(Skinned);

			fTimePos += t;
			if(fTimePos > Skinned->GetClipEndTime(ClipName))
				fTimePos = 0;
			
			Skinned->GetFinalTransforms(ClipName, fTimePos, matFinalTransforms);
		}
	};
	/// @brief 渲染项描述结构体
	struct RenderItem
	{
		RenderItem() = default;

		int iFramesDirty = 3;							// 脏标识 默认为后台缓冲区的数量; 重置时应当 = nSwapChainBufferCount
        bool bVisible = 1;								// 渲染项是否可见 若该值为 0, 则渲染项不可见(不进行渲染操作)

        D3D12_PRIMITIVE_TOPOLOGY emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT nIndexCount = 0; 					        // 顶点索引的数据
		UINT nStartIndexLocation = 0; 					// 索引位置起始值
		UINT nBaseVertexLocation = 0;  					// 顶点位置基值
        DirectX::BoundingBox Bounds;

// 下列字段需要通过 D3D12_SKINNED 宏来启用
#ifdef D3D12_SKINNED
		UINT nSkinnedCBIndex = -1;						// 蒙皮数据的常量缓冲区
		SkinnedInstance* Skinned = NULL;
#endif
// 下列字段需要通过 D3D12_INSTANCE 宏来切换
#ifdef D3D12_INSTANCE
        UINT nInstanceIndex = 0;                        // GPU 的实例数据索引
		UINT nInstanceTotal = 0;						// 实例总量
        UINT nInstanceCount = 0;						// 实例的有效数量
		std::vector<InstanceData> InstanceDatas;		// 实例数据 size == nInstanceTotal
#else 
		const UINT nInstanceCount = 1;
		
		DirectX::XMFLOAT4X4 matWorld;					// 世界变换矩阵
		DirectX::XMFLOAT4X4 matTexTransform;			// 纹理变换矩阵
		
		UINT nCBObjectIndex;							// 渲染项的常量缓冲区索引
        Light::Material* pMaterial = NULL;				// 渲染项所使用的材质纹理数据
#endif
    	Resource::MeshGeometry *pGeo = NULL;			// 渲染项所使用的网格几何体包		
    };
};
#endif