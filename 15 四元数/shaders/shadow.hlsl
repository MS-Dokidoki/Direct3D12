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
    float2 vec2TexCoords: TEXCOORD;
};

PsInput VsMain(VsInput input)
{
    PsInput vout;
    MaterialData mat = MaterialDatas[CBObject_nMaterialIndex];

    float4 posW = mul(float4(input.vec3Position, 1.0f), CBObject_matWorld);
    vout.vec4Position = mul(posW, CBScene_matViewProj);

    float4 texC = mul(float4(input.vec2TexCoords, 0.0f, 1.0f), CBObject_matTexTransform);
    vout.vec2TexCoords = mul(posW, mat.matMaterialTransform).xy;

    return vout;
}

void PsMain(PsInput input)
{
    MaterialData mat = MaterialDatas[CBObject_nMaterialIndex];
    float4 diffuseAlbedo = mat.vec4DiffuseAlbedo;
    uint diffuseMapIndex = mat.nDiffuseMapIndex;

    diffuseAlbedo *= Textures[diffuseMapIndex].Sample(SamS_AnisotropicClamp, input.vec2TexCoords);
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1)
#endif 
}