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
		static UINT M3dFileCodePage = CP_UTF8;				// M3d �ļ���Ĭ�ϱ����ʽ

		// M3d ��������
		enum M3dMaterialTypes
		{
			M3DMATERIAL_TYPE_SKINNED,				// �������ͱ�ѡ��ʱ, ˵��������Ҫ����Ƥ��ɫ���в���ʹ��
			M3DMATERIAL_TYPE_NONE
		};
		
		struct M3dMaterial
		{
			std::string Name;						// ������
			DirectX::XMFLOAT3 vec3DiffuseAlbedo;	// ������������
			DirectX::XMFLOAT3 vec3FresnelR0;		// ���ʵķ���������ϵ��
			float nRoughness;						// ���ʵĴֲڶ�
			bool bAlphaClip;						// �����Ƿ����� Alpha �޳�
			std::string MaterialType;			// ��������
			std::wstring DiffuseMap;				// ����������
			std::wstring NormalMap;					// ��������
		};
		
		struct M3dSubset
		{
			UINT nSubsetID;			// �Ӽ� ID, һ���Ӧ�ڲ�������
			UINT nVertexStart;		
			UINT nVertexCount;		
			UINT nFaceStart;		// �Ӽ���������Ƭ����ʼֵ = �Ӽ���������ʼֵ / 3
			UINT nFaceCount;		// �Ӽ���������Ƭ������ = �Ӽ����������� / 3
		};
		
		struct M3dSkinnedVertex
		{
			DirectX::XMFLOAT3 vec3Position;		// ����λ������
			DirectX::XMFLOAT3 vec3TangentU;		// ����������
			DirectX::XMFLOAT3 vec3Normal;		// ���㷨����
			DirectX::XMFLOAT2 vec2TexCoords;	// ������������
			DirectX::XMFLOAT4 vec4BoneWeights;  // ���������ȾȨ��(x + y + z + w == 1)
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