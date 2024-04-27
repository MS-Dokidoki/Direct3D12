#pragma once
#ifndef _D3DHELPER_LIGHT_H
#define _D3DHELPER_LIGHT_H
#include "D3DBase.h"

#define MAX_NUM_LIGHT 16
namespace D3DHelper
{
	namespace Light
	{
		/// @brief 材质结构体
		struct Material
		{
			std::string Name;						// 材质名
			int iFramesDirty = 3;					// 数据脏标识
			int nCBMaterialIndex;					// 本材质信息在常量缓冲区的索引
			int nDiffuseSrvHeapIndex;				// 漫反射纹理在 SRV 堆中的索引
            int nNormalSrvHeapIndex;
			
			// 用于着色材质的常量缓冲区数据
			DirectX::XMFLOAT4 vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};	// 漫反射反照率
			DirectX::XMFLOAT3 vec3FresnelR0 = {0.01f, 0.01f, 0.01f};		// 材质属性R(0°)
			float nRoughness = 64.0f;										// 表面粗糙度
			DirectX::XMFLOAT4X4 matTransform = MATRIX_IDENTITY;
		};
		
		/// @brief 光源常量数据
		struct Light
		{
			DirectX::XMFLOAT3 vec3Strength;			// 光源颜色
			float nFalloffStart;					// 光源线性衰减起始范围(点光源/聚光灯光源)
			DirectX::XMFLOAT3 vec3Direction;		// 光源方向(方向光源/聚光灯光源)
			float nFalloffEnd;						// 光源线性衰减结束范围(点光源/聚光灯光源)
			DirectX::XMFLOAT3 vec3Position;			// 光源位置(点光源/聚光灯光源)
			float nSpotPower;						// 聚光灯因子
		};
	};
};

#endif