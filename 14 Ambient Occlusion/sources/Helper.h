#pragma once
#include <D3DHelper.h>

using namespace D3DHelper;

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

void _BuildMaterials(std::vector<MaterialListItem>& MaterialList, 
                    std::vector<SrvListItem>& SrvList, 
                    std::unordered_map<std::string, Resource::Texture>& Textures, 
                    std::unordered_map<std::string, Light::Material>& Materials,
                    UINT& materialCounter,
                    UINT srvIndexCounter);

void _BuildGeometries(ID3D12Device* device, 
                     ID3D12GraphicsCommandList* cmdList, 
                     std::unordered_map<std::string, Resource::MeshGeometry>& Geos, 
                     std::unordered_map<std::string, GeoListItem>& GeoList);


void _LoadTextures(ID3D12Device* device, 
                  ID3D12GraphicsCommandList* cmdList, 
                  std::vector<TextureListItem>& TextureList, 
                  std::unordered_map<std::string, Resource::Texture>& Textures);

void _BuildSRV(ID3D12Device* device, 
              ID3D12DescriptorHeap* pSRVHeap, 
              std::vector<SrvListItem>& SrvList, 
              UINT descriptorSize);

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices);
void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC* sampler);