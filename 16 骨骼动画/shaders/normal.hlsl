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
    float3 vec3Normal_World: NORMAL;
    float3 vec3Tangent_World: TANGENT;
    float2 vec2TexCoords: TEXCOORD;
};

PsInput VsMain(VsInput input)
{
    PsInput vout;
    MaterialData mat = MaterialDatas[CBObject_nMaterialIndex];
    
    vout.vec3Normal_World = mul(input.vec3Normal, (float3x3)CBObject_matWorld);
    vout.vec3Tangent_World = mul(input.vec3TangentU, (float3x3)CBObject_matWorld);

    float4 posW = mul(float4(input.vec3Position, 1.0), CBObject_matWorld);
    vout.vec4Position = mul(posW, CBScene_matViewProj);

    float4 texC = mul(float4(input.vec2TexCoords, 0.0f, 1.0f), CBObject_matTexTransform);
    vout.vec2TexCoords = mul(texC, mat.matMaterialTransform).xy;

    return vout;
}

float4 PsMain(PsInput input): SV_TARGET
{
    MaterialData mat = MaterialDatas[CBObject_nMaterialIndex];
    float4 diffuseAlbedo = mat.vec4DiffuseAlbedo;
    uint diffuseMapIndex = mat.nDiffuseMapIndex;
    uint normalMapIndex = mat.nNormalMapIndex;

    diffuseAlbedo *= Textures[diffuseMapIndex].Sample(SamS_AnisotropicWrap, input.vec2TexCoords);
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif

    input.vec3Normal_World = normalize(input.vec3Normal_World);
    float3 normalV = mul(input.vec3Normal_World, (float3x3)CBScene_matView);

    return float4(normalV, 0.0f);
}
