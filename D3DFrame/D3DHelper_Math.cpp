#include "D3DHelper.h"
using namespace DirectX;
using namespace D3DHelper;

XMFLOAT4X4 MathHelper::Identity4x4()
{
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

float MathHelper::RandomF(float minv, float maxv)
{
	return minv + Random() * (maxv - minv);
}

float MathHelper::Random()
{
	return (float)rand() / (float)RAND_MAX;
}