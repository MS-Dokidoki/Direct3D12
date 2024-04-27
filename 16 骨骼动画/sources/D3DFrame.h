#pragma once
#include <D3DApp.h>
#include <Camera.h>
#include "ShadowMap.h"
#include "SsaoMap.h"
#include "project.h"
#define USE_FRAMERESOURCE           // 默认开启帧资源

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

struct SkinnedVertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    XMFLOAT3 TangentU;
    XMFLOAT4 BoneWeights;
    UINT BoneIndices[4];
};

#define RENDER_TYPE_OPAQUE "Opaque"
#define RENDER_TYPE_TRANSPARENT "Transparent"
#define RENDER_TYPE_ALPHATESTED "AlphaTested"
#define RENDER_TYPE_DEBUG "Debug"
#define RENDER_TYPE_OPAQUE_SHADOW "Opaque_Shadow"
#define RENDER_TYPE_SKY_BOX "SkyBox"

struct TextureListItem
{
    std::string Name;
    std::wstring FileName;

    TextureListItem(LPCSTR lpszName, LPCWSTR lpszFileName): Name(lpszName), FileName(lpszFileName){}

    TextureListItem(LPCSTR lpszName, LPCSTR lpszFileNameA): Name(lpszName)
    {
        WCHAR Buffer[MAX_PATH] = {0};

        MultiByteToWideChar(CP_UTF8, 0, lpszFileNameA, -1, Buffer, MAX_PATH);

        FileName = Buffer;
    }
};

struct SrvListItem
{
    bool bSkip = 0;
    D3D12_SRV_DIMENSION ViewDimension;
    D3D12_RESOURCE_DESC ResourceDesc;
    ID3D12Resource* pResource;

    SrvListItem(D3D12_SRV_DIMENSION vd, D3D12_RESOURCE_DESC rd, ID3D12Resource* r): ViewDimension(vd), ResourceDesc(rd), pResource(r){}
    SrvListItem(bool skip): bSkip(skip){}
    SrvListItem(){}

};

struct GeoListItem
{
    bool bAutoRelease;
    void* pVertices;
    void* pIndices;
    UINT nVertexByteSize;
    UINT nVertexByteStride;
    UINT nIndexByteSize;
    DXGI_FORMAT emIndexFormat;

    std::unordered_map<std::string, Resource::SubmeshGeometry> Submeshes; 
};

struct MaterialListItem
{
    D3D12_SRV_DIMENSION ViewDimension;
    DirectX::XMFLOAT4X4 matTransform;
    DirectX::XMFLOAT4 vec4DiffuseAlbedo;
    DirectX::XMFLOAT3 vec3FresnelR0;
    float nRoughness;
    std::string Name;
    bool bDiffIsNull;
    std::string lpszDiffuseMap;
    bool bNormIsNull;
    std::string lpszNormalMap;

    MaterialListItem(D3D12_SRV_DIMENSION viewDimension, LPCSTR name, DirectX::XMFLOAT4 diffuseAlbedo,
                    DirectX::XMFLOAT3 fresnelR0, DirectX::XMFLOAT4X4& transform,
                    float roughness, LPCSTR diffuseMap, LPCSTR normalMap)
    :ViewDimension(viewDimension), Name(name), 
    vec4DiffuseAlbedo(diffuseAlbedo), vec3FresnelR0(fresnelR0), matTransform(transform), 
    nRoughness(roughness), bDiffIsNull(0), bNormIsNull(0)
    {
        if(!diffuseMap)
            bDiffIsNull = 1;
        else
            lpszDiffuseMap = diffuseMap;

        if(!normalMap)
            bNormIsNull = 1;
        else
            lpszNormalMap = normalMap;
    }

    MaterialListItem(D3D12_SRV_DIMENSION viewDimension, std::string& name, DirectX::XMFLOAT4 diffuseAlbedo,
                    DirectX::XMFLOAT3 fresnelR0, DirectX::XMFLOAT4X4& transform,
                    float roughness, std::string& diffuseMap, std::string& normalMap)
    :ViewDimension(viewDimension), Name(name),
    vec4DiffuseAlbedo(diffuseAlbedo), vec3FresnelR0(fresnelR0), matTransform(transform), 
    nRoughness(roughness), lpszDiffuseMap(diffuseMap), lpszNormalMap(normalMap),
    bDiffIsNull(0), bNormIsNull(0)
    {}
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    ~D3DFrame();
    
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
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PipelineStates;    // 渲染管线
    ComPtr<ID3D12RootSignature> pRootSignature;                                 // 根签名
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;                                      // 着色器资源描述符堆

    // std::vector<RenderItem> AllRenderItems;                         
    C_VECTOR(RenderItem) AllRenderItems;
    std::unordered_map<std::string, std::vector<UINT>> RenderItems;
    std::unordered_map<std::string, Resource::Texture> Textures;    // 纹理数据
    std::unordered_map<std::string, Light::Material> Materials;     // 材质数据
    std::unordered_map<std::string, Resource::MeshGeometry> Geos;   // 几何体数据
    
    UINT nCBObjectCounter = 0;      // 常量缓冲区索引计数器
    UINT nCBMaterialCounter = 0;    // 材质缓冲区索引计数器
    UINT nSrvIndexCounter = 0;      // SRV 索引计数器

    std::vector<TextureListItem> TextureList;   // 纹理加载列表
    std::vector<MaterialListItem> MaterialList; // 材质注册列表
    std::vector<SrvListItem> SrvList;           // SRV 注册列表
    std::unordered_map<std::string, GeoListItem> GeoList;   // 几何体数据注册列表
private:        
	Camera camera;  // 主摄像机 
    POINT lastPos; 
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

    float nLightRotationAngle = 0.0f;
    XMFLOAT3 vec3BaseLightDirection[3];
    XMFLOAT3 vec3RotatedLightDirection[3];
    XMFLOAT3 vec3MainLightPos;
    float nLightNearZ;
    float nLightFarZ;

    XMFLOAT4X4 matShadowView, matShadowProj, matShadowTransform;

private:
    void DrawSceneToNormalMap();

private:
    float weights[12] = {0};
    ComPtr<ID3D12RootSignature> pSsaoRootSignature;
    SsaoMap ssao;

    UINT nNormalMapSrvIndex = 0;

private:
    void LoadModels();
    void UpdateAnimations(const GameTimer&);

private:
// 因为该项目只用一个实例使用模型, 故直接定义成员变量
    SkinnedInstance Soldier;
    D3DHelper::Animation::SkinnedAnimation SoldierSkinned;

    UINT nSoldierMatCount;
};

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices);
