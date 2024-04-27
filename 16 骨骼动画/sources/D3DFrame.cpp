/**
 * 该项目对 D3DFrame 的部分代码进行了颠覆式的替换
 * 本项目将纹理的加载，材质的定义以及着色器资源视图的创建进行了进一步的封装
 * 同时进一步抽象了几何数据上传至 GPU 的过程
 * 2023.9.17
 * 引入了《DirectX 12 3D 游戏开发实战》中的 LoadM3d 和 SkinnedData 模块
 * 成功地实现了骨骼动画的效果
 * 同时 发现了原有模块在读取 .m3d 文件上出现问题 并计划进行修复
*/

#define D3D12_DYNAMIC_INDEX
#define D3D12_SKINNED
#include "D3DFrame.h"
#include "M3dLoader.h"

#define RENDER_TYPE_NORMAL "Normal"
#define RENDER_TYPE_SSAO "Ssao"
#define RENDER_TYPE_SSAO_BLUR "SsaoBlur"
#define RENDER_TYPE_SKINNED_OPAQUE "SkinnedOpaque"

void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC* sampler);

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{
    stSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    stSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
    
    vec3BaseLightDirection[0] = { 0.57735f, -0.57735f, 0.57735f }; 
    vec3BaseLightDirection[1] = { -0.57735f, -0.57735f, 0.57735f };
    vec3BaseLightDirection[2] = { 0.0f, -0.707f, -0.707f };
}

D3DFrame::~D3DFrame()
{
    VectorFree(AllRenderItems);
}

bool D3DFrame::Initialize()
{
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;     
#endif
    if(!D3DApp::Initialize())
        return 0;

    camera.LookAt(XMFLOAT3(0.0f, 0.0f, -5.0f), {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    
    stShadowMap.Init(pD3dDevice.Get(), cxClient, cyClient);

    // 重置命令列表, 准备写入命令
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    LoadModels();
    LoadTextures();
    BuildGeometries();

    // 关闭命令列表, 命令 GPU 执行
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* cmdLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    
    // 在 GPU 执行期间, CPU 也不能停
    BuildMaterials();
    nSkyMapSrvIndex = SrvList.size() - 1;
    nShadowMapSrvIndex = nSkyMapSrvIndex + 1;

    SrvList.push_back(SrvListItem(1));  // Shadow Map
    
    BuildSRV();
    BuildRenderItems();
    BuildRootSignatures();
    BuildPipelineStates();

    
    stShadowMap.BuildDescriptor(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(pSRVHeap->GetCPUDescriptorHandleForHeapStart(), nShadowMapSrvIndex, nCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(pSRVHeap->GetGPUDescriptorHandleForHeapStart(), nShadowMapSrvIndex,  nCbvSrvUavDescriptorSize),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(pDSVHeap->GetCPUDescriptorHandleForHeapStart(), 1, nDSVDescriptorSize)
    );

    // 等待 GPU 的命令完成
    FlushCommandQueue();

    // clear memory
    for(auto& item: Textures)
        item.second.pUploader = nullptr;
    for(auto& item: Geos)
        item.second.DisposeUploader();
    
    return 1;
}

void D3DFrame::LoadModels()
{
// Skull Model
    Vertex* vertices;
    UINT* indices;

    UINT nVertexByteSize, nIndexByteSize;
    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> skullIndices;
    
    LoadSkullModel(skullVertices, skullIndices);

    XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
    XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

    nVertexByteSize = skullVertices.size() * sizeof(Vertex);
    nIndexByteSize = skullIndices.size() * sizeof(SkullModelIndex);

    vertices = (Vertex*)malloc(nVertexByteSize);
    indices = (UINT*)malloc(nIndexByteSize);
    CopyMemory(indices, &skullIndices[0], nIndexByteSize);

    for(UINT i = 0; i < skullVertices.size(); ++i)
    {
        vertices[i].Position = XMFLOAT3(skullVertices[i].Position);
        vertices[i].Normal = XMFLOAT3(skullVertices[i].Normal);
     
        XMVECTOR P = XMLoadFloat3(&vertices[i].Position);
        XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR T;

        if(fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
        {
            T = XMVector3Normalize(XMVector3Cross(up, N));
            XMStoreFloat3(&vertices[i].TangentU, T);
        }
        else
        {
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            T = XMVector3Normalize(XMVector3Cross(N, up));
        }
        vertices[i].TexCoords = {0.0f, 0.0f};
        XMStoreFloat3(&vertices[i].TangentU, T);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));

    GeoListItem skull;
    skull.bAutoRelease = 1;
    skull.pVertices = vertices;
    skull.pIndices = indices;
    skull.nVertexByteStride = sizeof(Vertex);
    skull.emIndexFormat = DXGI_FORMAT_R16_UINT;
    skull.nVertexByteSize = nVertexByteSize;
    skull.nIndexByteSize = nIndexByteSize;
    
    Resource::SubmeshGeometry submesh;
    submesh.Bounds = bounds;
    submesh.nBaseVertexLocation = 0;
    submesh.nStartIndexLocation = 0;
    submesh.nIndexCount = skullIndices.size() * 3;
    
    skull.Submeshes["main"] = submesh;

    GeoList["Skull"] = skull;

// Solider
    SkinnedVertex* pM3dVertices;
    UINT* pM3dIndices;

	std::vector<M3dLoader::M3dSkinnedVertex> m3dVertices;
    std::vector<M3dLoader::M3dSubset> m3dSubsets;
    std::vector<M3dLoader::M3dMaterial> m3dMaterials;
    std::vector<UINT> m3dIndices;
	
	M3dLoader::LoadM3dFile(PROJECT_ROOT("/Resources/soldier.m3d"), m3dVertices, m3dIndices, m3dSubsets, m3dMaterials, SoldierSkinned);

    nSoldierMatCount = m3dMaterials.size();
    nIndexByteSize = sizeof(UINT) * m3dIndices.size();
    nVertexByteSize = sizeof(SkinnedVertex) * m3dVertices.size();

    pM3dVertices = (SkinnedVertex*)malloc(nVertexByteSize);
    pM3dIndices = (UINT*)malloc(nIndexByteSize);

    CopyMemory(pM3dIndices, &m3dIndices[0], nIndexByteSize);


    for(UINT i = 0; i < m3dVertices.size(); ++i)
    {
        pM3dVertices[i].Position = m3dVertices[i].vec3Position;
        pM3dVertices[i].Normal = m3dVertices[i].vec3Normal;
        pM3dVertices[i].TangentU = m3dVertices[i].vec3TangentU;
        pM3dVertices[i].TexCoords = m3dVertices[i].vec2TexCoords;
        pM3dVertices[i].BoneWeights = m3dVertices[i].vec4BoneWeights;
        CopyMemory(pM3dVertices[i].BoneIndices, m3dVertices[i].vec4BoneIndices, 4 * sizeof(UINT));

	}

    // Geometry
    GeoListItem soldier;
    soldier.bAutoRelease = 1;
    soldier.nIndexByteSize = nIndexByteSize;
    soldier.nVertexByteSize = nVertexByteSize;
    soldier.nVertexByteStride = sizeof(SkinnedVertex);
    soldier.emIndexFormat = DXGI_FORMAT_R32_UINT;
	soldier.pIndices = pM3dIndices;
    soldier.pVertices = pM3dVertices;
    
    for(UINT i = 0; i < m3dSubsets.size(); ++i)
    {
        Resource::SubmeshGeometry submesh;
        submesh.nIndexCount = m3dSubsets[i].nFaceCount * 3;
        submesh.nStartIndexLocation = m3dSubsets[i].nFaceStart * 3;
        submesh.nBaseVertexLocation = 0;
        soldier.Submeshes["sm_" + std::to_string(i)] = submesh;
    }

    GeoList["Soldier"] = soldier;
    XMFLOAT4X4 mat = MathHelper::Identity4x4();

    // Materials && Textures
    for(UINT i = 0; i < nSoldierMatCount; ++i)
    {
		std::string matName = "Soldier_" + std::to_string(i);
        std::string diffName = m3dMaterials[i].Name + "_diff";
        std::string normName = m3dMaterials[i].Name + "_norm";
		
        TextureList.push_back(TextureListItem(diffName.c_str(), (L"Resources\\" + m3dMaterials[i].DiffuseMap).c_str()));
        TextureList.push_back(TextureListItem(normName.c_str(), (L"Resources\\" + m3dMaterials[i].NormalMap).c_str()));
        
		MaterialList.push_back(MaterialListItem(
            D3D12_SRV_DIMENSION_TEXTURE2D,
            matName, 
            XMFLOAT4(m3dMaterials[i].vec3DiffuseAlbedo.x, m3dMaterials[i].vec3DiffuseAlbedo.y, m3dMaterials[i].vec3DiffuseAlbedo.z, 1.0f),
            m3dMaterials[i].vec3FresnelR0,
            mat,
            m3dMaterials[i].nRoughness, diffName, normName
        ));
    }

    Soldier.ClipName = "Take1";
    Soldier.fTimePos = 0.0f;
    Soldier.Skinned = &SoldierSkinned;
    Soldier.matFinalTransforms.resize(SoldierSkinned.BoneCount());
}

void D3DFrame::LoadTextures()
{
    Resource::Texture tex;

    TextureList.push_back(TextureListItem("wood", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/plywood.dds"));
    TextureList.push_back(TextureListItem("wood_normal", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/plywood_normal.dds"));
    TextureList.push_back(TextureListItem("brick", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/bricks.dds"));
    TextureList.push_back(TextureListItem("brick_normal", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/bricks_normal.dds"));
    TextureList.push_back(TextureListItem("tile", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/tile.dds"));
    TextureList.push_back(TextureListItem("tile_normal", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/tile_normal.dds"));
    TextureList.push_back(TextureListItem("default", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/white1x1.dds"));
    TextureList.push_back(TextureListItem("default_normal", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/default_normal.dds"));
    TextureList.push_back(TextureListItem("grassCube", L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/grasscube1024.dds"));
    
    for(auto& item: TextureList)
    {
        tex.Name = item.Name;
        tex.FileName = item.FileName;
        tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), item.FileName.c_str(), &tex.pUploader);
        
        Textures[tex.Name] = tex;
    }

    TextureList.clear();
}

void D3DFrame::BuildGeometries()
{
// Main Geo
    Resource::MeshGeometry Main;
    Geometry::Mesh box = Geometry::GeometryGenerator::CreateCube(1.0f, 1.0f, 1.0f);
    Geometry::Mesh cylinder = Geometry::GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
    Geometry::Mesh sphere = Geometry::GeometryGenerator::CreateSphere(0.5f, 20, 20);
    Geometry::Mesh grid = Geometry::GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
    Vertex quad[] = {
        {XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
        {XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)}
    };

    UINT quadIndex[] = {
        0, 1, 3,
        3, 1, 2
    };

    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.vertices.size();
    UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.vertices.size();

    UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.indices.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.indices.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.indices.size();
    UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.indices.size();

    Resource::SubmeshGeometry boxSubmesh;
    boxSubmesh.nIndexCount = box.indices.size();
    boxSubmesh.nStartIndexLocation = boxIndexOffset;
    boxSubmesh.nBaseVertexLocation = boxVertexOffset;
    
    Resource::SubmeshGeometry gridSubmesh;
    gridSubmesh.nIndexCount = grid.indices.size();
    gridSubmesh.nStartIndexLocation = gridIndexOffset;
    gridSubmesh.nBaseVertexLocation = gridVertexOffset;

    Resource::SubmeshGeometry sphereSubmesh;
    sphereSubmesh.nIndexCount = sphere.indices.size();
    sphereSubmesh.nStartIndexLocation = sphereIndexOffset;
    sphereSubmesh.nBaseVertexLocation = sphereVertexOffset;

    Resource::SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.nIndexCount = cylinder.indices.size();
    cylinderSubmesh.nStartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.nBaseVertexLocation = cylinderVertexOffset;

    Resource::SubmeshGeometry quadSubmesh;
    quadSubmesh.nIndexCount = 6;
    quadSubmesh.nStartIndexLocation = quadIndexOffset;
    quadSubmesh.nBaseVertexLocation = quadVertexOffset;

    UINT vertexTotal = quadVertexOffset /*+ quad.vertices.size()*/ + 4;
    std::vector<Vertex> vertices(vertexTotal);

    UINT k = 0;
    for(UINT i = 0; i < box.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = box.vertices[i].vec3Normal;
        vertices[k].Position = box.vertices[i].vec3Position;
        vertices[k].TexCoords = box.vertices[i].vec2TexCoords;
        vertices[k].TangentU = box.vertices[i].vec3TangentU;
    }

    for(UINT i = 0; i < grid.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = grid.vertices[i].vec3Normal;
        vertices[k].Position = grid.vertices[i].vec3Position;
        vertices[k].TexCoords = grid.vertices[i].vec2TexCoords;
        vertices[k].TangentU = grid.vertices[i].vec3TangentU;
    }
    
    for(UINT i = 0; i < sphere.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = sphere.vertices[i].vec3Normal;
        vertices[k].Position = sphere.vertices[i].vec3Position;
        vertices[k].TexCoords = sphere.vertices[i].vec2TexCoords;
        vertices[k].TangentU = sphere.vertices[i].vec3TangentU;
    }
    
    for(UINT i = 0; i < cylinder.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = cylinder.vertices[i].vec3Normal;
        vertices[k].Position = cylinder.vertices[i].vec3Position;
        vertices[k].TexCoords = cylinder.vertices[i].vec2TexCoords;
        vertices[k].TangentU = cylinder.vertices[i].vec3TangentU;
    }

    for(UINT i = 0; i < _countof(quad); ++i, ++k)
    {
        vertices[k].Normal = quad[i].Normal;
        vertices[k].Position = quad[i].Position;
        vertices[k].TexCoords = quad[i].TexCoords;
        vertices[k].TangentU = quad[i].TangentU;
    }

    std::vector<UINT16> indices;

    indices.insert(std::end(indices), std::begin(box.indices), std::end(box.indices));
    indices.insert(std::end(indices), std::begin(grid.indices), std::end(grid.indices));
    indices.insert(std::end(indices), std::begin(sphere.indices), std::end(sphere.indices));
    indices.insert(std::end(indices), std::begin(cylinder.indices), std::end(cylinder.indices));
    indices.insert(std::end(indices), std::begin(quadIndex), std::end(quadIndex));

    UINT vertexBytesSize = vertexTotal * sizeof(Vertex);
    UINT indexBytesSize = indices.size() * sizeof(UINT16);

    Main.Name = "Main";
    Main.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &Main.pUploaderVertexBuffer);
    Main.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], indexBytesSize, &Main.pUploaderIndexBuffer);

    Main.nIndexBufferByteSize = indexBytesSize;
    Main.nVertexBufferByteSize = vertexBytesSize;
    Main.nVertexByteStride = sizeof(Vertex);

    Main.DrawArgs["box"] = boxSubmesh;
    Main.DrawArgs["grid"] = gridSubmesh;
    Main.DrawArgs["sphere"] = sphereSubmesh;
    Main.DrawArgs["cylinder"] = cylinderSubmesh;
    Main.DrawArgs["quad"] = quadSubmesh;

    Geos["Main"] = Main;

// Others Geo
    for(auto& item: GeoList)
    {
        Resource::MeshGeometry geo;

        geo.Name = item.first;

        geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), item.second.pVertices, item.second.nVertexByteSize, &geo.pUploaderVertexBuffer);
        geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), item.second.pIndices, item.second.nIndexByteSize, &geo.pUploaderIndexBuffer);

        if(item.second.bAutoRelease)
        {
            free(item.second.pVertices);
            free(item.second.pIndices);
        }

        geo.nVertexBufferByteSize = item.second.nVertexByteSize;
        geo.nVertexByteStride = item.second.nVertexByteStride;
        geo.nIndexBufferByteSize = item.second.nIndexByteSize;
        geo.emIndexFormat = item.second.emIndexFormat;
        geo.DrawArgs = item.second.Submeshes;

        Geos[geo.Name] = geo;
    }

    GeoList.clear();
}   

void D3DFrame::BuildMaterials()
{
    XMFLOAT4X4 matIdentity = MathHelper::Identity4x4();

    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURE2D, 
        "wood", 
        {1.0f, 1.0f, 1.0f, 1.0f},
        {.05f, .05f, .05f},
        matIdentity,
        0.1f,
        "wood", "wood_normal"
    ));

    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURE2D,
        "brick",
        {1.0f, 1.0f, 1.0f, 1.0f},
        {.02f, .02f, .02f},
        matIdentity,
        0.1f,
        "brick", "brick_normal"
    ));

    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURE2D,
        "tile",
        {1.0f, 1.0f, 1.0f, 1.0f},
        {.02f, .02f, .02f},
        matIdentity,
        0.3f,
        "tile", "tile_normal"
    ));

    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURE2D,
        "skull",
        {.3f, .3f, .3f, 1.0f},
        {.6f, .6f, .6f},
        matIdentity,
        0.2f, 
        "default", "default_normal"
    ));

    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURECUBE,
        "sky",
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        matIdentity,
        1.0f,
        "grassCube", NULL
    ));

    for(auto& item: MaterialList)
    {
        Light::Material mat;
        mat.iFramesDirty = 3;
        mat.Name = item.Name;
        mat.matTransform = item.matTransform;
        mat.vec4DiffuseAlbedo = item.vec4DiffuseAlbedo;
        mat.vec3FresnelR0 = item.vec3FresnelR0;
        mat.nRoughness = item.nRoughness;
        mat.nCBMaterialIndex = nCBMaterialCounter++;

        std::unordered_map<std::string, Resource::Texture>::iterator diffItor, normItor;
        SrvListItem sli;

        if(!item.bDiffIsNull)
        {
            diffItor = Textures.find(item.lpszDiffuseMap);
            if(diffItor != Textures.end())
            {
                mat.nDiffuseSrvHeapIndex= nSrvIndexCounter++;

                sli.ViewDimension = item.ViewDimension;
                sli.pResource = diffItor->second.pResource.Get();
                sli.ResourceDesc = sli.pResource->GetDesc();
                SrvList.push_back(sli);
            }
            else
                goto a;
        }
        else
        {
a:          mat.nDiffuseSrvHeapIndex = -1;
        }
        
        if(!item.bNormIsNull)
        {
            normItor = Textures.find(item.lpszNormalMap);
            if(normItor != Textures.end())
            {
                mat.nNormalSrvHeapIndex = nSrvIndexCounter++;

                sli.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                sli.pResource = normItor->second.pResource.Get();
                sli.ResourceDesc = sli.pResource->GetDesc();
                SrvList.push_back(sli);
            }
            else
                goto b;
        }
        else
b:          mat.nNormalSrvHeapIndex = -1;
        
        Materials[mat.Name] = mat;
    }

    MaterialList.clear();

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
        Init(pD3dDevice.Get(), sizeof(Constant::MaterialConstant), Materials.size());

}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry info;
    AllRenderItems = VectorCreateEx(RenderItem);

    RenderItem sky;
    sky.iFramesDirty = 3;
    sky.nCBObjectIndex = nCBObjectCounter++;
    sky.pMaterial = &Materials["sky"];
    sky.pGeo = &Geos["Main"];
    sky.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = sky.pGeo->DrawArgs["box"];
    sky.nIndexCount = info.nIndexCount;
    sky.nStartIndexLocation = info.nStartIndexLocation;
    sky.nBaseVertexLocation = info.nBaseVertexLocation;

    XMStoreFloat4x4(&sky.matWorld, XMMatrixScaling(2000.0f, 2000.0f, 2000.0f));
    sky.matTexTransform = MathHelper::Identity4x4();

    // AllRenderItems.push_back(sky);
    VectorPushBackEx(AllRenderItems, sky);
    RenderItems[RENDER_TYPE_SKY_BOX].push_back(0);

    RenderItem debug;
    debug.iFramesDirty = 3;
    debug.nCBObjectIndex = nCBObjectCounter++;
    debug.pMaterial = &Materials["skull"];
    debug.pGeo = &Geos["Main"];
    debug.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = debug.pGeo->DrawArgs["quad"];
    debug.nIndexCount = info.nIndexCount;
    debug.nStartIndexLocation = info.nStartIndexLocation;
    debug.nBaseVertexLocation = info.nBaseVertexLocation;
    
    debug.matTexTransform = MathHelper::Identity4x4();
    debug.matWorld = MathHelper::Identity4x4();

    // AllRenderItems.push_back(debug);
    VectorPushBackEx(AllRenderItems, debug);
    RenderItems[RENDER_TYPE_DEBUG].push_back(1);

    RenderItem skull;
    skull.iFramesDirty = 3;
    skull.nCBObjectIndex = nCBObjectCounter++;
    skull.pMaterial = &Materials["skull"];
    skull.pGeo = &Geos["Skull"];
    skull.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = skull.pGeo->DrawArgs["main"];
    skull.nIndexCount = info.nIndexCount;
    skull.nStartIndexLocation = info.nStartIndexLocation;
    skull.nBaseVertexLocation = info.nBaseVertexLocation;

    XMStoreFloat4x4(&skull.matWorld, XMMatrixScaling(.5f, .5f, .5f) * XMMatrixTranslation(0.0f, 2.5f, 0.0f));
    skull.matTexTransform = MathHelper::Identity4x4();
    // AllRenderItems.push_back(skull);
    VectorPushBackEx(AllRenderItems, skull);

    RenderItem box;
    box.iFramesDirty = 3;
    box.nCBObjectIndex = nCBObjectCounter++;
    box.pMaterial = &Materials["wood"];
    box.pGeo = &Geos["Main"];
    box.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    
    info = box.pGeo->DrawArgs["box"];
    box.nIndexCount = info.nIndexCount;
    box.nStartIndexLocation = info.nStartIndexLocation;
    box.nBaseVertexLocation = info.nBaseVertexLocation;
    XMStoreFloat4x4(&box.matWorld, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    XMStoreFloat4x4(&box.matTexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

    // AllRenderItems.push_back(box);
    VectorPushBackEx(AllRenderItems, box);

    RenderItem grid;
    grid.iFramesDirty = 3;
    grid.nCBObjectIndex = nCBObjectCounter++;
    grid.pMaterial = &Materials["tile"]; 
    grid.pGeo = &Geos["Main"];
    grid.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = grid.pGeo->DrawArgs["grid"];
    grid.nIndexCount = info.nIndexCount;
    grid.nBaseVertexLocation = info.nBaseVertexLocation;
    grid.nStartIndexLocation = info.nStartIndexLocation;
    grid.matWorld = MathHelper::Identity4x4();
    XMStoreFloat4x4(&grid.matTexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
    
    // AllRenderItems.push_back(grid);
    VectorPushBackEx(AllRenderItems, grid);

    XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    UINT& objIndex = nCBObjectCounter;
    for(int i = 0; i < 5; ++i)
    {
        RenderItem lCI, rCI;    // Cylinder Item 
        RenderItem lSI, rSI;    // Sphere Item

        XMMATRIX lCW = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX rCW = XMMatrixTranslation(5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX lSW = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rSW = XMMatrixTranslation(5.0f, 3.5f, -10.0f + i * 5.0f);

        lCI.iFramesDirty = 3;
        lCI.nCBObjectIndex = objIndex++;
        lCI.pMaterial = &Materials["brick"];
        lCI.pGeo = &Geos["Main"];
        lCI.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        info = lCI.pGeo->DrawArgs["cylinder"];
        lCI.nIndexCount = info.nIndexCount;
        lCI.nBaseVertexLocation = info.nBaseVertexLocation;
        lCI.nStartIndexLocation = info.nStartIndexLocation;
        XMStoreFloat4x4(&lCI.matWorld, lCW);
        XMStoreFloat4x4(&lCI.matTexTransform, brickTexTransform);

        rCI.iFramesDirty = 3;
        rCI.nCBObjectIndex = objIndex++;
        rCI.pMaterial = &Materials["brick"];
        rCI.pGeo = &Geos["Main"];
        rCI.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        info = rCI.pGeo->DrawArgs["cylinder"];
        rCI.nIndexCount = info.nIndexCount;
        rCI.nBaseVertexLocation = info.nBaseVertexLocation;
        rCI.nStartIndexLocation = info.nStartIndexLocation;
        XMStoreFloat4x4(&rCI.matWorld, rCW);
        XMStoreFloat4x4(&rCI.matTexTransform, brickTexTransform);
        
        lSI.iFramesDirty = 3;
        lSI.nCBObjectIndex = objIndex++;
        lSI.pMaterial = &Materials["brick"];
        lSI.pGeo = &Geos["Main"];
        lSI.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        info = lSI.pGeo->DrawArgs["sphere"];
        lSI.nIndexCount = info.nIndexCount;
        lSI.nBaseVertexLocation = info.nBaseVertexLocation;
        lSI.nStartIndexLocation = info.nStartIndexLocation;
        lSI.matTexTransform = MathHelper::Identity4x4();
        XMStoreFloat4x4(&lSI.matWorld, lSW);

        
        rSI.iFramesDirty = 3;
        rSI.nCBObjectIndex = objIndex++;
        rSI.pMaterial = &Materials["brick"];
        rSI.pGeo = &Geos["Main"];
        rSI.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        info = rSI.pGeo->DrawArgs["sphere"];
        rSI.nIndexCount = info.nIndexCount;
        rSI.nBaseVertexLocation = info.nBaseVertexLocation;
        rSI.nStartIndexLocation = info.nStartIndexLocation;
        rSI.matTexTransform = MathHelper::Identity4x4();
        XMStoreFloat4x4(&rSI.matWorld, rSW);
        /*
        AllRenderItems.push_back(lCI);
        AllRenderItems.push_back(rCI);
        AllRenderItems.push_back(lSI);
        AllRenderItems.push_back(rSI);
        */

        VectorPushBackEx(AllRenderItems, lCI);
        VectorPushBackEx(AllRenderItems, rCI);
        VectorPushBackEx(AllRenderItems, lSI);
        VectorPushBackEx(AllRenderItems, rSI);

    }
    
    for(UINT i = 2; i < /*AllRenderItems.size()*/ VectorSize(AllRenderItems); ++i)
        RenderItems[RENDER_TYPE_OPAQUE].push_back(i);

// Soldier
    UINT nIndexBegin = VectorSize(AllRenderItems);
    RenderItem model;
    
    XMMATRIX modelScale = XMMatrixScaling(0.05f, 0.05f, -0.05f);
    XMMATRIX modelRot = XMMatrixRotationY(XM_PI);
    XMMATRIX modelOffset = XMMatrixTranslation(0.0f, 0.0f, -5.0f);
    XMStoreFloat4x4(&model.matWorld, modelScale*modelRot*modelOffset);
    model.matTexTransform = MathHelper::Identity4x4();

    model.iFramesDirty = 3;
    model.pGeo = &Geos["Soldier"];
    model.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    model.nSkinnedCBIndex = 0;
    model.Skinned = &Soldier;

    for(UINT i = 0; i < 5; ++i)
    {
        std::string submeshName = "sm_" + std::to_string(i);

        Resource::SubmeshGeometry& subset = model.pGeo->DrawArgs[submeshName];

        model.nCBObjectIndex = nCBObjectCounter++;
        model.pMaterial = &Materials["Soldier_" + std::to_string(i)];
        model.nIndexCount = subset.nIndexCount;
        model.nStartIndexLocation = subset.nStartIndexLocation;
        model.nBaseVertexLocation = subset.nBaseVertexLocation;

        VectorPushBackEx(AllRenderItems, model);
        RenderItems[RENDER_TYPE_SKINNED_OPAQUE].push_back(nIndexBegin++);
    }
}

void D3DFrame::BuildRootSignatures()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
    {
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 2, VectorSize(AllRenderItems));

        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_SSAO)
        .Init(pD3dDevice.Get(), CONSTANT_VALUE::nCBSsaoByteSize, 1);
        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_SKINNED)
        .Init(pD3dDevice.Get(), CONSTANT_VALUE::nCBSkinnedByteSize, 1);
    }

    CD3DX12_DESCRIPTOR_RANGE range[2];
	CD3DX12_ROOT_PARAMETER params[6];
    CD3DX12_STATIC_SAMPLER_DESC sampler[7];

    GetStaticSampler(sampler);
	
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 48, 3, 0);
    
    params[0].InitAsConstantBufferView(0);          // Object
    params[1].InitAsConstantBufferView(1);          // Skinned
    params[2].InitAsConstantBufferView(2);          // World
    params[3].InitAsShaderResourceView(0, 1);       // Materials
    params[4].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);  // SkyMap ShadowMap
    params[5].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);  // Textures

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(6, params, 7, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serialize, error;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialize, &error), &error);

    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, serialize->GetBufferPointer(), serialize->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    CD3DX12_DESCRIPTOR_RANGE ssao_range[2];
    CD3DX12_ROOT_PARAMETER ssao_params[4];
    CD3DX12_ROOT_SIGNATURE_DESC ssao_desc;
    CD3DX12_STATIC_SAMPLER_DESC ssao_sampler[4];

    ssao_sampler[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    ssao_sampler[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    ssao_sampler[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0, 0.0f, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
    ssao_sampler[3].Init(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);

    ssao_range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
    ssao_range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

    ssao_params[0].InitAsConstantBufferView(0);
    ssao_params[1].InitAsConstants(1, 1, 0);
    ssao_params[2].InitAsDescriptorTable(1, &ssao_range[0], D3D12_SHADER_VISIBILITY_PIXEL);
    ssao_params[3].InitAsDescriptorTable(1, &ssao_range[1], D3D12_SHADER_VISIBILITY_PIXEL);
    
    ssao_desc.Init(4, ssao_params, 4, ssao_sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> ssao_serialize, ssao_error;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&ssao_desc, D3D_ROOT_SIGNATURE_VERSION_1, &ssao_serialize, &ssao_error), &ssao_error);

    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, ssao_serialize->GetBufferPointer(), ssao_serialize->GetBufferSize(), IID_PPV_ARGS(&pSsaoRootSignature)));

}

void D3DFrame::BuildPipelineStates()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
    
#define SHADER(name) {Shaders[name]->GetBufferPointer(), Shaders[name]->GetBufferSize()}
#define CreatePipelineState(type, desc) ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState))); PipelineStates[type] = pPipelineState

    // sizeof(float) == 4
    // sizeof(uint) == 4
    D3D12_INPUT_ELEMENT_DESC inputLayouts2[]= {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},  // 12
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},   // 12
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},    // 8
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},  // 12
        {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},  // 16
        {"BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} // 16
    };

    D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;

    D3D_SHADER_MACRO AlphaTest[] = {
        "ALPHATEST", "1",
        NULL, NULL
    };
    
    D3D_SHADER_MACRO Skinned[] = {
        "SKINNED", "1",
        NULL, NULL
    };

    Shaders["Main_Vs"] = CompileShader(L"shaders/shader.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Skinned_Vs"] = CompileShader(L"shaders/shader.hlsl", Skinned, "VsMain", "vs_5_1");
    Shaders["Main_Ps"] = CompileShader(L"shaders/shader.hlsl", AlphaTest, "PsMain", "ps_5_1");
    Shaders["Skinned_Ps"] = CompileShader(L"shaders/shader.hlsl", Skinned, "PsMain", "ps_5_1");
    Shaders["Sky_Vs"] = CompileShader(L"shaders/shader.hlsl", NULL, "VsMain_Sky", "vs_5_1");
    Shaders["Sky_Ps"] = CompileShader(L"shaders/shader.hlsl", NULL, "PsMain_Sky", "ps_5_1");
    Shaders["Shadow_Vs"] = CompileShader(L"shaders/shadow.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Shadow_Ps"] = CompileShader(L"shaders/shadow.hlsl", AlphaTest, "PsMain", "ps_5_1");
    Shaders["Debug_Vs"] = CompileShader(L"shaders/debug.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Debug_Ps"] = CompileShader(L"shaders/debug.hlsl", NULL, "PsMain", "ps_5_1");
    Shaders["Normal_Vs"] = CompileShader(L"shaders/normal.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Normal_Ps"] = CompileShader(L"shaders/normal.hlsl", NULL, "PsMain", "ps_5_1");
    Shaders["Ssao_Vs"] = CompileShader(L"shaders/ssao.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Ssao_Ps"] = CompileShader(L"shaders/ssao.hlsl", NULL, "PsMain", "ps_5_1");
    Shaders["SsaoBlur_Vs"] = CompileShader(L"shaders/ssao_blur.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["SsaoBlur_Ps"] = CompileShader(L"shaders/ssao_blur.hlsl", NULL, "PsMain", "ps_5_1");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC base = {};
    base.InputLayout =  {inputLayouts, _countof(inputLayouts)};
    base.pRootSignature = pRootSignature.Get();
    base.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    base.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    base.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    base.NumRenderTargets = 1;
    base.RTVFormats[0] = emBackBufferFormat;
    base.DSVFormat = emDepthStencilFormat;
    base.SampleMask = UINT_MAX;
    base.SampleDesc.Count = 1;
    base.SampleDesc.Quality = 0;
    base.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaque = base;
    opaque.VS = SHADER("Main_Vs");
    opaque.PS = SHADER("Main_Ps");
    CreatePipelineState(RENDER_TYPE_OPAQUE, opaque);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadow = base;
    shadow.pRootSignature = pRootSignature.Get();
    shadow.VS = SHADER("Shadow_Vs");
    shadow.PS = SHADER("Shadow_Ps");
    shadow.RasterizerState.DepthBias = 100000;
    shadow.RasterizerState.DepthBiasClamp = 0.0f;
    shadow.RasterizerState.SlopeScaledDepthBias = 1.0f;
    shadow.NumRenderTargets = 0;
    shadow.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;

    CreatePipelineState(RENDER_TYPE_OPAQUE_SHADOW, shadow);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC sky = base;
    sky.pRootSignature = pRootSignature.Get();
    sky.VS = SHADER("Sky_Vs");
    sky.PS = SHADER("Sky_Ps");
    sky.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    sky.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    CreatePipelineState(RENDER_TYPE_SKY_BOX, sky);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC debug = base;
    debug.pRootSignature = pRootSignature.Get();
    debug.VS = SHADER("Debug_Vs");
    debug.PS = SHADER("Debug_Ps");
    
    CreatePipelineState(RENDER_TYPE_DEBUG, debug);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC normal = base;
    normal.pRootSignature = pRootSignature.Get();
    normal.VS = SHADER("Normal_Vs");
    normal.PS = SHADER("Normal_Ps");
    normal.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    
    CreatePipelineState(RENDER_TYPE_NORMAL, normal);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssao = base;
    ssao.pRootSignature = pSsaoRootSignature.Get();
    ssao.VS = SHADER("Ssao_Vs");
    ssao.PS = SHADER("Ssao_Ps");
    ssao.RTVFormats[0] = DXGI_FORMAT_R16_UNORM;
    ssao.DepthStencilState.DepthEnable = 0;

    CreatePipelineState(RENDER_TYPE_SSAO, ssao);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssao_blur = ssao;
    ssao_blur.VS = SHADER("SsaoBlur_Vs");
    ssao_blur.PS = SHADER("SsaoBlur_Ps");

    CreatePipelineState(RENDER_TYPE_SSAO_BLUR, ssao_blur);
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC skinned = base;
    skinned.VS = SHADER("Skinned_Vs");
    skinned.PS = SHADER("Skinned_Ps");
    skinned.InputLayout = {inputLayouts2, _countof(inputLayouts2)};

    CreatePipelineState(RENDER_TYPE_SKINNED_OPAQUE, skinned);
    
}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = SrvList.size();
    desc.NodeMask = 0;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap));

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    for(auto& item: SrvList)
    {
        if(!item.bSkip)
        {
            srv.ViewDimension = item.ViewDimension;
            srv.Format = item.ResourceDesc.Format == 0? DXGI_FORMAT_R8G8B8A8_UNORM: item.ResourceDesc.Format;
            
            switch(srv.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                srv.Texture2D.MipLevels = item.ResourceDesc.MipLevels;
                srv.Texture2D.MostDetailedMip = 0;
                srv.Texture2D.PlaneSlice = 0;
                srv.Texture2D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                srv.TextureCube.MipLevels = item.ResourceDesc.MipLevels;
                srv.TextureCube.MostDetailedMip = 0;
                srv.TextureCube.ResourceMinLODClamp = 0;
                break;
            }

            pD3dDevice->CreateShaderResourceView(item.pResource, &srv, handle);
        }
        handle.Offset(1, nCbvSrvUavDescriptorSize);
    }

}   

void D3DFrame::CreateDescriptor()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    // Swap Chain Buffer(3) + Ssao Normal Map(1) + Ssao Maps(2)
    desc.NumDescriptors = nSwapChainBufferCount + 3;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRTVHeap)));

    desc.NumDescriptors = 2;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pDSVHeap)));

    nRTVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    nDSVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    nCbvSrvUavDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DFrame::Update(const GameTimer& t)
{
    static std::vector<XMFLOAT4X4> matlastTrans(14);
#ifdef USE_FRAMERESOURCE
    // 在当前帧命令存放之前，先进行命令分配器的切换
    iCurrFrameResourceIndex = (iCurrFrameResourceIndex + 1) % nFrameResourceCount;
    pCurrFrameResource = &pFrameResources[iCurrFrameResourceIndex];
    pCommandAllocator = pCurrFrameResource->pCommandAllocator;

    // 若当前帧资源未执行完成，说明命令分配器全都被 GPU 使用，等待执行完成
    if(pCurrFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrFrameResource->iFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(pCurrFrameResource->iFence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }
#endif

    nLightRotationAngle += 0.1f * t.DeltaTime();
    XMMATRIX R = XMMatrixRotationY(nLightRotationAngle);
    for(UINT i = 0; i < 3; ++i)
    {
        XMVECTOR baseDir = XMLoadFloat3(&vec3BaseLightDirection[i]);
        XMVECTOR lightDir = XMVector3TransformNormal(baseDir, R);
        XMStoreFloat3(&vec3RotatedLightDirection[i], lightDir);
    }

    InputProcess(t);
    camera.UpdateViewMatrix();

    UpdateObjects(t);
    UpdateMaterials(t);
    UpdateShadowSpace();
    UpdateScene(t);
    UpdateAnimations(t);
\
}

void D3DFrame::UpdateMaterials(const GameTimer& t)
{
    for(auto item: Materials)
    {
        Light::Material& matItem = item.second;
        if(matItem.iFramesDirty > 0)
        {
            Constant::MaterialConstant mat;
            XMMATRIX trans = XMLoadFloat4x4(&matItem.matTransform);
            XMStoreFloat4x4(&mat.matTransform, XMMatrixTranspose(trans));
            mat.nRoughness = matItem.nRoughness;
            mat.vec3FresnelR0 = matItem.vec3FresnelR0;
            mat.vec4DiffuseAlbedo = matItem.vec4DiffuseAlbedo;
            mat.nDiffuseTextureIndex = matItem.nDiffuseSrvHeapIndex;
            mat.nNormalTextureIndex = matItem.nNormalSrvHeapIndex;

            pCurrFrameResource->CBMaterial.CopyData(matItem.nCBMaterialIndex, &mat, sizeof(Constant::MaterialConstant));
            --matItem.iFramesDirty;
        }
    }
}

void D3DFrame::UpdateObjects(const GameTimer& t)
{
    for(UINT i = 0; i < VectorSize(AllRenderItems); ++i)
    {
        RenderItem* item = (RenderItem*)VectorAt(AllRenderItems, i);
        
        if(item->iFramesDirty > 0)
        {
            Constant::ObjectConstant obj;
            XMMATRIX world = XMLoadFloat4x4(&item->matWorld);
            XMMATRIX texTrans = XMLoadFloat4x4(&item->matTexTransform);

            obj.nMaterialIndex = item->pMaterial->nCBMaterialIndex;
            XMStoreFloat4x4(&obj.matWorld, XMMatrixTranspose(world));
            XMStoreFloat4x4(&obj.matTexTransform, XMMatrixTranspose(texTrans));
            pCurrFrameResource->CBObject.CopyData(item->nCBObjectIndex, &obj, CONSTANT_VALUE::nCBObjectByteSize);
            --item->iFramesDirty; 
        }
    }
}

void D3DFrame::UpdateScene(const GameTimer& t)
{
	XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );
    
    Constant::SceneConstant sc;
    XMMATRIX view = camera.GetView();
    XMMATRIX proj = camera.GetProj();
    XMMATRIX matInvView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX matInvProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX matViewProj = XMMatrixMultiply(view, proj);
    XMMATRIX matInvViewProj = XMMatrixInverse(&XMMatrixDeterminant(matViewProj), matViewProj);
    XMMATRIX matViewProjTex = XMMatrixMultiply(matViewProj, T);

    XMStoreFloat4x4(&sc.matView, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));
    XMStoreFloat4x4(&sc.matShadowTransform, XMMatrixTranspose(XMLoadFloat4x4(&matShadowTransform)));
    XMStoreFloat4x4(&sc.matViewProjTex, XMMatrixTranspose(matViewProjTex));

    sc.vec3EyePos = camera.GetPosition3f();
    sc.vec2RenderTargetSize = XMFLOAT2((float)cxClient, (float)cyClient);
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f/ cxClient, 1.0f/ cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();
    sc.vec4AmbientLight = {0.25f, 0.25f, 0.35f, 1.0f};
    sc.vec4FogColor = {0.7f, 0.7f, 0.7f, 1.0f};
	sc.fFogStart = 5.0f;
	sc.fFogRange = 150.0f;

    sc.lights[0].vec3Direction = vec3RotatedLightDirection[0];
    sc.lights[0].vec3Strength = { 0.6f, 0.6f, 0.6f };
    sc.lights[1].vec3Direction = vec3BaseLightDirection[1];
    sc.lights[1].vec3Strength = { 0.3f, 0.3f, 0.3f };
    sc.lights[2].vec3Direction = vec3BaseLightDirection[2];
    sc.lights[2].vec3Strength = {0.15f, 0.15f, 0.15f};
    pCurrFrameResource->CBScene.CopyData(0, &sc, CONSTANT_VALUE::nCBSceneByteSize);

    // Shadow Scene
    view = XMLoadFloat4x4(&matShadowView);
    proj = XMLoadFloat4x4(&matShadowProj);
    matInvView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    matInvProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    matViewProj = XMMatrixMultiply(view, proj);
    matInvViewProj = XMMatrixInverse(&XMMatrixDeterminant(matViewProj), matViewProj);
    matViewProjTex = XMMatrixMultiply(matViewProj, T);

    XMStoreFloat4x4(&sc.matView, view);
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));    
    XMStoreFloat4x4(&sc.matShadowTransform, XMMatrixTranspose(XMLoadFloat4x4(&matShadowTransform)));
    XMStoreFloat4x4(&sc.matViewProjTex, XMMatrixTranspose(matViewProjTex));
    sc.vec3EyePos = vec3MainLightPos;
    sc.vec2RenderTargetSize = XMFLOAT2(0, 0);
    sc.vec2InvRenderTargetSize = XMFLOAT2(0, 0);
    sc.fNearZ = nLightNearZ;
    sc.fFarZ = nLightFarZ;
    pCurrFrameResource->CBScene.CopyData(1, &sc, CONSTANT_VALUE::nCBSceneByteSize);

    Constant::SsaoConstant cbSsao;
    XMMATRIX matProjTex = XMMatrixMultiply(proj, T);

    XMStoreFloat4x4(&cbSsao.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&cbSsao.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&cbSsao.matProjTex, XMMatrixTranspose(matProjTex));
    ssao.GetOffsetVectors(cbSsao.vec4Offsets);
    CopyMemory(cbSsao.vec4BlurWeights, weights, 12 * sizeof(float));

    cbSsao.vec2InvRenderTargetSize = {1.0f/ ssao.SsaoMapWidth(), 1.0f / ssao.SsaoMapHeight()};
    cbSsao.nOcclusionFadeStart = 0.2f;
    cbSsao.nOcclusionFadeEnd = 1.0f;
    cbSsao.nOcclusionRadius = 0.5f;
    cbSsao.nSurfaceEpsilon = 0.05;
    
    pCurrFrameResource->CBOthers[Resource::FrameResource::FRAME_RESOURCE_TYPE_SSAO].CopyData(0, &cbSsao, CONSTANT_VALUE::nCBSsaoByteSize);
    
}

void D3DFrame::UpdateShadowSpace()
{
    XMVECTOR lightDir = XMLoadFloat3(&vec3RotatedLightDirection[0]);
    XMVECTOR lightPos = -2.0f * stSceneBounds.Radius * lightDir;
    XMVECTOR targetPos = XMLoadFloat3(&stSceneBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMStoreFloat3(&vec3MainLightPos, lightPos);

    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    float l = sphereCenterLS.x - stSceneBounds.Radius;
    float b = sphereCenterLS.y - stSceneBounds.Radius;
    float n = sphereCenterLS.z - stSceneBounds.Radius;
    float r = sphereCenterLS.x + stSceneBounds.Radius;
    float t = sphereCenterLS.y + stSceneBounds.Radius;
    float f = sphereCenterLS.z + stSceneBounds.Radius;

    nLightFarZ = f;
    nLightNearZ = n;
    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );

    XMMATRIX S = lightView * proj * T;
    XMStoreFloat4x4(&matShadowView, lightView);
    XMStoreFloat4x4(&matShadowProj, proj);
    XMStoreFloat4x4(&matShadowTransform, S);
}

void D3DFrame::UpdateAnimations(const GameTimer& t)
{
    Constant::SkinnedConstant sc;

    Soldier.UpdateAnimation(t.DeltaTime());
   
    pCurrFrameResource->CBOthers[Resource::FrameResource::FRAME_RESOURCE_TYPE_SKINNED]
    .CopyData(0, &Soldier.matFinalTransforms[0] , Soldier.matFinalTransforms.size() * sizeof(XMFLOAT4X4));
}

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    camera.SetLens(0.25f * XM_PI, AspectRatio(), 0.1f, 1000.0f);
}

void D3DFrame::InputProcess(const GameTimer& t)
{
	if(GetAsyncKeyState('W') & 0x8000)
		camera.Walk(10.0f*t.DeltaTime());

	if(GetAsyncKeyState('S') & 0x8000)
		camera.Walk(-10.0f*t.DeltaTime());

	if(GetAsyncKeyState('A') & 0x8000)
		camera.Strafe(-10.0f*t.DeltaTime());

	if(GetAsyncKeyState('D') & 0x8000)
		camera.Strafe(10.0f*t.DeltaTime());
}

void D3DFrame::OnMouseMove(WPARAM state, int x, int y)
{
	 if((state & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f*(x - lastPos.x));
        float dy = XMConvertToRadians(0.25f*(y - lastPos.y));

		camera.Pitch(dy);
		camera.RotateY(dx);
    }
    lastPos.x = x;
    lastPos.y = y;
}

void D3DFrame::OnMouseDown(WPARAM state, int x, int y)
{
	lastPos.x = x;
    lastPos.y = y;

    SetCapture(hwnd);
}

void D3DFrame::OnMouseUp(WPARAM state, int x, int y)
{
	ReleaseCapture();
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    ID3D12DescriptorHeap* Heaps[] = {pSRVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, Heaps);

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

    // Materials
    pCommandList->SetGraphicsRootShaderResourceView(3, pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress());
    // Sky Map, Shadow Map
    pCommandList->SetGraphicsRootDescriptorTable(4, CD3DX12_GPU_DESCRIPTOR_HANDLE(pSRVHeap->GetGPUDescriptorHandleForHeapStart(), nSkyMapSrvIndex, nCbvSrvUavDescriptorSize));
	// Textures
    pCommandList->SetGraphicsRootDescriptorTable(5, pSRVHeap->GetGPUDescriptorHandleForHeapStart());
    
    DrawSceneToShadowMap();

    // World
    pCommandList->SetGraphicsRootConstantBufferView(2, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 1, &DepthStencilBufferView());
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, NULL);
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_OPAQUE].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPAQUE]);
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_SKINNED_OPAQUE].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_SKINNED_OPAQUE]);

    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_DEBUG].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_DEBUG]);
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_SKY_BOX].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_SKY_BOX]);

    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	
    HRESULT hr = pSwapChain->Present(0, 0);
    ThrowIfFailed(hr);

    iCurrBackBuffer = (iCurrBackBuffer + 1) % nSwapChainBufferCount;
	
#ifdef USE_FRAMERESOURCE
    pCurrFrameResource->iFence = (UINT)++iCurrentFence;
    pCommandQueue->Signal(pFence.Get(), iCurrentFence);
#else
    FlushCommandQueue();
#endif
}

void D3DFrame::DrawSceneToShadowMap()
{
    pCommandList->RSSetViewports(1, &stShadowMap.Viewport());
    pCommandList->RSSetScissorRects(1, &stShadowMap.ScissorRect());

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        stShadowMap.Resource(),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    ));

    pCommandList->OMSetRenderTargets(0, NULL, 0, &stShadowMap.Dsv());
    pCommandList->ClearDepthStencilView(stShadowMap.Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    D3D12_GPU_VIRTUAL_ADDRESS cbSceneAddr = pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress() + CONSTANT_VALUE::nCBSceneByteSize;
    pCommandList->SetGraphicsRootConstantBufferView(2, cbSceneAddr);
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_OPAQUE_SHADOW].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPAQUE]);

    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_SKINNED_OPAQUE].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_SKINNED_OPAQUE]);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        stShadowMap.Resource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));
}

void D3DFrame::DrawSceneToNormalMap()
{
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ssao.NormalMap(),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));
    
    float clearColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    pCommandList->ClearRenderTargetView(ssao.NormalMapRtv(), clearColor, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
    pCommandList->OMSetRenderTargets(1, &ssao.NormalMapRtv(), 1, &DepthStencilBufferView());

    pCommandList->SetGraphicsRootConstantBufferView(1, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_NORMAL].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPAQUE]);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ssao.NormalMap(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));

}

void D3DFrame::DrawItems(ID3D12GraphicsCommandList* list, std::vector<UINT>& items)
{
    for(UINT i : items)
    {
        RenderItem& item = *(RenderItem*)VectorAt(AllRenderItems, i);

        list->IASetPrimitiveTopology(item.emPrimitiveType);
        list->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        list->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        // Object
        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item.nCBObjectIndex * CONSTANT_VALUE::nCBObjectByteSize;
        list->SetGraphicsRootConstantBufferView(0, addr);
#ifdef D3D12_SKINNED
        if(item.Skinned)
        {
            addr = pCurrFrameResource->CBOthers[Resource::FrameResource::FRAME_RESOURCE_TYPE_SKINNED].Resource()->GetGPUVirtualAddress();
            addr += item.nSkinnedCBIndex * CONSTANT_VALUE::nCBSkinnedByteSize;

            // Skinned Data
            list->SetGraphicsRootConstantBufferView(1, addr);
        }
        else
#endif
        {
            list->SetGraphicsRootConstantBufferView(1, 0);
        }
        list->DrawIndexedInstanced(item.nIndexCount, item.nInstanceCount, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }
}

#define IsDigit(c) (c >= '0' && c <= '9')

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices)
{
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pBlock;
    UINT nVertexCounter = 0, nTriangleCounter = 0;

//****** Read File
    if((hFile = CreateFileA("C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Models/skull.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return;
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = (char*)malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

//****** Load Data
    pBlock = pFileBuffer;

    while(!IsDigit(*pBlock))++pBlock;
    pBegin = pBlock;
    while(*pBlock != '\r')++pBlock;
    *pBlock++ = '\0';

    nVertexCounter = atoi(pBegin);
    while(!IsDigit(*pBlock))++pBlock;
    pBegin = pBlock;
    while(*pBlock != '\r')++pBlock;
    *pBlock++ = '\0';
    
    nTriangleCounter = atoi(pBegin);
    
    vertices.resize(nVertexCounter);
    indices.resize(nTriangleCounter);

    while(*pBlock++ != '{');

    for(UINT i = 0; i < nVertexCounter; ++i)
    {
        for(UINT j = 0; j < 6; ++j)
        {
            while(!IsDigit(*pBlock) && *pBlock != '-')++pBlock;
            pBegin = pBlock;
            while(*pBlock != ' ' && *pBlock != '\r')++pBlock;
            *pBlock++ = '\0';

            vertices[i].v[j] = atof(pBegin);
        }
    }

    while(*pBlock++ != '\n');
    assert(*pBlock == '}');

    while(*pBlock++ != '{');
    for(UINT i = 0; i < nTriangleCounter; ++i)
    {
        for(UINT j = 0; j < 3; ++j)
        {
            while(!IsDigit(*pBlock))++pBlock;
            pBegin = pBlock;
            while(*pBlock != ' ' && *pBlock != '\r')++pBlock;
            *pBlock++ = '\0';
            indices[i].i[j] = atof(pBegin);
        }
    }

    free(pFileBuffer);
}   

void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC* sampler)
{
    sampler[0].Init(
        0, 
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );

    sampler[1].Init(
        1, 
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );

    sampler[2].Init(
        2, 
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );

    sampler[3].Init(
        3, 
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );

    sampler[4].Init(
        4, 
        D3D12_FILTER_ANISOTROPIC, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f, 8
    );

    sampler[5].Init(
        5, 
        D3D12_FILTER_ANISOTROPIC, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f, 8
    );

    sampler[6].Init(
        6, 
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
    );
}
