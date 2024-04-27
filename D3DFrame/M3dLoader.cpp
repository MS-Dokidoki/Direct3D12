#include "M3dLoader.h"

#define IsDigit(c) (c >= '0' && c <= '9')

using namespace BaseHelper;
using namespace D3DHelper;
using namespace DirectX;

void M3dLoader::ReadMaterials(ScannerA& scanner, UINT nMaterial, std::vector<M3dMaterial>& materials)
{
	M3dLoader::M3dMaterial mat;
	UINT temp;

	for(UINT i = 0; i < nMaterial; ++i)
	{
		scanner(':') >> mat.Name;
		scanner(':') >> mat.vec3DiffuseAlbedo.x >> mat.vec3DiffuseAlbedo.y >> mat.vec3DiffuseAlbedo.z;
		scanner(':') >> mat.vec3FresnelR0.x >> mat.vec3FresnelR0.y >> mat.vec3FresnelR0.z;
		scanner(':') >> mat.nRoughness;
		scanner(':') >> temp;
		mat.bAlphaClip = temp;
		scanner(':') >> mat.MaterialType;
		scanner(':') >> mat.DiffuseMap;
		scanner(':') >> mat.NormalMap;
		materials.push_back(mat);
	}
}

void M3dLoader::ReadSubsets(ScannerA& scanner, UINT nSubset, std::vector<M3dSubset>& subsets)
{
	M3dSubset subset;

	for(UINT i = 0; i < nSubset; ++i)
	{
		scanner >> subset.nSubsetID >> subset.nVertexStart >> subset.nVertexCount >> subset.nFaceStart >> subset.nFaceCount;
		subsets.push_back(subset);
	}
}

void M3dLoader::ReadVertices(ScannerA& scanner, UINT nVertex, std::vector<M3dVertex>& vertices)
{
	M3dVertex vertex;
	for(UINT i = 0; i < nVertex; ++i)
	{
		scanner >> vertex.vec3Position.x >> vertex.vec3Position.y >> vertex.vec3Position.z;
		scanner >> vertex.vec4TangentU.x >> vertex.vec4TangentU.y >> vertex.vec4TangentU.z >> vertex.vec4TangentU.w;
		scanner >> vertex.vec3Normal.x >> vertex.vec3Normal.y >> vertex.vec3Normal.z;
		scanner >> vertex.vec2TexCoords.x >> vertex.vec2TexCoords.y;
		vertices.push_back(vertex);
	}
}

void M3dLoader::ReadSkinnedVertices(ScannerA& scanner, UINT nVertex, std::vector<M3dSkinnedVertex>& vertices)
{
	M3dSkinnedVertex vertex;
	float blah;

	for(UINT i = 0; i < nVertex; ++i)
	{
		scanner >> vertex.vec3Position.x >> vertex.vec3Position.y >> vertex.vec3Position.z;
		scanner >> vertex.vec3TangentU.x >> vertex.vec3TangentU.y >> vertex.vec3TangentU.z >> blah;
		scanner >> vertex.vec3Normal.x >> vertex.vec3Normal.y >> vertex.vec3Normal.z;
		scanner >> vertex.vec2TexCoords.x >> vertex.vec2TexCoords.y;
		scanner >> vertex.vec4BoneWeights.x >> vertex.vec4BoneWeights.y >> vertex.vec4BoneWeights.z >> vertex.vec4BoneWeights.w;
		scanner >> vertex.vec4BoneIndices[0] >> vertex.vec4BoneIndices[1] >> vertex.vec4BoneIndices[2] >> vertex.vec4BoneIndices[3]; 
		vertices.push_back(vertex);
	}
}

void M3dLoader::ReadIndices(ScannerA& scanner, UINT nIndex, std::vector<UINT>& indices)
{
	UINT index;
	for(UINT i = 0; i < nIndex; ++i)
	{
		scanner >> index;
		indices.push_back(index);
	}
}

void ReadBoneOffsets(ScannerA& scanner, UINT nBone, std::vector<XMFLOAT4X4>& boneOffsets)
{
	boneOffsets.resize(nBone);

	for(UINT i = 0; i < nBone; ++i)
	{
		XMFLOAT4X4& m = boneOffsets[i];
		scanner(' ') >>
		m(0, 0) >> m(0, 1) >> m(0, 2) >> m(0, 3) >>
		m(1, 0) >> m(1, 1) >> m(1, 2) >> m(1, 3) >>
		m(2, 0) >> m(2, 1) >> m(2, 2) >> m(2, 3) >>
		m(3, 0) >> m(3, 1) >> m(3, 2) >> m(3, 3);

	}

}

void ReadBoneHierarchy(ScannerA& scanner, UINT nBone, std::vector<int>& boneHierarchies)
{
	boneHierarchies.resize(nBone);
	for(UINT i = 0; i < nBone; ++i)
		scanner(':') >> boneHierarchies[i];
}

void ReadAnimationClip(ScannerA& scanner, UINT nBone, UINT nAnimationClip, std::unordered_map<std::string, Animation::AnimationClip>& clips)
{
	std::string name;
	UINT nKeyFrame;
	Animation::AnimationClip clip;
	
	clip.BoneAnimations.resize(nBone);
	for(UINT i = 0; i < nAnimationClip; ++i)
	{
		Animation::BoneAnimation bone;
		scanner(' ') >> name;

		for(UINT j = 0; j < nBone; ++j)
		{
			scanner(':') >> nKeyFrame;
			scanner('{');
			bone.Keyframes.resize(nKeyFrame);
			for(UINT k = 0; k < nKeyFrame; ++k)
			{
				Animation::Keyframe keyFrame;

				scanner >> keyFrame.nTimePos;
				scanner >> keyFrame.vec3Translation.x >> keyFrame.vec3Translation.y >> keyFrame.vec3Translation.z;
				scanner >> keyFrame.vec3Scale.x >> keyFrame.vec3Scale.y >> keyFrame.vec3Scale.z;
				scanner >> keyFrame.vec4RotationQuat.x >> keyFrame.vec4RotationQuat.y >> keyFrame.vec4RotationQuat.z >> keyFrame.vec4RotationQuat.w;

				bone.Keyframes[k] = keyFrame;
			}

			clip.BoneAnimations[j] = bone;
		}

		clips[name] = clip;
	}
}

void M3dLoader::ReadAnimations(ScannerA& scanner, UINT nBone, UINT nAnimationClip, Animation::SkinnedAnimation& animation)
{
	std::vector<XMFLOAT4X4> offsets;
	std::vector<int> hierarchies;
	std::unordered_map<std::string, Animation::AnimationClip> clips;

	ReadBoneOffsets(scanner, nBone, offsets);
	ReadBoneHierarchy(scanner, nBone, hierarchies);
	ReadAnimationClip(scanner, nBone, nAnimationClip, clips);

	animation.Set(hierarchies, offsets, clips);

}

bool M3dLoader::LoadM3dFile(LPCWSTR file, 
						 std::vector<M3dVertex>& vertices, std::vector<UINT>& indices, 
						 std::vector<M3dSubset>& subsets, std::vector<M3dMaterial>& materials)
{
	FILE_HANDLE hFile = File::OpenFile(file, File::FILE_METHOD_OPEN_EXISTING);
	ScannerA scanner;
	LPSTR pBuffer;
	
	if(hFile)
	{
		File::Read(hFile, (void**)&pBuffer, NULL);

		scanner = pBuffer;

		// Read File Header
		UINT nMaterial, nVertex, nTriangle, nBone, nClip;
		UINT* ppHeaders[] = {&nMaterial, &nVertex, &nTriangle, &nBone, &nClip};

		scanner('#');
		for(UINT i = 0; i < 5; ++i)
			scanner >> *ppHeaders[i];

		ReadMaterials(scanner, nMaterial, materials);
		ReadSubsets(scanner, nMaterial, subsets);
		ReadVertices(scanner, nVertex, vertices);
		ReadIndices(scanner, nTriangle * 3, indices);

		return 1;
	}

	return 0;
}

bool M3dLoader::LoadM3dFile(LPCWSTR file, 
						 std::vector<M3dSkinnedVertex>& vertices, std::vector<UINT>& indices, 
						 std::vector<M3dSubset>& subsets, std::vector<M3dMaterial>& materials,
						 Animation::SkinnedAnimation& animation)
{
	FILE_HANDLE hFile = File::OpenFile(file, File::FILE_METHOD_OPEN_EXISTING);
	ScannerA scanner;
	LPSTR pBuffer;

	if(hFile)
	{
		File::Read(hFile, (void**)&pBuffer, NULL);
		scanner = pBuffer;

		// Read File Header
		UINT nMaterial, nVertex, nTriangle, nBone, nClip;
		UINT* ppHeaders[] = {&nMaterial, &nVertex, &nTriangle, &nBone, &nClip};

		scanner('#');
		for(UINT i = 0; i < 5; ++i)
			scanner >> *ppHeaders[i];

		ReadMaterials(scanner, nMaterial, materials);
		ReadSubsets(scanner, nMaterial, subsets);
		ReadSkinnedVertices(scanner, nVertex, vertices);
		ReadIndices(scanner, nTriangle * 3, indices);
		ReadAnimations(scanner, nBone, nClip, animation);

		return 1;
	}
	
	return 0;
}
