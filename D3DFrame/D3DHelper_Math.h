#pragma once
#ifndef _D3DHELPER_MATH_H
#define _D3DHELPER_MATH_H
#include "D3DBase.h"

namespace D3DHelper
{
	namespace MathHelper
    {
        static const float Infinity = FLT_MAX;
		
		DirectX::XMFLOAT4X4 Identity4x4(); 
		float RandomF(float minv, float maxv);
		float Random();
    };
};

#endif