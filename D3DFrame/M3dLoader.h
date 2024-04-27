#pragma once
#ifndef _M3DLOADER_H
#define _M3DLOADER_H
#include "BaseHelper.h"
#include "D3DBase.h"
#include "string"
#include "D3DHelper_Animation.h"

namespace D3DHelper
{
	namespace M3dLoader
	{
		static UINT M3dFileCodePage = CP_UTF8;				// M3d 文件的默认编码格式

		// M3d 材质类型
		enum M3dMaterialTypes
		{
			M3DMATERIAL_TYPE_SKINNED,				// 当该类型被选中时, 说明材质需要在蒙皮着色器中才能使用
			M3DMATERIAL_TYPE_NONE
		};
		
		struct M3dMaterial
		{
			std::string Name;						// 材质名
			DirectX::XMFLOAT3 vec3DiffuseAlbedo;	// 材质漫反射率
			DirectX::XMFLOAT3 vec3FresnelR0;		// 材质的菲涅尔折射系数
			float nRoughness;						// 材质的粗糙度
			bool bAlphaClip;						// 材质是否启用 Alpha 剔除
			std::string MaterialType;			// 材质类型
			std::wstring DiffuseMap;				// 漫反射纹理
			std::wstring NormalMap;					// 法线纹理
		};
		
		struct M3dSubset
		{
			UINT nSubsetID;			// 子集 ID, 一般对应于材质索引
			UINT nVertexStart;		
			UINT nVertexCount;		
			UINT nFaceStart;		// 子集的三角形片面起始值 = 子集的索引起始值 / 3
			UINT nFaceCount;		// 子集的三角形片面数量 = 子集的索引数量 / 3
		};
		
		struct M3dSkinnedVertex
		{
			DirectX::XMFLOAT3 vec3Position;		// 顶点位置向量
			DirectX::XMFLOAT3 vec3TangentU;		// 顶点切向量
			DirectX::XMFLOAT3 vec3Normal;		// 顶点法向量
			DirectX::XMFLOAT2 vec2TexCoords;	// 顶点纹理坐标
			DirectX::XMFLOAT4 vec4BoneWeights;  // 顶点骨骼渲染权重(x + y + z + w == 1)
			UINT vec4BoneIndices[4];
		};
		
		struct M3dVertex
		{
			DirectX::XMFLOAT3 vec3Position;
			DirectX::XMFLOAT4 vec4TangentU;
			DirectX::XMFLOAT3 vec3Normal;
			DirectX::XMFLOAT2 vec2TexCoords;
		};

		void ReadMaterials(BaseHelper::ScannerA&, UINT numMaterial, std::vector<M3dMaterial>& materials);
		void ReadVertices(BaseHelper::ScannerA&, UINT numVertex, std::vector<M3dVertex>& vertices);
		void ReadSkinnedVertices(BaseHelper::ScannerA&, UINT numVertex, std::vector<M3dSkinnedVertex>& vertices);
		void ReadSubsets(BaseHelper::ScannerA&, UINT numSubset, std::vector<M3dSubset>& subsets);
		void ReadIndices(BaseHelper::ScannerA&, UINT numIndex, std::vector<UINT>& indices);
		void ReadAnimations(BaseHelper::ScannerA&, UINT numBone, UINT numAnimationClip, Animation::SkinnedAnimation& animation);
		
		bool LoadM3dFile(LPCWSTR file, 
						 std::vector<M3dVertex>& vertices,
						 std::vector<UINT>& indices,
						 std::vector<M3dSubset>& subsets,
						 std::vector<M3dMaterial>& materials);
		
		bool LoadM3dFile(LPCWSTR file,
						 std::vector<M3dSkinnedVertex>& vertices,
						 std::vector<UINT>& indices,
						 std::vector<M3dSubset>& subsets,
						 std::vector<M3dMaterial>& materials,
						 D3DHelper::Animation::SkinnedAnimation& animation);
	};
};

#endif