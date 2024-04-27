#include "D3DHelper_Animation.h"
#include "D3DHelper_Math.h"

using namespace D3DHelper::Animation;
using namespace DirectX;

float BoneAnimation::GetStartTime() const
{
	return Keyframes.front().nTimePos;
}

float BoneAnimation::GetEndTime() const
{
	return Keyframes.back().nTimePos;
}

void BoneAnimation::Interpolate(float t, XMFLOAT4X4& M) const
{
	XMVECTOR Z = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR S;
	XMVECTOR T;
	XMVECTOR Q;

	if(t <= Keyframes.front().nTimePos)
	{
		S = XMLoadFloat3(&Keyframes.front().vec3Scale);
		T = XMLoadFloat3(&Keyframes.front().vec3Translation);
		Q = XMLoadFloat4(&Keyframes.front().vec4RotationQuat);
		
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, Z, Q, T));
	}
	else if(t >= Keyframes.back().nTimePos)
	{
		S = XMLoadFloat3(&Keyframes.back().vec3Scale);
		T = XMLoadFloat3(&Keyframes.back().vec3Translation);
		Q = XMLoadFloat4(&Keyframes.back().vec4RotationQuat);
		
		XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, Z, Q, T));
	}
	else
	{
		for(UINT i = 0; i < Keyframes.size() - 1; ++i)
		{
			if(t >= Keyframes[i].nTimePos && t <= Keyframes[i + 1].nTimePos)
			{
				float lerpPercent = (t - Keyframes[i].nTimePos) / (Keyframes[i + 1].nTimePos - Keyframes[i].nTimePos);
				
				XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].vec3Scale);
				XMVECTOR s1 = XMLoadFloat3(&Keyframes[i + 1].vec3Scale);
				
				XMVECTOR t0 = XMLoadFloat3(&Keyframes[i].vec3Translation);
				XMVECTOR t1 = XMLoadFloat3(&Keyframes[i + 1].vec3Translation);
				
				XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].vec4RotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&Keyframes[i + 1].vec4RotationQuat);
				
				S = XMVectorLerp(s0, s1, lerpPercent);
				T = XMVectorLerp(t0, t1, lerpPercent);
				Q = XMVectorLerp(q0, q1, lerpPercent);
				
				XMStoreFloat4x4(&M, XMMatrixAffineTransformation(S, Z, Q, T));
				break;
			}
		}
	}

}

float AnimationClip::GetStartTime() const
{
	float t = MathHelper::Infinity;
	
	for(UINT i = 0; i < BoneAnimations.size(); ++i)
		t = min(t, BoneAnimations[i].GetStartTime());
	
	return t;
}

float AnimationClip::GetEndTime() const
{
	float t = 0.0f;
	
	for(UINT i = 0; i < BoneAnimations.size(); ++i)
		t = max(t, BoneAnimations[i].GetEndTime());
	
	return t;
}

void AnimationClip::Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& Ms) const
{
	for(UINT i = 0; i < BoneAnimations.size(); ++i)
		BoneAnimations[i].Interpolate(t, Ms[i]);
}

UINT SkinnedAnimation::BoneCount() const
{
	return BoneHierarchy.size();
}

float SkinnedAnimation::GetClipStartTime(const std::string& clipName) const
{
	auto itor = Animations.find(clipName);
	if(itor == Animations.end())
		return 0;
	else 
		return itor->second.GetStartTime();
}

float SkinnedAnimation::GetClipEndTime(const std::string& clipName) const
{
	auto itor = Animations.find(clipName);
	if(itor == Animations.end())
		return 0;
	else 
		return itor->second.GetEndTime();
}

void SkinnedAnimation::Set(std::vector<int>& boneHierarchy, std::vector<DirectX::XMFLOAT4X4>& boneOffsets, std::unordered_map<std::string, AnimationClip>& animations)
{
	BoneHierarchy = boneHierarchy;
	BoneOffsets = boneOffsets;
	Animations = animations;
}

void SkinnedAnimation::GetFinalTransforms(const std::string& clipName, float t, std::vector<DirectX::XMFLOAT4X4>& trans) const
{
	UINT nBones = BoneOffsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(nBones);

	auto& clip = Animations.find(clipName);
	clip->second.Interpolate(t, toParentTransforms);

	std::vector<XMFLOAT4X4> toRootTransforms(nBones);

	toRootTransforms[0] = toParentTransforms[0];

	for(UINT i = 1; i < nBones; ++i)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

		int parentIndex = BoneHierarchy[i];
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	for(UINT i = 0; i < nBones; ++i)
	{
		XMMATRIX offset = XMLoadFloat4x4(&BoneOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMStoreFloat4x4(&trans[i], XMMatrixTranspose(XMMatrixMultiply(offset, toRoot)));
	}	
}