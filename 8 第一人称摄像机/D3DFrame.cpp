#include "D3DFrame.h"

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{

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
	Resource::Texture tex;
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/plywood.dds";
    tex.Name = "wood";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["wood"] = tex;

    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/bricks.dds";
    tex.Name = "brick";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["brick"] = tex;

    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/white1x1.dds";
    tex.Name = "while";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["white"] = tex;

    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/tile.dds";
    tex.Name = "tile";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["tile"] = tex;

    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/stone.dds";
    tex.Name = "stone";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["stone"] = tex;
}

void D3DFrame::BuildGeometries()
{
    Resource::MeshGeometry Geo, SkullGeo;
    Geometry::Mesh box = Geometry::GeometryGenerator::CreateCube(1.0f, 1.0f, 1.0f);
    Geometry::Mesh cylinder = Geometry::GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
    Geometry::Mesh sphere = Geometry::GeometryGenerator::CreateSphere(0.5f, 20, 20);
    Geometry::Mesh grid = Geometry::GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
    
    UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.vertices.size();

    UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.indices.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.indices.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.indices.size();

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

    UINT vertexTotal = cylinderVertexOffset + cylinder.vertices.size();
    std::vector<Vertex> vertices(vertexTotal);

    UINT k = 0;
    for(UINT i = 0; i < box.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = box.vertices[i].vec3Normal;
        vertices[k].Position = box.vertices[i].vec3Position;
        vertices[k].TexCoords = box.vertices[i].vec2TexCoords;
    }

    for(UINT i = 0; i < grid.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = grid.vertices[i].vec3Normal;
        vertices[k].Position = grid.vertices[i].vec3Position;
        vertices[k].TexCoords = grid.vertices[i].vec2TexCoords;
    }
    
    for(UINT i = 0; i < sphere.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = sphere.vertices[i].vec3Normal;
        vertices[k].Position = sphere.vertices[i].vec3Position;
        vertices[k].TexCoords = sphere.vertices[i].vec2TexCoords;
    }
    
    for(UINT i = 0; i < cylinder.vertices.size(); ++i, ++k)
    {
        vertices[k].Normal = cylinder.vertices[i].vec3Normal;
        vertices[k].Position = cylinder.vertices[i].vec3Position;
        vertices[k].TexCoords = cylinder.vertices[i].vec2TexCoords;
    }
    
    std::vector<UINT16> indices;

    indices.insert(std::end(indices), std::begin(box.indices), std::end(box.indices));
    indices.insert(std::end(indices), std::begin(grid.indices), std::end(grid.indices));
    indices.insert(std::end(indices), std::begin(sphere.indices), std::end(sphere.indices));
    indices.insert(std::end(indices), std::begin(cylinder.indices), std::end(cylinder.indices));
    
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

    Geos["Geo"] = Geo;

    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> skullIndices;
    
    LoadSkullModel(skullVertices, skullIndices);

    vertexBytesSize = skullVertices.size() * sizeof(SkullModelVertex);
    indexBytesSize = skullIndices.size() * sizeof(SkullModelIndex);
    
    vertices.resize(skullVertices.size());

    for(UINT i = 0; i < skullVertices.size(); ++i)
        vertices[i] = {XMFLOAT3(skullVertices[i].Position), XMFLOAT3(skullVertices[i].Normal), {0.0f, 0.0f}};

    SkullGeo.Name = "SkullGeo";
    SkullGeo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &SkullGeo.pUploaderVertexBuffer);
    SkullGeo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &skullIndices[0], indexBytesSize, &SkullGeo.pUploaderIndexBuffer);

    SkullGeo.nIndexBufferByteSize = indexBytesSize;
    SkullGeo.nVertexBufferByteSize = vertexBytesSize;
    SkullGeo.nVertexByteStride = sizeof(Vertex);

    Resource::SubmeshGeometry  skullSubmesh;
    skullSubmesh.nIndexCount = skullIndices.size();
    skullSubmesh.nStartIndexLocation = 0;
    skullSubmesh.nBaseVertexLocation = 0;

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
    wood.nRoughness = 0.2f;
    wood.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    wood.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};
    
    Light::Material brick;
    brick.iFramesDirty = 3;
    brick.Name = "brick";
    brick.nCBMaterialIndex = 1;
    brick.nDiffuseSrvHeapIndex = 1;
    brick.nRoughness = 0.1f;
    brick.vec3FresnelR0 = {0.02f, 0.02f, 0.02f};
    brick.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material skull;
    skull.iFramesDirty = 3;
    skull.Name = "skull";
    skull.nCBMaterialIndex = 2;
    skull.nDiffuseSrvHeapIndex = 2;
    skull.nRoughness = 0.3f;
    skull.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    skull.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material stone;
    stone.iFramesDirty = 3;
    stone.Name = "stone";
    stone.nCBMaterialIndex = 3;
    stone.nDiffuseSrvHeapIndex = 3;
    stone.nRoughness = 0.3f;
    stone.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    stone.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material tile;
    tile.iFramesDirty = 3;
    tile.Name = "tile";
    tile.nCBMaterialIndex = 4;
    tile.nDiffuseSrvHeapIndex = 4;
    tile.nRoughness = 0.3f;
    tile.vec3FresnelR0 = {0.02f, 0.02f, 0.02f};
    tile.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Materials["wood"] = wood;
    Materials["brick"] = brick;
    Materials["skull"] = skull;
    Materials["tile"] = tile;
    Materials["stone"] = stone;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
    {
        pFrameResources[i].InitOtherConstantBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
        Init(pD3dDevice.Get(), CONSTANT_VALUE::nCBMaterialByteSize, Materials.size());
    }    

}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry info;

    RenderItem box;
    box.iFramesDirty = 3;
    box.nCBObjectIndex = 0;
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
    grid.nCBObjectIndex = 1;
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
    UINT objIndex = 2;
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
        lSI.pMaterial = &Materials["stone"];        // TODO: Load stone texture
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
        rSI.pMaterial = &Materials["stone"];
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

    for(UINT i = 0; i < AllRenderItems.size(); ++i)
        RenderItems[Opaque].push_back(&AllRenderItems[i]);

}

void D3DFrame::BuildRootSignatures()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, AllRenderItems.size());
    
    CD3DX12_STATIC_SAMPLER_DESC sampler[1];
    sampler[0].Init(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    CD3DX12_DESCRIPTOR_RANGE range[1];
    CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;

    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    params[0].InitAsConstantBufferView(0);
    params[1].InitAsConstantBufferView(1);
    params[2].InitAsConstantBufferView(2);
    params[3].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignDesc.Init(4, params, 1, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializeRootSign;
    ComPtr<ID3DBlob> error;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializeRootSign, &error), &error);

    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, serializeRootSign->GetBufferPointer(), serializeRootSign->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
}

void D3DFrame::BuildPipelineStates()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
    D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Shader
    D3D_SHADER_MACRO NormalDefine[] = {
        "FOG", "1",
        NULL, NULL
    };
    D3D_SHADER_MACRO AlphaTestedDefine[] = {
        "FOG", "1",
        "ALPHA_TESTED", "1",
        NULL, NULL
    };

    std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;

    Shaders["Normal_Vs"] = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Normal_Ps"] = CompileShader(L"shader.hlsl", NormalDefine, "PsMain", "ps_5_1");
    Shaders["AlphaTested_Ps"] = CompileShader(L"shader.hlsl", AlphaTestedDefine, "PsMain", "ps_5_1");
    
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
    PipelineStates[Opaque] = pPipelineState;

    // AlphaTested
    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTest = desc;
    alphaTest.PS = {Shaders["AlphaTested_Ps"]->GetBufferPointer(), Shaders["AlphaTested_Ps"]->GetBufferSize()};
    alphaTest.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&alphaTest, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[AlphaTested] = pPipelineState;

    // Transparent
    D3D12_GRAPHICS_PIPELINE_STATE_DESC trans = desc;
    D3D12_RENDER_TARGET_BLEND_DESC BlendDesc = {};
    BlendDesc.BlendEnable = 1;
    BlendDesc.LogicOpEnable = 0;
    BlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    BlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    BlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    BlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    BlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    BlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    BlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    BlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    trans.BlendState.RenderTarget[0] = BlendDesc;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&trans, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Transparent] = pPipelineState;
}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = Textures.size();
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());
    ID3D12Resource* res = Textures["wood"].pResource.Get();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    res = Textures["brick"].pResource.Get();
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    res = Textures["white"].pResource.Get();
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
	handle.Offset(1, nCbvSrvUavDescriptorSize);

	res = Textures["stone"].pResource.Get();
	srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
	handle.Offset(1, nCbvSrvUavDescriptorSize);

	res = Textures["tile"].pResource.Get();
	srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
	
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
    camera.UpdateViewMatrix();
    InputProcess(t);
    UpdateObjects(t);
    UpdateMaterials(t);
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
//            mat.nDiffuseTextureIndex = matItem.nDiffuseSrvHeapIndex;

            pCurrFrameResource->CBMaterial.CopyData(matItem.nCBMaterialIndex, &mat, CONSTANT_VALUE::nCBMaterialByteSize);
            --matItem.iFramesDirty;
        }
    }
}

void D3DFrame::UpdateObjects(const GameTimer& t)
{
    for(auto item: AllRenderItems)
    {
        if(item.iFramesDirty > 0)
        {
            Constant::ObjectConstant obj;
            XMMATRIX world = XMLoadFloat4x4(&item.matWorld);
            XMMATRIX texTrans = XMLoadFloat4x4(&item.matTexTransform);

//            obj.nIndex = item.pMaterial->nCBMaterialIndex;
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
    
    XMStoreFloat4x4(&sc.matView, view);
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));
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
    sc.lights[0].vec3Direction = { 0.57735f, -0.57735f, 0.57735f };
    sc.lights[0].vec3Strength = { 0.6f, 0.6f, 0.6f };
    sc.lights[1].vec3Direction = { -0.57735f, -0.57735f, 0.57735f };
    sc.lights[1].vec3Strength = { 0.3f, 0.3f, 0.3f };
    sc.lights[2].vec3Direction = { 0.0f, -0.707f, -0.707f };
    sc.lights[2].vec3Strength = {0.15f, 0.15f, 0.15f};
    pCurrFrameResource->CBScene.CopyData(0, &sc, CONSTANT_VALUE::nCBSceneByteSize);
}

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    camera.SetLens(45.0f, AspectRatio(), 0.1f, 1000.0f);
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
    
    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap* Heaps[] = {pSRVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, Heaps);

    pCommandList->SetPipelineState(PipelineStates[Opaque].Get());
    pCommandList->SetGraphicsRootConstantBufferView(2, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());

    DrawItems(pCommandList.Get(), RenderItems[Opaque]);

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

void D3DFrame::DrawItems(ID3D12GraphicsCommandList* list, std::vector<RenderItem*>& items)
{
    for(UINT i = 0; i < items.size(); ++i)
    {
        RenderItem* item = items[i];

        list->IASetPrimitiveTopology(item->emPrimitiveType);
        list->IASetVertexBuffers(0, 1, &item->pGeo->GetVertexBufferView());
        list->IASetIndexBuffer(&item->pGeo->GetIndexBufferView());

        // Object
        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item->nCBObjectIndex * CONSTANT_VALUE::nCBObjectByteSize;
        list->SetGraphicsRootConstantBufferView(0, addr);

        addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += item->pMaterial->nCBMaterialIndex * CONSTANT_VALUE::nCBMaterialByteSize;
        list->SetGraphicsRootConstantBufferView(1, addr);

        D3D12_GPU_DESCRIPTOR_HANDLE handle = pSRVHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += item->pMaterial->nDiffuseSrvHeapIndex * nCbvSrvUavDescriptorSize;
        list->SetGraphicsRootDescriptorTable(3, handle);
        
        list->DrawIndexedInstanced(item->nIndexCount, 1, item->nStartIndexLocation, item->nBaseVertexLocation, 0);
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
