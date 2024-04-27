#pragma once
#ifndef _D3DHELPER_ANIMATION_H
#define _D3DHELPER_ANIMATION_H
#include "D3DBase.h"
#include <unordered_map>

namespace D3DHelper
{
	namespace Animation
	{
		struct Keyframe
		{
			float nTimePos;
			DirectX::XMFLOAT3 vec3Translation;
			DirectX::XMFLOAT3 vec3Scale;
			DirectX::XMFLOAT4 vec4RotationQuat;
		};
		
		struct BoneAnimation
		{
			float GetStartTime() const;
			float GetEndTime() const;
			void Interpolate(float t, DirectX::XMFLOAT4X4& M) const;
			
			std::vector<Keyframe> Keyframes;
		};
		
		struct AnimationClip
		{
			// 获取该动画片段中最早的起始时间
			float GetStartTime() const;
			// 获取该动画片段中最晚的结束时间
			float GetEndTime() const;
			void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& M) const;
			
			std::vector<BoneAnimation> BoneAnimations;
		};
		
		class SkinnedAnimation
		{
		public:	
			UINT BoneCount() const;
			
			float GetClipStartTime(const std::string& clipName) const;
			float GetClipEndTime(const std::string& clipName) const;
			
			void Set(std::vector<int>& boneHierarchy, std::vector<DirectX::XMFLOAT4X4>& boneOffsets, std::unordered_map<std::string, AnimationClip>& animations);
			
			void GetFinalTransforms(const std::string& clipName, float timePos, std::vector<DirectX::XMFLOAT4X4>& finalTransforms) const;

			AnimationClip& GetAnimationClip(std::string& clipName){return Animations[clipName];}
		private:
			std::vector<int> BoneHierarchy;
			std::vector<DirectX::XMFLOAT4X4> BoneOffsets;
			std::unordered_map<std::string, AnimationClip> Animations;
		};
		
	};
};

#endif