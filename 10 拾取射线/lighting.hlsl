/**
 *	D3DFrame HLSL ������ɫ���ļ� 
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
	float Shininess;		// ����� = 1 - �ֲڶ�
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

/// �����������˥��
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

/// ������-ʯ��˽��Ʒ�
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightDir)
{
	float cosIncidentAngle = saturate(dot(normal, lightDir));
	
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0) * pow(f0, 5);

	return reflectPercent;
}

/// �����
float3 BlinnPhong(float3 lightStrength, float3 lightDir, float3 normal, float3 eyeDir, Material mat)
{
	const float m = mat.Shininess * 256.0f;
	float3 halfDir = normalize(eyeDir + lightDir);
	
	float roughnessFactor = (m + 8.0) * pow(max(dot(halfDir, normal), 0.0f), m) / 8.0f;	// �ֲڶ�����
	float3 fresnelFactor = SchlickFresnel(mat.vec3FresnelR0, halfDir, lightDir);		// ����������
	
	float3 vec3SpecAlbedo = roughnessFactor * fresnelFactor;
	vec3SpecAlbedo = vec3SpecAlbedo / (vec3SpecAlbedo + 1.0f);
	
	return (mat.vec4DiffuseAlbedo.rgb + vec3SpecAlbedo) * lightStrength;
}

/// ƽ�й�
// ������Ϣ:
// L: ��Դ����
// mat: ��������
// normal: Ƭ�η���
// eyeDir: �۲�����
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 eyeDir)
{
	float3 lightDir = -L.vec3Direction;
	
	float ndotl = max(dot(lightDir, normal), 0.0);
	float3 lightStrength = L.vec3Strength * ndotl;
	
	return BlinnPhong(lightStrength, lightDir, normal, eyeDir, mat);
}

/// ���Դ
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

/// �۹�Դ
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

// �����Ϲ�
// �ú���ʹ�õ��ĺ��� MAX_NUM_LIGHT, NUM_DIR_LIGHT, NUM_POINT_LIGHT, NUM_SPOT_LIGHT;
// MAX_NUM_LIGHT: ����Դ����; Ĭ��Ϊ 16
// NUM_DIR_LIGHT: ƽ�й�Դ����; Ĭ��Ϊ 1
// NUM_POINT_LIGHT: ���Դ����; Ĭ��Ϊ 0
// NUM_SPOT_LIGHT:  �۹�Դ����; Ĭ��Ϊ 0
// ��������:
// L[MAX_NUM_LIGHT]: ��Դ����; ���밴�� {[ƽ�й�...], [���Դ...], [�۹�Դ...]] | length < MAX_NUM_LIGHT} ��������
// mat: ������Ϣ
// pos: Ƭ��λ��
// normal: Ƭ�η���
// eyeDir: �۲�����
// shadowFactor: ��Ӱ����; ��ʹ������Ϊ(1, 1, 1)
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