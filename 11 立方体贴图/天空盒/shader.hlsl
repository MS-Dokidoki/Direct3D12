#define NUM_DIR_LIGHT 3
#include "lighting.hlsl"

struct MaterialData
{
    float4 vec4DiffuseAlbedo;
    float3 vec3FresnelR0;
    float nRoughness;
    float4x4 matMaterialTransform;
    uint nDiffuseTextureIndex;
};

cbuffer Object: register(b0)
{
    float4x4 matWorld;
    float4x4 matTexTransform;
    uint nIndex;
}

cbuffer Scene: register(b1)
{
    float4x4 matView;
    float4x4 matInvView;
    float4x4 matProj;
    float4x4 matInvProj;
    float4x4 matViewProj;
    float4x4 matInvViewProj;
    float3 vec3EyePos;
    float fPerObjectPad1;
    float2 vec2RenderTargetSize;
    float2 vec2InvRenderTargetSize;
    float fNearZ;
    float fFarZ;
    float fTotalTime;
    float fDeltaTime;
    float4 vec4AmbientLight;
    float4 vec4FogColor;
    float fFogStart;
    float fFogRange;
    float2 fPerScenePad2;

    Light lights[MAX_NUM_LIGHT];
};

struct VsInput
{
    float3 iPos: POSITION;
    float3 iNormal: NORMAL;
    float2 iTexC: TEXCOORD;
};

struct PsInput
{
    float4 iPos: SV_POSITION;
    float3 iPosW: POSITION;
    float3 iNormal: NORMAL;
    float2 iTexC: TEXCOORD;
};

struct PsInput_Sky
{
    float4 iPos: SV_POSITION;
    float3 iPosL: POSITION;
};

SamplerState gsamAnisotropicWrap: register(s0);
StructuredBuffer<MaterialData> MaterialDatas: register(t0, space1);
TextureCube SkyTexture: register(t0);
Texture2D DiffuseTextures[5]: register(t1);

PsInput VsMain(VsInput input)
{
    PsInput output;
    MaterialData matData = MaterialDatas[nIndex];

    float4 posW = mul(float4(input.iPos, 1.0), matWorld);
    output.iPosW = posW.xyz;

    output.iPos = mul(posW, matViewProj);
    output.iNormal = mul(input.iNormal, (float3x3)matWorld);

    float4 texC = mul(float4(input.iTexC, 0.0, 1.0), matTexTransform);
    output.iTexC = mul(texC, matData.matMaterialTransform).xy;

    return output;
}

float4 PsMain(PsInput input): SV_TARGET
{
    float4 fragColor = 0.0;
    MaterialData matData = MaterialDatas[nIndex];
    float4 vec4DiffuseAlbedo = matData.vec4DiffuseAlbedo;
    float3 vec3FresnelR0 = matData.vec3FresnelR0;
    float nRoughness = matData.nRoughness;
    uint nDiffuseTextureIndex = matData.nDiffuseTextureIndex;

    vec4DiffuseAlbedo *= DiffuseTextures[nDiffuseTextureIndex].Sample(gsamAnisotropicWrap, input.iTexC);

#ifdef ALPHA_TESTED
    clip(vec4DiffuseAlbedo.a - 0.1f);
#endif

    input.iNormal = normalize(input.iNormal);
    float3 eyeDir = vec3EyePos - input.iPosW;
    float eyeDis = length(eyeDir);
    eyeDir /= eyeDis;

    float4 AmbientLight = vec4AmbientLight * vec4DiffuseAlbedo;
    float3 shadowFactor = 1.0;
    Material mat;
    mat.vec4DiffuseAlbedo = vec4DiffuseAlbedo;
    mat.vec3FresnelR0 = vec3FresnelR0;
    mat.Shininess = 1.0 - nRoughness;
    
    float4 DirLight = ComputeLighting(lights, mat, input.iPosW, input.iNormal, eyeDir, shadowFactor);
    fragColor = DirLight + AmbientLight;

#ifdef FOG
    float fogAmount = saturate((eyeDis - fFogStart) / fFogRange);
    fragColor = lerp(fragColor, vec4FogColor, fogAmount);
#endif
    fragColor.a = vec4DiffuseAlbedo.a;

    return fragColor;
}

PsInput_Sky VsMain_Sky(VsInput input)
{
    PsInput_Sky output;

    output.iPosL = input.iPos;
    
    float4 posW = mul(float4(input.iPos, 1.0f), matWorld);
    posW.xyz += vec3EyePos;

    // 令 z 分量为 w, 以便透视除法后的 z 分量为 1.0, 即在远平面上
    output.iPos = mul(posW, matViewProj).xyww;

    return output;
}

float4 PsMain_Sky(PsInput_Sky input): SV_TARGET
{
    return SkyTexture.Sample(gsamAnisotropicWrap, input.iPosL);
}
