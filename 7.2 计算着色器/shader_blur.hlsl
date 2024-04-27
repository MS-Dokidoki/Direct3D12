/**********************************************************/
/*       最大半径为 5 高斯模糊的 HLSL 着色器代码文件         */
/**********************************************************/

cbuffer Settings: register(b0)
{
    int blurRadius;

    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

static const int MaxBlurRadius = 5;

#define N 256
#define CacheSize (N + 2 * MaxBlurRadius)

Texture2D Input: register(t0);
RWTexture2D<float4> Output: register(u0);

groupshared float4 Cache[CacheSize];

// 水平方向的高斯模糊
[numgroups(N, 1, 1)]
void CsMain_BlurHorz(int3 groupThreadID: SV_GROUPTHREADID, int3 dispatchThreadID: SV_DISPATCHTHREADID)
{
    float weights[11] = {w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10};

    // 将输入数据填写到共享内存中

    if(groupThreadID.x < blurRadius)
    {
        // 位于图像左侧边缘的像素对其采样像素进行钳制
        int x = max(dispatchThreadID.x - blurRadius, 0);
        Cache[groupThreadID.x] = Input[int2(x, dispatchThreadID.y)];
    }
    else if(groupThreadID.x >= N - blurRadius)
    {
        int x = min(dispatchThreadID.x + blurRadius, Input.Length.x - 1);
        Cache[groupThreadID.x + 2 * blurRadius] = Input[int2(x, dispatchThreadID.y)];
    }

	Cache[groupThreadID.x + blurRadius] = Input[min(dispatchThreadID.xy, Input.Length.xy - 1)];
	
	GroupMemoryBarrierWithGroupSync();
	
	float4 blurColor = float4(0, 0, 0, 0);
	for(int i = -blurRadius; i <= blurRadius; ++i)
	{
		int k = groupThreadID.x + blurRadius + i;
		blurColor += weights[i + blurRadius] * Cache[k];
	}
	
	Output[dispatchThreadID.xy] = blurColor;
}

[numgroups(1, N, 1)]
void CsMain_BlurVert(int3 groupThreadID: SV_GROUPTHREADID, int3 dispatchThreadID: SV_DISPATCHTHREADID)
{
	float weights[11] = {w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10};
	
	if(groupThreadID.y < blurRadius)
	{
		int y = max(dispatchThreadID.y - blurRadius, 0);
		Cache[groupThreadID.y] = Input[int2(dispatchThreadID.x, y)];
	}
	else if(groupThreadID.y >= N - blurRadius)
	{
		int y = min(Input.Length.y - 1, dispatchThreadID.y + blurRadius);
		Cache[groupThreadID.y + 2 * blurRadius] = Input[int2(dispatchThreadID.x, y)];
	}
	Cache[groupThreadID.y + blurRadius] = Input[min(dispatchThreadID.xy, Input.Length.xy - 1)];
	
	GroupMemoryBarrierWithGroupSync();
	
	float4 blurColor = float4(0, 0, 0, 0);
	for(int i = -blurRadius; i <= blurRadius; ++i)
	{
		int k = groupThreadID.y + blurRadius + i;
		blurColor += weights[i + blurRadius] * Cache[k];
	}
	
	Output[dispatchThreadID.xy] = blurColor;
}