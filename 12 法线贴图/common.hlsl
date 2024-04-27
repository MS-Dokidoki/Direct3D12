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