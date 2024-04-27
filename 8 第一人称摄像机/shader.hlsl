#define NUM_DIR_LIGHT 1
#include "lighting.hlsl"

cbuffer Object: register(b0)
{
    float4x4 matWorld;
    float4x4 matTexTransform;
}

cbuffer Material: register(b1)
{
    float4 vec4DiffuseAlbedo;
    float3 vec3FresnelR0;
    float nRoughness;
    float4x4 matMaterialTransform;
}

cbuffer Scene: register(b2)
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

SamplerState gsamAnisotropicWrap: register(s0);
Texture2D gDiffuseTexture: register(t0);

PsInput VsMain(VsInput input)
{
    PsInput output;
    float4 posW = mul(float4(input.iPos, 1.0), matWorld);
    output.iPosW = posW.xyz;

    output.iPos = mul(posW, matViewProj);
    output.iNormal = mul(input.iNormal, (float3x3)matWorld);

    float4 texC = mul(float4(input.iTexC, 0.0, 1.0), matTexTransform);
    output.iTexC = mul(texC, matMaterialTransform).xy;

    return output;
}

float4 PsMain(PsInput input): SV_TARGET
{
    float4 fragColor = 0.0;
    float4 diffuseAlbedo;

    diffuseAlbedo = gDiffuseTexture.Sample(gsamAnisotropicWrap, input.iTexC) * vec4DiffuseAlbedo;
#ifdef ALPHA_TESTED
    clip(diffuseAlbedo.a - 0.1f);
#endif

    input.iNormal = normalize(input.iNormal);
    float3 eyeDir = vec3EyePos - input.iPosW;
    float eyeDis = length(eyeDir);
    eyeDir /= eyeDis;

    float4 AmbientLight = vec4AmbientLight * diffuseAlbedo;
    float3 shadowFactor = 1.0;
    Material mat;
    mat.vec4DiffuseAlbedo = diffuseAlbedo;
    mat.vec3FresnelR0 = vec3FresnelR0;
    mat.Shininess = 1.0 - nRoughness;
    
    float4 DirLight = ComputeLighting(lights, mat, input.iPosW, input.iNormal, eyeDir, shadowFactor);
    fragColor = DirLight + AmbientLight;

#ifdef FOG
    float fogAmount = saturate((eyeDis - fFogStart) / fFogRange);
    fragColor = lerp(fragColor, vec4FogColor, fogAmount);
#endif
    fragColor.a = diffuseAlbedo.a;

    return fragColor;
}

float4 PsMain_DCX(PsInput params): SV_TARGET
{
    return float4(0.1f, 0.1f, 0.1f, 0.1f);
}