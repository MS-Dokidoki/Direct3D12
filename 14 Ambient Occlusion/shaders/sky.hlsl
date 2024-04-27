#include "common.hlsl"

struct VsInput
{
	float3 vec3Position: POSITION;
	float3 vec3Normal: NORMAL;
	float3 vec3TangentU: TANGENT;
	float2 vec2TexCoords: TEXCOORDS;
};

struct PsInput
{
	float4 vec4Position: SV_POSITION;
	float3 vec3Position_Local: POSITION;
};

// Sky Main
PsInput VsMain(VsInput vin)
{
	PsInput vout;
	
	vout.vec3Position = vin.vec3Position;
	float4 pos = mul(float4(vin.vec3Position, 1.0f), CBObject_matWorld);
	vout.vec4Position = mul(pos, CBScene_matViewProj).xyww;
	
	return vout;
}

float4 PsMain(PsInput pin): SV_TARGET
{
	return SkyCube.Sample(SamS_AnisotropicWrap, pin.vec3Position_Local);
}