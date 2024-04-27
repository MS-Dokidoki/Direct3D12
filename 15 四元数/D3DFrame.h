#pragma once
#include <D3DApp.h>
#include <Camera.h>
#include "ShadowMap.h"
#include "SsaoMap.h"

#define USE_FRAMERESOURCE           // Ĭ�Ͽ���֡��Դ
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

#define GetHillHeight(x, z) (0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z)))
#define RAND(a, b) a + rand() % ((b - a) + 1)
#define RANDF(a, b) a + ((float)(rand()) / (float)RAND_MAX) * (b - a)

union SkullModelVertex
{
    float v[6];
    struct
    {
        float Position[3];
        float Normal[3];
    };
};

union SkullModelIndex
{
    UINT16 i[3];
    struct
    {
        UINT16 first;
        UINT16 second;
        UINT16 thrid;
    };
};

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    XMFLOAT3 TangentU;
};

struct DebugVertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoords;
};

#define RENDER_TYPE_OPAQUE "Opaque"
#define RENDER_TYPE_TRANSPARENT "Transparent"
#define RENDER_TYPE_ALPHATESTED "AlphaTested"
#define RENDER_TYPE_DEBUG "Debug"
#define RENDER_TYPE_OPAQUE_SHADOW "Opaque_Shadow"
#define RENDER_TYPE_SKY_BOX "SkyBox"

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void CreateDescriptor();
    virtual void OnResize();
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    void OnMouseMove(WPARAM, int, int);
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);
    
    void InputProcess(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateScene(const GameTimer&);
    void UpdateMaterials(const GameTimer&);

    void LoadTextures();
    void BuildMaterials();
    void BuildGeometries();
    void BuildRenderItems();
    void BuildSRV();
    void BuildRootSignatures();
    void BuildPipelineStates();

    void DrawItems(ID3D12GraphicsCommandList*, std::vector<UINT>& );
private:
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PipelineStates;    // ��Ⱦ����
    ComPtr<ID3D12RootSignature> pRootSignature;                                 // ��ǩ��
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;                                      // ��ɫ����Դ��������

    std::vector<RenderItem> AllRenderItems;
    std::unordered_map<std::string, std::vector<UINT>> RenderItems;
    std::unordered_map<std::string, Resource::Texture> Textures;  // ��������
    std::unordered_map<std::string, Light::Material> Materials; // ��������
    std::unordered_map<std::string, Resource::MeshGeometry> Geos;   // ����������
   
private:
    bool bUseFrame = 0;
    POINT lastPos;

	Camera camera;

// ShadowMap
private:
    void UpdateShadowSpace();
    void DrawSceneToShadowMap();

private:
	UINT nSkyMapSrvIndex = 0;
    UINT nShadowMapSrvIndex = 0;
	UINT nNullMapSrvIndex = 0;
	
    ShadowMap stShadowMap;
    BoundingSphere stSceneBounds;
    ComPtr<ID3D12RootSignature> pShadowRootSignature;

    // Light
    float nLightRotationAngle = 0.0f;
    XMFLOAT3 vec3BaseLightDirection[3];
    XMFLOAT3 vec3RotatedLightDirection[3];
    XMFLOAT3 vec3MainLightPos;
    float nLightNearZ;
    float nLightFarZ;

    XMFLOAT4X4 matShadowView, matShadowProj, matShadowTransform;

// Ssao
private:
    void DrawSceneToNormalMap();

private:
    float weights[12] = {0};
    ComPtr<ID3D12RootSignature> pSsaoRootSignature;
    SsaoMap ssao;

    UINT nNormalMapSrvIndex = 0;

// Animation
private:
    static const UINT nSkullRenderItemIndex = 2;
    void DefineSkullAnimation();
    void UpdateAnimation(float dt);
    
private:
    D3DHelper::Animation::BoneAnimation SkullAnimation;

    float nAnimTimePos = 0;
};

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices);
