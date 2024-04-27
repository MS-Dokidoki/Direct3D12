#include "lighting.hlsl"

cbuffer Object: register(b0)
{
    float4x4 CBObject_matWorld;
    float4x4 CBObject_matTexTransform;
	uint CBObject_nMaterialIndex;
}

cbuffer Scene: register(b1)
{
	float4x4 CBScene_matView;
	float4x4 CBScene_matInvView;
	float4x4 CBScene_matProj;
	float4x4 CBScene_matInvProj;
	float4x4 CBScene_matViewProj;
	float4x4 CBScene_matInvViewProj;
	float4x4 CBScene_matShadowTransform;
	float4x4 CBScene_matViewProjTex;
	float3 CBScene_vec3EyePos;
	float CBScene_cbPerObjectPad1;
	float2 CBScene_vec2RenderTargetSize;
	float2 CBScene_vec2InvRenderTargetSize;
	float CBScene_nNearZ;
	float CBScene_nFarZ;
	float CBScene_fTotalTime;
	float CBScene_fDeltaTime;
	float4 CBScene_vec4AmbientLight;
	float4 CBScene_vec4FogColor;
	float CBScene_fFogStart;
    float CBScene_fFogRange;
    float2 CBScene_fPerScenePad2;

	Light CBScene_Lights[MAX_NUM_LIGHT];
}

struct MaterialData
{
	float4 vec4DiffuseAlbedo;
	float3 vec3FresnelR0;
	float nRoughness;
	float4x4 matMaterialTransform;
	uint nDiffuseMapIndex;
	uint nNormalMapIndex;
};

SamplerState SamS_PointWarp: register(s0);
SamplerState SamS_PointClamp: register(s1);
SamplerState SamS_LinearWrap: register(s2);
SamplerState SamS_LinearClamp: register(s3);
SamplerState SamS_AnisotropicWrap: register(s4);
SamplerState SamS_AnisotropicClamp: register(s5);
SamplerComparisonState SamS_Shadow: register(s6);

TextureCube SkyCube: register(t0);
Texture2D ShadowMap: register(t1);
Texture2D SsaoMap: register(t2);
Texture2D Textures[16]: register(t3);
StructuredBuffer<MaterialData> MaterialDatas : register(t0, space1);

float3 NormalSampleToWorldSpace(float3 normalSample, float3 unitNormal_World, float3 tangent_World)
{
	float3 normalT = 2.0f * normalSample - 1.0f;

	float3 N = unitNormal_World;
	float3 T = normalize(tangent_World - dot(tangent_World, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = {T, B, N};

	float3 bumpedNormal = mul(normalT, TBN);

	return bumpedNormal;
}

float CalcShadowFactor(float4 shadowPos)
{
	shadowPos.xyz /= shadowPos.w;

	float depth = shadowPos.z;

	uint width, height, numMips;
	ShadowMap.GetDimensions(0, width, height, numMips);

	float dx = 1.0f / (float)width;

	float percentLit = 0.0f;
	const float2 offsets[9] = {
		{-dx, -dx}, {0.0f, -dx}, {dx, -dx},
		{-dx, 0.0f}, {0.0f, 0.0f}, {dx, 0.0f},
		{-dx, dx}, {0.0f, dx}, {dx, dx}
	};

	[unroll]
	for(int i = 0; i < 9; ++i)
	{
		percentLit += ShadowMap.SampleCmpLevelZero(SamS_Shadow, shadowPos.xy + offsets[i], depth).r;
	}

	return percentLit / 9.0f;
}
