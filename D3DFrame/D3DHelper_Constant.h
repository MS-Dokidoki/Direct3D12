#pragma once
#ifndef _D3DHELPER_CONSTANT_H
#define _D3DHELPER_CONSTANT_H
#include "D3DBase.h"
#include "D3DHelper_Math.h"

#define D3DHelper_CalcConstantBufferBytesSize(byteSize) ((byteSize + 255) & ~255)

#define WORLD_DEF_FOGCOLOR {0.7f, 0.7f, 0.7f, 0.7f}
#define WORLD_DEF_FOGSTART 5.0f
#define WORLD_DEF_FOGRANGE 150.0f

#define MATRIX_IDENTITY MathHelper::Identity4x4()

#include "D3DHelper_Light.h"

namespace D3DHelper
{
    namespace Constant
	{
		/// @brief 材质常量数据
		struct MaterialConstant
		{
			DirectX::XMFLOAT4 vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};	// 漫反射反照率
			DirectX::XMFLOAT3 vec3FresnelR0 = {0.01f, 0.01f, 0.01f};		// 材质属性R(0°)
			float nRoughness = 64.0f;						// 表面粗糙度
			DirectX::XMFLOAT4X4 matTransform = MATRIX_IDENTITY;
			UINT nDiffuseTextureIndex = 0;
            UINT nNormalTextureIndex = 0;
        };
		
		/// @brief 场景常量数据
        struct SceneConstant
        {
            DirectX::XMFLOAT4X4 matView = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matInvView = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matInvProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matViewProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matInvViewProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matShadowTransform = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matViewProjTex = MATRIX_IDENTITY;
            DirectX::XMFLOAT3 vec3EyePos = {0.0f, 0.0f, 0.0f};
            float fPerScenePad1 = 0.0f;

            DirectX::XMFLOAT2 vec2RenderTargetSize = {0.0f, 0.0f};
            DirectX::XMFLOAT2 vec2InvRenderTargetSize = {0.0f, 0.0f};
            float fNearZ = 0.0f;
            float fFarZ = 0.0f;
            float fTotalTime = 0.0f;
            float fDeltaTime = 0.0f;
			DirectX::XMFLOAT4 vec4AmbientLight = {1.0f, 1.0f, 1.0f, 1.0f};
            DirectX::XMFLOAT4 vec4FogColor = WORLD_DEF_FOGCOLOR;
            float fFogStart = WORLD_DEF_FOGSTART;
            float fFogRange = WORLD_DEF_FOGRANGE;
            DirectX::XMFLOAT2 fPerScenePad2 = {0.0f, 0.0f};

            Light::Light lights[MAX_NUM_LIGHT];
        };

        /// @brief 对象常量数据
        struct ObjectConstant
        {
            DirectX::XMFLOAT4X4 matWorld = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matTexTransform = MATRIX_IDENTITY;
			UINT nMaterialIndex;
        };

        struct SsaoConstant
        {
            DirectX::XMFLOAT4X4 matProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matInvProj = MATRIX_IDENTITY;
            DirectX::XMFLOAT4X4 matProjTex = MATRIX_IDENTITY;
            DirectX::XMFLOAT4 vec4Offsets[14];
            DirectX::XMFLOAT4 vec4BlurWeights[3];
            DirectX::XMFLOAT2 vec2InvRenderTargetSize = {0.0f, 0.0f};
            float nOcclusionRadius = 0.0f;
            float nOcclusionFadeStart = 0.0f;
            float nOcclusionFadeEnd = 0.0f;
            float nSurfaceEpsilon = 0.0f;
        };

        struct SsaoBlurConstant
        {
            bool bHorizontalBlur;
        };

        struct SkinnedConstant
        {
            DirectX::XMFLOAT4X4 matTransforms[96];
        };
	};
	
	namespace CONSTANT_VALUE
    {
        static const UINT nCBObjectByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::ObjectConstant));
        static const UINT nCBSceneByteSize  = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::SceneConstant));
        static const UINT nCBMaterialByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::MaterialConstant));
        static const UINT nCBSsaoByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::SsaoConstant));
        static const UINT nCBSsaoBlurByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::SsaoBlurConstant));
        static const UINT nCBSkinnedByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Constant::SkinnedConstant));
    };
};

#endif