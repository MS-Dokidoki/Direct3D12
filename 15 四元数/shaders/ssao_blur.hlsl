
cbuffer CBSsao: register(b0)
{
    float4x4 CBSsao_matProj;
    float4x4 CBSsao_matInvProj;
    float4x4 CBSsao_matProjTex;
    float4 CBSsao_vec4Offsets[14];
    float4 CBSsao_vec4BlurWeights[3];
    float2 CBSsao_vec2InvRnederTargetSize;
    float CBSsao_nOcclusionRadius;      
    float CBSsao_nOcclusionFadeStart;  
    float CBSsao_nOcclusionFadeEnd;     
    float CBSsao_nSurfaceEpsilon;    
}

cbuffer CBSsao_Blur: register(b1)
{
    bool CBSsao_bHorizontalBlur;
}

Texture2D NormalMap: register(t0);
Texture2D DepthMap: register(t1);
Texture2D InputMap: register(t2);

SamplerState Sams_PointClamp: register(s0);
SamplerState Sams_LinearClamp: register(s1);
SamplerState Sams_DepthMap: register(s2);
SamplerState Sams_LinearWarp: register(s3);

static const int nBlurRadius = 5;

// 全屏四边形的各个角点
// 根据三角形缠绕顺序可以推导出
// 屏幕空间是标准的 Windows 窗口空间坐标系(即以左上角为原点的坐标系)
static const float2 texCoords[6] = {
    {0.0f, 1.0f},   // 左下角
    {0.0f, 0.0f},   // 左上角
    {1.0f, 0.0f},   // 右上角
    {0.0f, 1.0f},   // 左下角
    {1.0f, 0.0f},   // 右上角
    {1.0f, 1.0f}    // 右下角
};

struct PsInput
{
    float4 vec4Position: SV_POSITION;
    float2 vec2TexCoords: TEXCOORD;
};

PsInput VsMain(uint vid: SV_VERTEXID)
{
    PsInput vout;

    vout.vec2TexCoords = texCoords[vid];

    vout.vec4Position = float4(2.0f * vout.vec2TexCoords.x - 1.0f, 1.0f - 2.0f * vout.vec2TexCoords.y, 0.0f, 1.0f);

    return vout;
}

float NDCDepthToViewDepth(float z_ndc)
{
    return CBSsao_matProj[3][2] / (z_ndc - CBSsao_matProj[2][2]);
}

float4 PsMain(PsInput pin): SV_TARGET
{
    float weights[11] = {
        CBSsao_vec4BlurWeights[0].x, CBSsao_vec4BlurWeights[0].y, CBSsao_vec4BlurWeights[0].z, CBSsao_vec4BlurWeights[0].w,
        CBSsao_vec4BlurWeights[1].x, CBSsao_vec4BlurWeights[1].y, CBSsao_vec4BlurWeights[1].z, CBSsao_vec4BlurWeights[1].w,
        CBSsao_vec4BlurWeights[2].x, CBSsao_vec4BlurWeights[2].y, CBSsao_vec4BlurWeights[2].z
    };

    float2 texOffset;
    if(CBSsao_bHorizontalBlur)
    {
        texOffset = float2(CBSsao_vec2InvRnederTargetSize.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0f, CBSsao_vec2InvRnederTargetSize.y);
    }
    float4 color = weights[nBlurRadius] * InputMap.SampleLevel(Sams_PointClamp, pin.vec2TexCoords, 0.0f);
    float totalWeight = weights[nBlurRadius];

    float3 centerNormal = NormalMap.SampleLevel(Sams_PointClamp, pin.vec2TexCoords, 0.0f).xyz;
    float centerDepth = NDCDepthToViewDepth(DepthMap.SampleLevel(Sams_DepthMap, pin.vec2TexCoords, 0.0f).r);

    for(int i = -nBlurRadius; i <= nBlurRadius; ++i)
    {
        if(i == 0)
            continue;
        float2 tex = pin.vec2TexCoords + i * texOffset;

        float3 neighborNormal = NormalMap.SampleLevel(Sams_PointClamp, tex, 0.0f).xyz;
        float neighborDepth = DepthMap.SampleLevel(Sams_DepthMap, tex, 0.0f).r;

        if(dot(neighborNormal, centerNormal) >= 0.8f && abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = weights[i + nBlurRadius];
            color += weight * InputMap.SampleLevel(Sams_PointClamp, pin.vec2TexCoords, 0.0f);

            totalWeight += weight;
        }
    }

    return color / totalWeight;
}
