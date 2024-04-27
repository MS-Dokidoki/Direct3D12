#define NUM_DIR_LIGHT 3
#include "common.hlsl"

TextureCube CubeMap: register(t0);
Texture2D DiffuseMaps[5]: register(t1);
StructuredBuffer<MaterialData> MaterialDatas: register(t0, space1);

struct VsInput
{
	float3 vec3Position: POSITION;
	float3 vec3Normal: NORMAL;
	float2 vec2TexCoords: TEXCOORD;
};

struct PsInput
{
    float4 vec4Position: SV_POSITION;
	float3 vec3Position_World: POSITION;
	float3 vec3Normal_World: NORMAL;
	float2 vec2TexCoords: TEXCOORD;
};

PsInput VsMain(VsInput vin)
{
	PsInput vout;
	
	MaterialData matData = MaterialDatas[CBObject_nMaterialIndex];
	
	float4 pos = mul(float4(vin.vec3Position, 1.0f), CBObject_matWorld);
	vout.vec3Position_World = pos.xyz;
	
	vout.vec4Position = mul(pos, CBScene_matViewProj);
	
	vout.vec3Normal_World = mul(vin.vec3Normal, (float3x3)CBObject_matWorld);
	
	float4 texCoords = mul(float4(vin.vec2TexCoords, 0.0f, 1.0f), CBObject_matTexTransform);
	vout.vec2TexCoords = mul(texCoords, matData.matMaterialTransform).xy;
	
	return vout;
}

float4 PsMain(PsInput pin): SV_TARGET
{
	MaterialData matData = MaterialDatas[CBObject_nMaterialIndex];
	float4 diffuseAlbedo = matData.vec4DiffuseAlbedo;
	float3 fresnelR0 = matData.vec3FresnelR0;
	float roughness = matData.nRoughness;
	uint nDiffuseTexIndex = matData.nDiffuseMapIndex;
	
	diffuseAlbedo *= DiffuseMaps[nDiffuseTexIndex].Sample(SamS_PointWarp, pin.vec2TexCoords);
	
	pin.vec3Normal_World = normalize(pin.vec3Normal_World);
	
	float3 eyeDir = normalize(CBScene_vec3EyePos - pin.vec3Position_World);
	
	float4 vec4Ambient = CBScene_vec4AmbientLight * diffuseAlbedo;
	
	Material mat;
	mat.vec4DiffuseAlbedo = diffuseAlbedo;
	mat.vec3FresnelR0 = fresnelR0;
	mat.Shininess = 1.0f - roughness;
	float3 shadowFactor = 1.0;

	float4 vec4Diffuse = ComputeLighting(CBScene_Lights, mat, pin.vec3Position_World, pin.vec3Normal_World, eyeDir, shadowFactor);
	
	float4 fragColor = vec4Ambient + vec4Diffuse;
	
	float3 r = reflect(-eyeDir, pin.vec3Normal_World);
	float4 vec4ReflectionColor = CubeMap.Sample(SamS_LinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, pin.vec3Normal_World, r);
	fragColor.rgb += (1.0f - roughness) * fresnelFactor * vec4ReflectionColor.rgb;
	
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
	return CubeMap.Sample(SamS_AnisotropicWrap, pin.vec3Position_Local);
}