
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
Texture2D RandomVectorsMap: register(t2);

SamplerState Sams_PointClamp: register(s0);
SamplerState Sams_LinearClamp: register(s1);
SamplerState Sams_DepthMap: register(s2);
SamplerState Sams_LinearWarp: register(s3);

static const int nSampleCount = 14;

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
    float3 vec3Position_View: POSITION;
    float2 vec2TexCoords: TEXCOORD;
};

PsInput VsMain(uint vid: SV_VERTEXID)
{
    PsInput vout;

    // 各个角点即为其纹理坐标
    vout.vec2TexCoords = texCoords[vid];

    // 将角点转换至 NDC 空间
    vout.vec4Position = float4(vout.vec2TexCoords.x * 2.0f - 1.0f, 1.0f - 2.0f * vout.vec2TexCoords.y, 0.0f, 1.0f);

    // 将 NDC 坐标转化至投影空间(观察空间的近平面上)
    float4 posV = mul(vout.vec4Position, CBSsao_matInvProj);
    vout.vec3Position_View = posV.xyz / posV.w;

    return vout;
}

// distZ 为点 p 和点 r 的深度值之差
float OcclusionFunction(float distZ)
{
    float occlusion = 0.0f;

    // 若 distZ 大于平面阈值, 则进行遮蔽值的计算
    // 若 distZ 小于平面阈值, 则点 p 与点 r 的深度值距离过近, 亦认为该点 r 无法遮蔽点 p
    // 若 distZ 为负, 即点 r 位于点 p 的后面
    if(distZ > CBSsao_nSurfaceEpsilon)
    {
        float fadeLength = CBSsao_nOcclusionFadeEnd - CBSsao_nOcclusionFadeStart;
        // saturate(x) == clamp(x, 0.0f, 1.0f)
        // 若 distZ 越接近 nOcclusionFadeEnd, 则遮蔽值越接近 0
        occlusion = saturate((CBSsao_nOcclusionFadeEnd - distZ) / fadeLength);
    }
    return occlusion;
}

float NDCDepthToViewDepth(float z_ndc)
{
    // z_ndc = Proj[2][2] + Proj[3][2] / z_view
    return CBSsao_matProj[3][2] / (z_ndc - CBSsao_matProj[2][2]);
}
float PsMain(PsInput input): SV_TARGET
{
    // p -- 待进行处理的目标像素
    // n -- 目标像素的法线
    // q -- 偏离于点 p 的随机一点
    // r -- 潜在遮蔽点

    // 通过法线贴图获取法线
    float3 n = NormalMap.SampleLevel(Sams_PointClamp, input.vec2TexCoords, 0.0f).xyz;

    // 通过深度贴图获取当前像素处于 NDC 空间的深度值
    // 并将其转化至观察空间
    float pz = DepthMap.SampleLevel(Sams_DepthMap, input.vec2TexCoords, 0.0f).r;
    pz = NDCDepthToViewDepth(pz);

    //
    // 重新构建处于观察空间的目标点 p
    // 根据 p = t * v 可以推导出
    // p.z = t * v.z; 
    // t = p.z / v.z
    // p = (p.z / v.z) * v
    float3 p = (pz / input.vec3Position_View.z) * input.vec3Position_View;

    // 提取随机向量并将其映射至[-1, 1]的区间内
    float3 randomVec = 2.0f * RandomVectorsMap.SampleLevel(Sams_LinearWarp, 4.0f * input.vec2TexCoords, 0.0f).rgb - 1.0f;
    
    float occlusionSum = 0.0f;

    for(int i = 0; i < nSampleCount; ++i)
    {
        // 以随机向量为平面法线, 对偏移向量进行反射处理
        // 因为我们所提供的偏移向量为固定且均匀分布的, 所以根据随机向量反射所得的结果也都是均匀的
        float3 offset = reflect(CBSsao_vec4Offsets[i].xyz, randomVec);

        // sign(x): 若 x > 0, 则返回 1; x == 0, 则返回 0; x < 0, 则返回 -1
        float flip = sign(dot(offset, n));

        // 位于遮蔽半径内的随机一点
        float3 q = p + flip * CBSsao_nOcclusionRadius * offset;

        // 将点 q 从观察空间变换到 NDC 空间并转化为 纹理坐标
        float4 q_tex = mul(float4(q, 1.0f), CBSsao_matProjTex);
        q_tex /= q_tex.w;

        // 根据深度纹理获取在点 q 的路径上的潜在遮蔽点 r
        float rz = DepthMap.SampleLevel(Sams_DepthMap, q_tex.xy, 0.0f).r;
        rz = NDCDepthToViewDepth(rz);

        float3 r = (rz / q.z) * q;

        // 测试点 r 是否遮挡点 p
        float distZ = p.z - r.z;

        // dot(n, normalize(r - p)) 获取(r - p / ||r - p||) 与法线 n 之间的夹角的余弦值
        // 当 n 与 r - p 之间的余弦值为 0 时, 说明其夹角为 90 度
        // 因此, 将该值设置为遮蔽因子, 在一定程度上可以防止倾斜平面的自相交测试
        float dp = max(dot(n, normalize(r - p)), 0.0f);
		
        float occlusion = dp * OcclusionFunction(distZ);

        occlusionSum += occlusion;
    }

    occlusionSum /= nSampleCount;
    float access = 1.0f - occlusionSum;

    return saturate(pow(access, 6.0f));
}  
