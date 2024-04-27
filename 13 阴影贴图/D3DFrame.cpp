#define D3D12_DYNAMIC_INDEX
#include "D3DFrame.h"

void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC* sampler);

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{
    stSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    stSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
    
    vec3BaseLightDirection[0] = { 0.57735f, -0.57735f, 0.57735f }; 
    vec3BaseLightDirection[1] = { -0.57735f, -0.57735f, 0.57735f };
    vec3BaseLightDirection[2] = { 0.0f, -0.707f, -0.707f };
}

bool D3DFrame::Initialize()
{
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;     
#endif
    if(!D3DApp::Initialize())
        return 0;
    
    camera.LookAt(XMFLOAT3(0.0f, 0.0f, -5.0f), {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

	ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));
	LoadTextures();
	BuildGeometries();
	BuildMaterials();

    stShadowMap.Init(pD3dDevice.Get(), cxClient, cyClient);
	ThrowIfFailed(pCommandList->Close());
	
    ID3D12CommandList* ppLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppLists), ppLists);
	
    BuildSRV();
    BuildRenderItems();
    BuildRootSignatures();
    BuildPipelineStates();

	FlushCommandQueue();
    return 1;
}

void D3DFrame::LoadTextures()
{
    char* TextureNames[] = {
        "wood",
        "wood_normal",
        "brick",
        "brick_normal",
        "tile",
        "tile_normal",
        "default",
        "default_normal",
        "grassCube",
    };

    LPWSTR TexturePaths[] = {
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/plywood.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/plywood_normal.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/bricks.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/bricks_normal.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/tile.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/tile_normal.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/white1x1.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/Normals/default_normal.dds",
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/grasscube1024.dds",
    };
    
    assert(_countof(TextureNames) == _countof(TexturePaths));

    for(UINT i = 0; i < _countof(TextureNames); ++i)
    {
        Resource::Texture tex;
        tex.Name = TextureNames[i];
        tex.FileName = TexturePaths[i];
        tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), TexturePaths[i], &tex.pUploader);
        Textures[TextureNames[i]] = tex;
    }
}

void D3DFrame::BuildGeometries()
{
    Resource::MeshGeometry Geo, SkullGeo;
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

    Geo.Name = "Geo";
    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], indexBytesSize, &Geo.pUploaderIndexBuffer);

    Geo.nIndexBufferByteSize = indexBytesSize;
    Geo.nVertexBufferByteSize = vertexBytesSize;
    Geo.nVertexByteStride = sizeof(Vertex);

    Geo.DrawArgs["box"] = boxSubmesh;
    Geo.DrawArgs["grid"] = gridSubmesh;
    Geo.DrawArgs["sphere"] = sphereSubmesh;
    Geo.DrawArgs["cylinder"] = cylinderSubmesh;
    Geo.DrawArgs["quad"] = quadSubmesh;

    Geos["Geo"] = Geo;

    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> skullIndices;
    
    LoadSkullModel(skullVertices, skullIndices);

    vertices.resize(skullVertices.size());

    XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
    XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

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

    vertexBytesSize = vertices.size() * sizeof(Vertex);
    indexBytesSize = skullIndices.size() * sizeof(SkullModelIndex);

    SkullGeo.Name = "SkullGeo";
    SkullGeo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &SkullGeo.pUploaderVertexBuffer);
    SkullGeo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &skullIndices[0], indexBytesSize, &SkullGeo.pUploaderIndexBuffer);

    SkullGeo.nIndexBufferByteSize = indexBytesSize;
    SkullGeo.nVertexBufferByteSize = vertexBytesSize;
    SkullGeo.nVertexByteStride = sizeof(Vertex);

    Resource::SubmeshGeometry  skullSubmesh;
    skullSubmesh.nIndexCount = skullIndices.size() * 3;
    skullSubmesh.nStartIndexLocation = 0;
    skullSubmesh.nBaseVertexLocation = 0;
    skullSubmesh.Bounds = bounds;

    SkullGeo.DrawArgs["main"] = skullSubmesh;

    Geos["Skull"] = SkullGeo;
}   

void D3DFrame::BuildMaterials()
{
    Light::Material wood;
    wood.iFramesDirty = 3;
    wood.Name = "wood";
    wood.nCBMaterialIndex = 0;
    wood.nDiffuseSrvHeapIndex = 0;
    wood.nNormalSrvHeapIndex = 1;
    wood.nRoughness = 0.2f;
    wood.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    wood.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};
    
    Light::Material brick;
    brick.iFramesDirty = 3;
    brick.Name = "brick";
    brick.nCBMaterialIndex = 1;
    brick.nDiffuseSrvHeapIndex = 2;
    brick.nNormalSrvHeapIndex = 3;
    brick.nRoughness = 0.1f;
    brick.vec3FresnelR0 = {0.02f, 0.02f, 0.02f};
    brick.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material tile;
    tile.iFramesDirty = 3;
    tile.Name = "tile";
    tile.nCBMaterialIndex = 2;
    tile.nDiffuseSrvHeapIndex = 4;
    tile.nNormalSrvHeapIndex = 5;
    tile.nRoughness = 0.3f;
    tile.vec3FresnelR0 = {0.02f, 0.02f, 0.02f};
    tile.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material skull;
    skull.iFramesDirty = 3;
    skull.Name = "skull";
    skull.nCBMaterialIndex = 3;
    skull.nDiffuseSrvHeapIndex = 6;
    skull.nNormalSrvHeapIndex = 7;
    skull.nRoughness = 0.2f;
    skull.vec3FresnelR0 = {0.6f, 0.6f, 0.6f};
    skull.vec4DiffuseAlbedo = {0.3f, 0.3f, 0.3f, 1.0f};

    Light::Material sky;
    sky.iFramesDirty = 3;
    sky.Name = "sky";
    sky.nCBMaterialIndex = 4;
    sky.nDiffuseSrvHeapIndex = 8;
    sky.nNormalSrvHeapIndex = 9;
    sky.nRoughness = 1.0f;
    sky.vec3FresnelR0 = {1.0f, 1.0f, 1.0f};
    sky.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Materials["wood"] = wood;
    Materials["brick"] = brick;
    Materials["skull"] = skull;
    Materials["tile"] = tile;
    Materials["sky"] = sky;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
    {
		// 上传缓冲区不应当进行内存对齐
        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
        Init(pD3dDevice.Get(), sizeof(Constant::MaterialConstant), Materials.size());
    }    

}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry info;

    RenderItem sky;
    sky.iFramesDirty = 3;
    sky.nCBObjectIndex = 0;
    sky.pMaterial = &Materials["sky"];
    sky.pGeo = &Geos["Geo"];
    sky.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = sky.pGeo->DrawArgs["box"];
    sky.nIndexCount = info.nIndexCount;
    sky.nStartIndexLocation = info.nStartIndexLocation;
    sky.nBaseVertexLocation = info.nBaseVertexLocation;

    XMStoreFloat4x4(&sky.matWorld, XMMatrixScaling(2000.0f, 2000.0f, 2000.0f));
    sky.matTexTransform = MathHelper::Identity4x4();

    AllRenderItems.push_back(sky);
    RenderItems[RENDER_TYPE_SKY_BOX].push_back(0);

    RenderItem debug;
    debug.iFramesDirty = 3;
    debug.nCBObjectIndex = 1;
    debug.pMaterial = &Materials["skull"];
    debug.pGeo = &Geos["Geo"];
    debug.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = debug.pGeo->DrawArgs["quad"];
    debug.nIndexCount = info.nIndexCount;
    debug.nStartIndexLocation = info.nStartIndexLocation;
    debug.nBaseVertexLocation = info.nBaseVertexLocation;
    
    debug.matTexTransform = MathHelper::Identity4x4();
    debug.matWorld = MathHelper::Identity4x4();

    AllRenderItems.push_back(debug);
    RenderItems[RENDER_TYPE_DEBUG].push_back(1);

    RenderItem skull;
    skull.iFramesDirty = 3;
    skull.nCBObjectIndex = 2;
    skull.pMaterial = &Materials["skull"];
    skull.pGeo = &Geos["Skull"];
    skull.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = skull.pGeo->DrawArgs["main"];
    skull.nIndexCount = info.nIndexCount;
    skull.nStartIndexLocation = info.nStartIndexLocation;
    skull.nBaseVertexLocation = info.nBaseVertexLocation;

    XMStoreFloat4x4(&skull.matWorld, XMMatrixScaling(.5f, .5f, .5f) * XMMatrixTranslation(0.0f, 2.5f, 0.0f));
    skull.matTexTransform = MathHelper::Identity4x4();
    AllRenderItems.push_back(skull);

    RenderItem box;
    box.iFramesDirty = 3;
    box.nCBObjectIndex = 3;
    box.pMaterial = &Materials["wood"];
    box.pGeo = &Geos["Geo"];
    box.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    
    info = box.pGeo->DrawArgs["box"];
    box.nIndexCount = info.nIndexCount;
    box.nStartIndexLocation = info.nStartIndexLocation;
    box.nBaseVertexLocation = info.nBaseVertexLocation;
    XMStoreFloat4x4(&box.matWorld, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    XMStoreFloat4x4(&box.matTexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

    AllRenderItems.push_back(box);
    
    RenderItem grid;
    grid.iFramesDirty = 3;
    grid.nCBObjectIndex = 4;
    grid.pMaterial = &Materials["tile"]; 
    grid.pGeo = &Geos["Geo"];
    grid.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    info = grid.pGeo->DrawArgs["grid"];
    grid.nIndexCount = info.nIndexCount;
    grid.nBaseVertexLocation = info.nBaseVertexLocation;
    grid.nStartIndexLocation = info.nStartIndexLocation;
    grid.matWorld = MathHelper::Identity4x4();
    XMStoreFloat4x4(&grid.matTexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
    
    AllRenderItems.push_back(grid);

    XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
    UINT objIndex = 5;
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
        lCI.pGeo = &Geos["Geo"];
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
        rCI.pGeo = &Geos["Geo"];
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
        lSI.pGeo = &Geos["Geo"];
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
        rSI.pGeo = &Geos["Geo"];
        rSI.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        info = rSI.pGeo->DrawArgs["sphere"];
        rSI.nIndexCount = info.nIndexCount;
        rSI.nBaseVertexLocation = info.nBaseVertexLocation;
        rSI.nStartIndexLocation = info.nStartIndexLocation;
        rSI.matTexTransform = MathHelper::Identity4x4();
        XMStoreFloat4x4(&rSI.matWorld, rSW);

        AllRenderItems.push_back(lCI);
        AllRenderItems.push_back(rCI);
        AllRenderItems.push_back(lSI);
        AllRenderItems.push_back(rSI);
    }

    for(UINT i = 2; i < AllRenderItems.size(); ++i)
        RenderItems[RENDER_TYPE_OPEAQUE].push_back(i);
    
}

void D3DFrame::BuildRootSignatures()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 2, AllRenderItems.size());
    CD3DX12_DESCRIPTOR_RANGE range[2];
	CD3DX12_ROOT_PARAMETER params[5];
    CD3DX12_STATIC_SAMPLER_DESC sampler[7];

    GetStaticSampler(sampler);
	
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2);
    
    params[0].InitAsConstantBufferView(0);
    params[1].InitAsConstantBufferView(1);
    params[2].InitAsShaderResourceView(0, 1);       // Materials
    params[3].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);  // SkyMap ShadowMap
    params[4].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);  // Textures

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(5, params, 7, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serialize, error;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialize, &error), &error);

    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, serialize->GetBufferPointer(), serialize->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
}

void D3DFrame::BuildPipelineStates()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
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

    Shaders["Normal_Vs"] = CompileShader(L"shaders/shader.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Normal_Ps"] = CompileShader(L"shaders/shader.hlsl", AlphaTest, "PsMain", "ps_5_1");
    Shaders["Sky_Vs"] = CompileShader(L"shaders/shader.hlsl", NULL, "VsMain_Sky", "vs_5_1");
    Shaders["Sky_Ps"] = CompileShader(L"shaders/shader.hlsl", NULL, "PsMain_Sky", "ps_5_1");
    Shaders["Shadow_Vs"] = CompileShader(L"shaders/shadow.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Shadow_Ps"] = CompileShader(L"shaders/shadow.hlsl", AlphaTest, "PsMain", "ps_5_1");
    Shaders["ShadowDebug_Vs"] = CompileShader(L"shaders/shadow_debug.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["ShadowDebug_Ps"] = CompileShader(L"shaders/shadow_debug.hlsl", NULL, "PsMain", "ps_5_1");

    // Opaque
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.InputLayout = {inputLayouts, _countof(inputLayouts)};
    desc.pRootSignature = pRootSignature.Get();
    desc.VS = {Shaders["Normal_Vs"]->GetBufferPointer(), Shaders["Normal_Vs"]->GetBufferSize()};
    desc.PS = {Shaders["Normal_Ps"]->GetBufferPointer(), Shaders["Normal_Ps"]->GetBufferSize()};
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = emBackBufferFormat;
    desc.DSVFormat = emDepthStencilFormat;
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[RENDER_TYPE_OPEAQUE] = pPipelineState;

    // Sky Box
    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyDesc = desc;
    skyDesc.VS = {Shaders["Sky_Vs"]->GetBufferPointer(), Shaders["Sky_Vs"]->GetBufferSize()};
    skyDesc.PS = {Shaders["Sky_Ps"]->GetBufferPointer(), Shaders["Sky_Ps"]->GetBufferSize()};
    skyDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    skyDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&skyDesc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[RENDER_TYPE_SKY_BOX] = pPipelineState;

    // Shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDesc = desc;
    shadowDesc.RasterizerState.DepthBias = 100000;
    shadowDesc.RasterizerState.DepthBiasClamp = 0.0f;
    shadowDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    shadowDesc.VS = {Shaders["Shadow_Vs"]->GetBufferPointer(), Shaders["Shadow_Vs"]->GetBufferSize()};
    shadowDesc.PS = {Shaders["Shadow_Ps"]->GetBufferPointer(), Shaders["Shadow_Ps"]->GetBufferSize()};
    shadowDesc.NumRenderTargets = 0;
    shadowDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&shadowDesc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[RENDER_TYPE_OPAQUE_SHADOW] = pPipelineState;

    // Shadow debug
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDebugDesc = desc;
    shadowDebugDesc.VS = {Shaders["ShadowDebug_Vs"]->GetBufferPointer(), Shaders["ShadowDebug_Vs"]->GetBufferSize()};
    shadowDebugDesc.PS = {Shaders["ShadowDebug_Ps"]->GetBufferPointer(), Shaders["ShadowDebug_Ps"]->GetBufferSize()};
    
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&shadowDebugDesc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[RENDER_TYPE_DEBUG] = pPipelineState;
}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = Textures.size() + 3;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap)));

    ComPtr<ID3D12Resource> Texture2DList[] = {
        Textures["wood"].pResource,
        Textures["wood_normal"].pResource,
        Textures["brick"].pResource,
        Textures["brick_normal"].pResource,
        Textures["tile"].pResource,
        Textures["tile_normal"].pResource,
        Textures["default"].pResource,
        Textures["default_normal"].pResource
    };

    CD3DX12_CPU_DESCRIPTOR_HANDLE hSrvHeap(pSRVHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    for(UINT i = 0; i < _countof(Texture2DList); ++i)
    {
        srvDesc.Format = Texture2DList[i]->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = Texture2DList[i]->GetDesc().MipLevels;
        pD3dDevice->CreateShaderResourceView(Texture2DList[i].Get(), &srvDesc, hSrvHeap);
        hSrvHeap.Offset(1, nCbvSrvUavDescriptorSize);
    }
	
	nSkyMapSrvIndex = _countof(Texture2DList);
	nShadowMapSrvIndex = nSkyMapSrvIndex + 1;
	nNullMapSrvIndex = nShadowMapSrvIndex + 1;
	
    // CubeMap
    ID3D12Resource* res = Textures["grassCube"].pResource.Get();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.TextureCube.MipLevels = res->GetDesc().MipLevels;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0;

    pD3dDevice->CreateShaderResourceView(res, &srvDesc, hSrvHeap);
    hSrvHeap.Offset(2, nCbvSrvUavDescriptorSize);
	
    // NULL
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0;
	
    pD3dDevice->CreateShaderResourceView(NULL, &srvDesc, hSrvHeap);
	
	hSrvHeap.Offset(1, nCbvSrvUavDescriptorSize);
	
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	pD3dDevice->CreateShaderResourceView(NULL, &srvDesc, hSrvHeap);
    
	stShadowMap.BuildDescriptor(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(pSRVHeap->GetCPUDescriptorHandleForHeapStart(), nShadowMapSrvIndex, nCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(pSRVHeap->GetGPUDescriptorHandleForHeapStart(), nShadowMapSrvIndex, nCbvSrvUavDescriptorSize),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(pDSVHeap->GetCPUDescriptorHandleForHeapStart(), 1, nDSVDescriptorSize)
    );
}   

void D3DFrame::CreateDescriptor()
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = nSwapChainBufferCount;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&pRTVHeap)
    ));

    // DSV heap
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.NumDescriptors = 2;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&pDSVHeap)
    ));

    nRTVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    nDSVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    nCbvSrvUavDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DFrame::Update(const GameTimer& t)
{
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

    camera.UpdateViewMatrix();
    InputProcess(t);
    UpdateObjects(t);
    UpdateMaterials(t);
    UpdateShadowSpace();
    UpdateScene(t);
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
    for(auto& item: AllRenderItems)
    {
        if(item.iFramesDirty > 0)
        {
            Constant::ObjectConstant obj;
            XMMATRIX world = XMLoadFloat4x4(&item.matWorld);
            XMMATRIX texTrans = XMLoadFloat4x4(&item.matTexTransform);

            obj.nMaterialIndex = item.pMaterial->nCBMaterialIndex;
            XMStoreFloat4x4(&obj.matWorld, XMMatrixTranspose(world));
            XMStoreFloat4x4(&obj.matTexTransform, XMMatrixTranspose(texTrans));
            pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &obj, CONSTANT_VALUE::nCBObjectByteSize);
            --item.iFramesDirty; 
        }
    }
}

void D3DFrame::UpdateScene(const GameTimer& t)
{
    Constant::SceneConstant sc;
	
    XMMATRIX view = camera.GetView();
    XMMATRIX proj = camera.GetProj();
    XMMATRIX matInvView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX matInvProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX matViewProj = XMMatrixMultiply(view, proj);
    XMMATRIX matInvViewProj = XMMatrixInverse(&XMMatrixDeterminant(matViewProj), matViewProj);
    
    XMStoreFloat4x4(&sc.matView, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));
    XMStoreFloat4x4(&sc.matShadowTransform, XMMatrixTranspose(XMLoadFloat4x4(&matShadowTransform)));
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

    XMStoreFloat4x4(&sc.matView, view);
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));    
    XMStoreFloat4x4(&sc.matShadowTransform, XMMatrixTranspose(XMLoadFloat4x4(&matShadowTransform)));
    sc.vec3EyePos = vec3MainLightPos;
    sc.vec2RenderTargetSize = XMFLOAT2(0, 0);
    sc.vec2InvRenderTargetSize = XMFLOAT2(0, 0);
    sc.fNearZ = nLightNearZ;
    sc.fFarZ = nLightFarZ;
    pCurrFrameResource->CBScene.CopyData(1, &sc, CONSTANT_VALUE::nCBSceneByteSize);
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
    pCommandList->SetGraphicsRootShaderResourceView(2, pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(pSRVHeap->GetGPUDescriptorHandleForHeapStart(), nNullMapSrvIndex, nCbvSrvUavDescriptorSize));
	pCommandList->SetGraphicsRootDescriptorTable(4, pSRVHeap->GetGPUDescriptorHandleForHeapStart());
    
    DrawSceneToShadowMap();
    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 1, &DepthStencilBufferView());
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    pCommandList->SetGraphicsRootConstantBufferView(1, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(pSRVHeap->GetGPUDescriptorHandleForHeapStart(), nSkyMapSrvIndex, nCbvSrvUavDescriptorSize));
	
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_OPEAQUE].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPEAQUE]);
  
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
	
    ThrowIfFailed(pSwapChain->Present(0, 0));
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

    pCommandList->ClearDepthStencilView(stShadowMap.Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    pCommandList->OMSetRenderTargets(0, NULL, 0, &stShadowMap.Dsv());

    D3D12_GPU_VIRTUAL_ADDRESS cbSceneAddr = pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress() + CONSTANT_VALUE::nCBSceneByteSize;
    pCommandList->SetGraphicsRootConstantBufferView(1, cbSceneAddr);
    
    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_OPAQUE_SHADOW].Get());
    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPEAQUE]);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        stShadowMap.Resource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));
}

void D3DFrame::DrawItems(ID3D12GraphicsCommandList* list, std::vector<UINT>& items)
{
    for(UINT i : items)
    {
        RenderItem& item = AllRenderItems[i];

        list->IASetPrimitiveTopology(item.emPrimitiveType);
        list->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        list->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        // Object
        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item.nCBObjectIndex * CONSTANT_VALUE::nCBObjectByteSize;
        list->SetGraphicsRootConstantBufferView(0, addr);

        list->DrawIndexedInstanced(item.nIndexCount, item.nInstanceCount, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }
}

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices)
{
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pEnd, *pBlock;
    UINT iVertexCounter = 0, iIndexCounter = 0;
    UINT j;

//****** Read File
    if((hFile = CreateFileA("C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Models/skull.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return;
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = (char*)malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

    vertices.resize(31076);
    indices.resize(60339);
//****** Load Data
    pBlock = pFileBuffer;

    // vertex
    while(*pBlock++ != '{');

    while(*pBlock != '}' || *pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9') || *pBlock == '-')){++pBlock;}
        // 一组默认有 6 个浮点数
        for(j = 0; j < 6; ++j)
        {
            pBegin = pBlock;
            while(((*pBlock != ' ') && (*pBlock != '\r'))){++pBlock;}
            pEnd = pBlock++;      // ' '
            *pEnd = '\0';

            vertices[iVertexCounter].v[j] = atof(pBegin);
        }
        ++iVertexCounter;
        if(iVertexCounter == 31076)
            break;
    }

    // indices
    while(*pBlock++ != '{');

    while(*pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9'))){++pBlock;}

        for(j = 0; j < 3; ++j)
        {
            pBegin = pBlock;
            while(((*pBlock != ' ') && (*pBlock != '\r'))){++pBlock;}
            pEnd = pBlock++;      // ' '
            *pEnd = '\0';

            indices[iIndexCounter].i[j] = atoi(pBegin);
        }
        
        ++iIndexCounter;
        if(iIndexCounter == 60339)
            break;
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
