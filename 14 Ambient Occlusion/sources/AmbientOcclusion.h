#pragma once
#include <D3DApp.h>
#include <D3DHelper.h>
#include <Camera.h>
#include "Helper.h"
#include "project.h"

using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT3 TangentU;
    XMFLOAT2 TexCoords;
};

enum PipelineStateTypes
{
    PIPELINESTATE_TYPE_OPAQUE
};

enum RenderItemTypes
{
    RENDERITEM_TYPE_OPAQUE
};

class AmbientOcclusion: public D3DApp
{
public:
    AmbientOcclusion(HINSTANCE);
    ~AmbientOcclusion();

    virtual bool Initialize();
private:
    virtual void Update(const GameTimer&);
    virtual void Render(const GameTimer&);

    virtual void OnMouseDown(WPARAM, int, int);
    virtual void OnMouseUp(WPARAM, int, int);
    virtual void OnMouseMove(WPARAM, int, int);

private:
    void InputProcess(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateScene(const GameTimer&);
    void UpdateMaterials(const GameTimer&);

    void LoadTextures();
    void BuildGeometries();
    void BuildMaterialsAndSrv();
    void BuildRenderItems();

    void BuildPipelineStates();
    void BuildRootSignatures();

    void DrawItems(std::vector<UINT>&);

private:
    std::unordered_map<PipelineStateTypes, ComPtr<ID3D12PipelineState>> PipelineStates;
    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;

    std::vector<RenderItem> AllRenderItems;
    std::unordered_map<RenderItemTypes, std::vector<UINT>> RenderItems;

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
    Camera camera;
    POINT lastPos;

};