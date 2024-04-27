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

    std::unordered_map<std::string, Resource::Texture> Textures;    // ��������
    std::unordered_map<std::string, Light::Material> Materials;     // ��������
    std::unordered_map<std::string, Resource::MeshGeometry> Geos;   // ����������

    UINT nCBObjectCounter = 0;      // ��������������������
    UINT nCBMaterialCounter = 0;    // ���ʻ���������������
    UINT nSrvIndexCounter = 0;      // SRV ����������

    std::vector<TextureListItem> TextureList;   // ��������б�
    std::vector<MaterialListItem> MaterialList; // ����ע���б�
    std::vector<SrvListItem> SrvList;           // SRV ע���б�
    std::unordered_map<std::string, GeoListItem> GeoList;   // ����������ע���б�

private:
    Camera camera;
    POINT lastPos;

};