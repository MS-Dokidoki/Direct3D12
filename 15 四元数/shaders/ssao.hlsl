
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

// ȫ���ı��εĸ����ǵ�
// ���������β���˳������Ƶ���
// ��Ļ�ռ��Ǳ�׼�� Windows ���ڿռ�����ϵ(�������Ͻ�Ϊԭ�������ϵ)
static const float2 texCoords[6] = {
    {0.0f, 1.0f},   // ���½�
    {0.0f, 0.0f},   // ���Ͻ�
    {1.0f, 0.0f},   // ���Ͻ�
    {0.0f, 1.0f},   // ���½�
    {1.0f, 0.0f},   // ���Ͻ�
    {1.0f, 1.0f}    // ���½�
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

    // �����ǵ㼴Ϊ����������
    vout.vec2TexCoords = texCoords[vid];

    // ���ǵ�ת���� NDC �ռ�
    vout.vec4Position = float4(vout.vec2TexCoords.x * 2.0f - 1.0f, 1.0f - 2.0f * vout.vec2TexCoords.y, 0.0f, 1.0f);

    // �� NDC ����ת����ͶӰ�ռ�(�۲�ռ�Ľ�ƽ����)
    float4 posV = mul(vout.vec4Position, CBSsao_matInvProj);
    vout.vec3Position_View = posV.xyz / posV.w;

    return vout;
}

// distZ Ϊ�� p �͵� r �����ֵ֮��
float OcclusionFunction(float distZ)
{
    float occlusion = 0.0f;

    // �� distZ ����ƽ����ֵ, ������ڱ�ֵ�ļ���
    // �� distZ С��ƽ����ֵ, ��� p ��� r �����ֵ�������, ����Ϊ�õ� r �޷��ڱε� p
    // �� distZ Ϊ��, ���� r λ�ڵ� p �ĺ���
    if(distZ > CBSsao_nSurfaceEpsilon)
    {
        float fadeLength = CBSsao_nOcclusionFadeEnd - CBSsao_nOcclusionFadeStart;
        // saturate(x) == clamp(x, 0.0f, 1.0f)
        // �� distZ Խ�ӽ� nOcclusionFadeEnd, ���ڱ�ֵԽ�ӽ� 0
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
    // p -- �����д����Ŀ������
    // n -- Ŀ�����صķ���
    // q -- ƫ���ڵ� p �����һ��
    // r -- Ǳ���ڱε�

    // ͨ��������ͼ��ȡ����
    float3 n = NormalMap.SampleLevel(Sams_PointClamp, input.vec2TexCoords, 0.0f).xyz;

    // ͨ�������ͼ��ȡ��ǰ���ش��� NDC �ռ�����ֵ
    // ������ת�����۲�ռ�
    float pz = DepthMap.SampleLevel(Sams_DepthMap, input.vec2TexCoords, 0.0f).r;
    pz = NDCDepthToViewDepth(pz);

    //
    // ���¹������ڹ۲�ռ��Ŀ��� p
    // ���� p = t * v �����Ƶ���
    // p.z = t * v.z; 
    // t = p.z / v.z
    // p = (p.z / v.z) * v
    float3 p = (pz / input.vec3Position_View.z) * input.vec3Position_View;

    // ��ȡ�������������ӳ����[-1, 1]��������
    float3 randomVec = 2.0f * RandomVectorsMap.SampleLevel(Sams_LinearWarp, 4.0f * input.vec2TexCoords, 0.0f).rgb - 1.0f;
    
    float occlusionSum = 0.0f;

    for(int i = 0; i < nSampleCount; ++i)
    {
        // ���������Ϊƽ�淨��, ��ƫ���������з��䴦��
        // ��Ϊ�������ṩ��ƫ������Ϊ�̶��Ҿ��ȷֲ���, ���Ը�����������������õĽ��Ҳ���Ǿ��ȵ�
        float3 offset = reflect(CBSsao_vec4Offsets[i].xyz, randomVec);

        // sign(x): �� x > 0, �򷵻� 1; x == 0, �򷵻� 0; x < 0, �򷵻� -1
        float flip = sign(dot(offset, n));

        // λ���ڱΰ뾶�ڵ����һ��
        float3 q = p + flip * CBSsao_nOcclusionRadius * offset;

        // ���� q �ӹ۲�ռ�任�� NDC �ռ䲢ת��Ϊ ��������
        float4 q_tex = mul(float4(q, 1.0f), CBSsao_matProjTex);
        q_tex /= q_tex.w;

        // ������������ȡ�ڵ� q ��·���ϵ�Ǳ���ڱε� r
        float rz = DepthMap.SampleLevel(Sams_DepthMap, q_tex.xy, 0.0f).r;
        rz = NDCDepthToViewDepth(rz);

        float3 r = (rz / q.z) * q;

        // ���Ե� r �Ƿ��ڵ��� p
        float distZ = p.z - r.z;

        // dot(n, normalize(r - p)) ��ȡ(r - p / ||r - p||) �뷨�� n ֮��ļнǵ�����ֵ
        // �� n �� r - p ֮�������ֵΪ 0 ʱ, ˵����н�Ϊ 90 ��
        // ���, ����ֵ����Ϊ�ڱ�����, ��һ���̶��Ͽ��Է�ֹ��бƽ������ཻ����
        float dp = max(dot(n, normalize(r - p)), 0.0f);
		
        float occlusion = dp * OcclusionFunction(distZ);

        occlusionSum += occlusion;
    }

    occlusionSum /= nSampleCount;
    float access = 1.0f - occlusionSum;

    return saturate(pow(access, 6.0f));
}  
