#define NUM_DIR_LIGHT 3
#include "common.hlsl"

struct VsInput
{
	float3 vec3Position: POSITION;
	float3 vec3Normal: NORMAL;
	float2 vec2TexCoords: TEXCOORD;
	float3 vec3TangentU: TANGENT;
};

struct PsInput
{
    float4 vec4Position: SV_POSITION;
	float3 vec3Position_World: POSITION;
	float3 vec3Normal_World: NORMAL;
	float3 vec3Tangent_World: TANGENT;
	float2 vec2TexCoords: TEXCOORD;
	float4 vec4ShadowPosition: POSITION1;
	float4 vec4SsaoPos: POSITION2;
};

PsInput VsMain(VsInput vin)
{
	PsInput vout;
	
	MaterialData matData = MaterialDatas[CBObject_nMaterialIndex];
	
	float4 pos = mul(float4(vin.vec3Position, 1.0f), CBObject_matWorld);
	vout.vec3Position_World = pos.xyz;

	vout.vec4Position = mul(pos, CBScene_matViewProj);
	vout.vec4ShadowPosition = mul(pos, CBScene_matShadowTransform);
	
	vout.vec3Normal_World = mul(vin.vec3Normal, (float3x3)CBObject_matWorld);
	vout.vec3Tangent_World = mul(vin.vec3TangentU, (float3x3)CBObject_matWorld);

	float4 texCoords = mul(float4(vin.vec2TexCoords, 0.0f, 1.0f), CBObject_matTexTransform);
	vout.vec2TexCoords = mul(texCoords, matData.matMaterialTransform).xy;
	vout.vec4SsaoPos = mul(pos, CBScene_matViewProjTex);

	return vout;
}

float4 PsMain(PsInput pin): SV_TARGET
{
	MaterialData matData = MaterialDatas[CBObject_nMaterialIndex];
	float4 diffuseAlbedo = matData.vec4DiffuseAlbedo;
	float3 fresnelR0 = matData.vec3FresnelR0;
	float roughness = matData.nRoughness;
	uint nDiffuseTexIndex = matData.nDiffuseMapIndex;
	uint nNormalTexIndex = matData.nNormalMapIndex;

	diffuseAlbedo *= Textures[nDiffuseTexIndex].Sample(SamS_PointWarp, pin.vec2TexCoords);
	
#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif

	pin.vec3Normal_World = normalize(pin.vec3Normal_World);
	
	float4 normalMapSample = Textures[nNormalTexIndex].Sample(SamS_AnisotropicWrap, pin.vec2TexCoords);
	float3 bumpedNormal = NormalSampleToWorldSpace(normalMapSample.xyz, pin.vec3Normal_World, pin.vec3Tangent_World);

	// bumpedNormal = pin.vec3Normal_World;

	float3 eyeDir = normalize(CBScene_vec3EyePos - pin.vec3Position_World);
		
	pin.vec4SsaoPos /= pin.vec4SsaoPos.w;
	float ambientFactor = SsaoMap.SampleLevel(SamS_LinearClamp, pin.vec4SsaoPos.xy, 0.0f).r;
	float4 vec4Ambient = CBScene_vec4AmbientLight * diffuseAlbedo;
	
	const float shininess = 1.0f - roughness;
	Material mat;
	mat.vec4DiffuseAlbedo = diffuseAlbedo;
	mat.vec3FresnelR0 = fresnelR0;
	mat.Shininess = shininess;
	float3 shadowFactor = 1.0;
	shadowFactor[0] = CalcShadowFactor(pin.vec4ShadowPosition);
	
	float4 vec4Diffuse = ComputeLighting(CBScene_Lights, mat, pin.vec3Position_World, bumpedNormal, eyeDir, shadowFactor);
	
	float4 fragColor = vec4Ambient + vec4Diffuse;
	
	float3 r = reflect(-eyeDir, bumpedNormal);
	float4 vec4ReflectionColor = SkyCube.Sample(SamS_LinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormal, r);
	fragColor.rgb += shininess * fresnelFactor * vec4ReflectionColor.rgb;
	
	fragColor.a = diffuseAlbedo.a;
	
	return fragColor;

}

struct PsInput_Sky
{
	float4 vec4Position: SV_POSITION;
	float3 vec3Position_Local: POSITION;
};

PsInput_Sky VsMain_Sky(VsInput vin)
{
	PsInput_Sky vout;

	vout.vec3Position_Local = vin.vec3Position;

	float4 pos = mul(float4(vin.vec3Position, 1.0f), CBObject_matWorld);
	pos.xyz += CBScene_vec3EyePos;
	vout.vec4Position = mul(pos, CBScene_matViewProj).xyww;

	return vout;
}

float4 PsMain_Sky(PsInput_Sky pin): SV_TARGET
{
	return SkyCube.Sample(SamS_AnisotropicWrap, pin.vec3Position_Local);
}