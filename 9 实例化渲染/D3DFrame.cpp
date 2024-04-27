#define D3D12_DYNAMIC_INDEX
#define D3D12_INSTANCE_RENDER
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
    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> indices;

    LoadSkullModel(skullVertices, indices);

    std::vector<Vertex> vertices(skullVertices.size());

    XMVECTOR vMin = XMLoadFloat3(&XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX));
    XMVECTOR vMax = XMLoadFloat3(&XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
    for(UINT i = 0; i < (UINT)skullVertices.size(); ++i)
    {
        XMVECTOR P = XMLoadFloat3(&XMFLOAT3(skullVertices[i].Position));

        XMFLOAT3 spherePos;
        XMStoreFloat3(&spherePos, XMVector3Normalize(P));

        float theta = atan2f(spherePos.z, spherePos.x);

        if(theta < 0.0f)
            theta += XM_2PI;
        
        float phi = acosf(spherePos.y);

        float u = theta / (2.0f * XM_PI);
        float v = phi / XM_PI;

        vertices[i].TexCoords = {u, v};
        vertices[i].Position = XMFLOAT3(skullVertices[i].Position);
        vertices[i].Normal = XMFLOAT3(skullVertices[i].Normal);

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));


    const UINT vertexBytesSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indexBytesSize = (UINT)indices.size() * sizeof(SkullModelIndex);

    Resource::MeshGeometry skull;
    skull.Name = "Skull";

    skull.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], indexBytesSize, &skull.pUploaderIndexBuffer);
    skull.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &skull.pUploaderVertexBuffer);

    skull.nIndexBufferByteSize = indexBytesSize;
    skull.nVertexBufferByteSize = vertexBytesSize;
    skull.nVertexByteStride = sizeof(Vertex);

    Resource::SubmeshGeometry main;
    main.Bounds = bounds;
    main.nBaseVertexLocation = 0;
    main.nStartIndexLocation = 0;
    main.nIndexCount = (UINT)indices.size() * 3;    

    skull.DrawArgs["main"] = main;
    Geos["skull"] = skull;
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
    stone.nCBMaterialIndex = 4;
    stone.nDiffuseSrvHeapIndex = 4;
    stone.nRoughness = 0.3f;
    stone.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    stone.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};

    Light::Material tile;
    tile.iFramesDirty = 3;
    tile.Name = "tile";
    tile.nCBMaterialIndex = 3;
    tile.nDiffuseSrvHeapIndex = 3;
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
		// 上传缓冲区不应当进行内存对齐
        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
        Init(pD3dDevice.Get(), sizeof(Constant::MaterialConstant), Materials.size());
    }    

}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry info;
    RenderItem item;
    item.iFramesDirty = 3;
    item.pGeo = &Geos["skull"];
    
    info = item.pGeo->DrawArgs["main"];
    item.nIndexCount = info.nIndexCount;
    item.nBaseVertexLocation = info.nBaseVertexLocation;
    item.nStartIndexLocation = info.nBaseVertexLocation;
    item.Bounds = info.Bounds;

    const UINT n = 5;
    item.nInstanceTotal = n * n * n;
    item.nInstanceCount = 0;
    
    item.InstanceDatas.resize(item.nInstanceTotal);

    float width = 200.0f, height = 200.0f, depth = 200.0f;
    float x = -0.5f * width, y = -0.5f * height, z = -0.5f * depth;
    float dx = width / (n - 1), dy = height / (n - 1), dz = depth / (n - 1);

    for(UINT k = 0; k < n; ++k)
    {
        for(UINT i = 0; i < n; ++i)
        {
            for(UINT j = 0; j < n; ++j)
            {
                UINT index = k * n * n + i * n + j;
                
                item.InstanceDatas[index].matWorld = XMFLOAT4X4(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    x + j * dx, y + i * dy, z + k * dz, 1.0f
                );

                XMStoreFloat4x4(&item.InstanceDatas[index].matTexTransform, XMMatrixScaling(2.0f, 2.0f, 1.0f));
                item.InstanceDatas[index].nMaterialIndex = index % Materials.size();
            }
        }
    }

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        item.nInstanceIndex = pFrameResources[i].InitInstanceBuffer(pD3dDevice.Get(), sizeof(InstanceData), item.nInstanceTotal);

    AllRenderItems.push_back(item);
    RenderItems[RENDER_TYPE_OPEAQUE].push_back(0);
}

void D3DFrame::BuildRootSignatures()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, 0);
    CD3DX12_DESCRIPTOR_RANGE range[1];
	CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_STATIC_SAMPLER_DESC sampler[1];

    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);
    params[0].InitAsConstantBufferView(0);
    params[1].InitAsShaderResourceView(0, 1);
    params[2].InitAsShaderResourceView(1, 1);
    params[3].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_PIXEL);

    sampler[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT);

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(4, params, 1, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Shader
    D3D_SHADER_MACRO NormalDefine[] = {
        "FOG", "1",
        NULL, NULL
    };
  
    std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;

    Shaders["Normal_Vs"] = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_1");
    Shaders["Normal_Ps"] = CompileShader(L"shader.hlsl", NormalDefine, "PsMain", "ps_5_1");
    
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

	res = Textures["tile"].pResource.Get();
	srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    pD3dDevice->CreateShaderResourceView(res, &srvDesc, handle);
	handle.Offset(1, nCbvSrvUavDescriptorSize);
	
	res = Textures["stone"].pResource.Get();
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
    UpdateInstanceDatas(t);
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
            mat.nDiffuseTextureIndex = matItem.nDiffuseSrvHeapIndex;

            pCurrFrameResource->CBMaterial.CopyData(matItem.nCBMaterialIndex, &mat, sizeof(Constant::MaterialConstant));
            --matItem.iFramesDirty;
        }
    }
}

void D3DFrame::UpdateInstanceDatas(const GameTimer& t)
{
    XMMATRIX view = camera.GetView();
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

    for(auto& e: AllRenderItems)
    {
        auto currInstanceBuffer = &pCurrFrameResource->CBInstances[e.nInstanceIndex];
        auto& instanceDatas = e.InstanceDatas;

        e.nInstanceCount = 0;
        for(UINT i = 0; i < (UINT)instanceDatas.size(); ++i)
        {
            XMMATRIX world = XMLoadFloat4x4(&instanceDatas[i].matWorld);
            XMMATRIX texTransform = XMLoadFloat4x4(&instanceDatas[i].matTexTransform);

            XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

            XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

            BoundingFrustum localSpaceFrustum;
            cameraFrustum.Transform(localSpaceFrustum, viewToLocal);

            if(localSpaceFrustum.Contains(e.Bounds) != DirectX::DISJOINT)
            {
                InstanceData data;
                XMStoreFloat4x4(&data.matWorld, XMMatrixTranspose(world));
                XMStoreFloat4x4(&data.matTexTransform, XMMatrixTranspose(texTransform));
                data.nMaterialIndex = instanceDatas[i].nMaterialIndex;

                currInstanceBuffer->CopyData(e.nInstanceCount++, &data, sizeof(InstanceData));
            }
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

    camera.SetLens( 0.25 * XM_PI, AspectRatio(), 0.1f, 1000.0f);
    BoundingFrustum::CreateFromMatrix(cameraFrustum, camera.GetProj());
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

    pCommandList->SetPipelineState(PipelineStates[RENDER_TYPE_OPEAQUE].Get());
    pCommandList->SetGraphicsRootConstantBufferView(0, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootShaderResourceView(1, pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootDescriptorTable(3, pSRVHeap->GetGPUDescriptorHandleForHeapStart());

    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPEAQUE]);

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

void D3DFrame::DrawItems(ID3D12GraphicsCommandList* list, std::vector<UINT>& items)
{
    for(UINT i : items)
    {
        RenderItem& item = AllRenderItems[i];

        list->IASetPrimitiveTopology(item.emPrimitiveType);
        list->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        list->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        list->SetGraphicsRootShaderResourceView(2, pCurrFrameResource->CBInstances[item.nInstanceIndex].Resource()->GetGPUVirtualAddress());

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
