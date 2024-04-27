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

    vout.vec4Position = float4(input.vec3Position, 1.0f);
    vout.vec2TexCoords = input.vec2TexCoords;
    return vout;
}

float4 PsMain(PsInput input): SV_TARGET
{
    return float4(Textures[10].SampleLevel(SamS_LinearClamp, input.vec2TexCoords.xy, 0.0f).rrr, 1.0);
}
