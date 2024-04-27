/**
 *	D3DFrame HLSL 光照着色器文件 
*/

#ifndef MAX_NUM_LIGHT
#define MAX_NUM_LIGHT 16
#endif
#ifndef NUM_DIR_LIGHT
#define NUM_DIR_LIGHT 1
#endif
#ifndef NUM_POINT_LIGHT
#define NUM_POINT_LIGHT 0
#endif
#ifndef NUM_SPOT_LIGHT
#define NUM_SPOT_LIGHT 0
#endif

struct Material
{
	float4 vec4DiffuseAlbedo;
	float3 vec3FresnelR0;
	float Shininess;		// 光泽度 = 1 - 粗糙度
};

struct Light
{
	float3 vec3Strength;
	float nFalloffStart;
	float3 vec3Direction;
	float nFalloffEnd;
	float3 vec3Position;
	float nSpotPower;
};

/// 计算光照线性衰减
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

/// 菲涅尔-石里克近似法
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightDir)
{
	float cosIncidentAngle = saturate(dot(normal, lightDir));
	
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0) * pow(f0, 5);

	return reflectPercent;
}

/// 镜面光
float3 BlinnPhong(float3 lightStrength, float3 lightDir, float3 normal, float3 eyeDir, Material mat)
{
	const float m = mat.Shininess * 256.0f;
	float3 halfDir = normalize(eyeDir + lightDir);
	
	float roughnessFactor = (m + 8.0) * pow(max(dot(halfDir, normal), 0.0f), m) / 8.0f;	// 粗糙度因子
	float3 fresnelFactor = SchlickFresnel(mat.vec3FresnelR0, halfDir, lightDir);		// 菲涅尔因子
	
	float3 vec3SpecAlbedo = roughnessFactor * fresnelFactor;
	vec3SpecAlbedo = vec3SpecAlbedo / (vec3SpecAlbedo + 1.0f);
	
	return (mat.vec4DiffuseAlbedo.rgb + vec3SpecAlbedo) * lightStrength;
}

/// 平行光
// 参数信息:
// L: 光源数据
// mat: 材质数据
// normal: 片段法线
// eyeDir: 观察向量
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 eyeDir)
{
	float3 lightDir = -L.vec3Direction;
	
	float ndotl = max(dot(lightDir, normal), 0.0);
	float3 lightStrength = L.vec3Strength * ndotl;
	
	return BlinnPhong(lightStrength, lightDir, normal, eyeDir, mat);
}

/// 点光源
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 eyeDir)
{
	float3 lightDir = L.vec3Position - pos;
	
	float d = length(lightDir);
	
	if(d > L.nFalloffEnd)
		return 0.0;
	
	lightDir /= d;
	
	float ndotl = max(dot(lightDir, normal), 0.0);
	float3 lightStrength = L.vec3Strength * ndotl;
	
	float att = CalcAttenuation(d, L.nFalloffStart, L.nFalloffEnd);
	lightStrength *= att;
	
	return BlinnPhong(lightStrength, lightDir, normal, eyeDir, mat);
}

/// 聚光源
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float eyeDir)
{
	float3 lightDir = L.vec3Position - pos;
	
	float d = length(lightDir);
	
	if(d > L.nFalloffEnd)
		return 0.0;
	
	lightDir /= d;
	
	float ndotl = max(dot(lightDir, normal), 0.0);
	float3 lightStrength = L.vec3Strength * ndotl;
	
	float att = CalcAttenuation(d, L.nFalloffStart, L.nFalloffEnd);
	lightStrength *= att;
	
	float spotFactor = pow(max(dot(-lightDir, L.vec3Direction), 0.0), L.nSpotPower);
	lightStrength *= spotFactor;
	
	return BlinnPhong(lightStrength, lightDir, normal, eyeDir, mat);
}

// 计算混合光
// 该函数使用到的宏有 MAX_NUM_LIGHT, NUM_DIR_LIGHT, NUM_POINT_LIGHT, NUM_SPOT_LIGHT;
// MAX_NUM_LIGHT: 最大光源数量; 默认为 16
// NUM_DIR_LIGHT: 平行光源数量; 默认为 1
// NUM_POINT_LIGHT: 点光源数量; 默认为 0
// NUM_SPOT_LIGHT:  聚光源数量; 默认为 0
// 函数参数:
// L[MAX_NUM_LIGHT]: 光源数据; 必须按照 {[平行光...], [点光源...], [聚光源...]] | length < MAX_NUM_LIGHT} 进行排列
// mat: 材质信息
// pos: 片段位置
// normal: 片段法线
// eyeDir: 观察向量
// shadowFactor: 阴影因子; 不使用则设为(1, 1, 1)
float4 ComputeLighting(Light L[MAX_NUM_LIGHT], Material mat, float3 pos, float3 normal, float3 eyeDir, float3 shadowFactor)
{
	float3 result = 0.0;
	int i = 0;

#if (NUM_DIR_LIGHT > 0)
	for(i = 0; i < NUM_DIR_LIGHT; ++i)
		result += shadowFactor[i] * ComputeDirectionalLight(L[i], mat, normal, eyeDir);
#endif
#if (NUM_POINT_LIGHT > 0)
	for(i = NUM_DIR_LIGHT; i < NUM_DIR_LIGHT + NUM_POINT_LIGHT; ++i)
		result += ComputePointLight(L[i], mat, pos, normal, eyeDir);
#endif
#if (NUM_SPOT_LIGHT > 0)
	for(i = NUM_DIR_LIGHT + NUM_POINT_LIGHT; i < NUM_DIR_LIGHT + NUM_POINT_LIGHT + NUM_SPOT_LIGHT; ++i)
		result += ComputeSpotLight(L[i], mat, pos, normal, eyeDir);
#endif
	return float4(result, 0.0);
}