#define D3D12_DYNAMIC_INDEX
#include "D3DFrame.h"

void LoadCarModel(std::vector<CarModelVertex>&, std::vector<CarModelIndex>&);

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
    
    camera.LookAt(XMFLOAT3(5.0f, 4.0f, -15.0f), {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

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
  
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/white1x1.dds";
    tex.Name = "while";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures["white"] = tex;
}

void D3DFrame::BuildGeometries()
{
    std::vector<CarModelVertex> carVertices;
    std::vector<CarModelIndex> indices;

    std::vector<Vertex> vertices;
    LoadCarModel(carVertices, indices);
    
    vertices.resize(carVertices.size());

    XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

    for(UINT i = 0; i < carVertices.size(); ++i)
    {
        XMVECTOR P = XMLoadFloat3(&XMFLOAT3(carVertices[i].Position));

        vertices[i].Position = XMFLOAT3(carVertices[i].Position);
        vertices[i].Normal = XMFLOAT3(carVertices[i].Normal);
        vertices[i].TexCoords = {0, 0};

        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }
    
    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f * (vMin + vMax));
    XMStoreFloat3(&bounds.Extents, 0.5f * (vMax - vMin));
    
    const UINT vertexBytesSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT indexBytesSize = (UINT)indices.size() * sizeof(CarModelIndex);

    Resource::MeshGeometry model;
    model.Name = "Car";

    // 将顶点数据存储到副本中
    ThrowIfFailed(D3DCreateBlob(vertexBytesSize, &model.pCPUVertexBuffer));
    CopyMemory(model.pCPUVertexBuffer->GetBufferPointer(), &vertices[0], vertexBytesSize);
    ThrowIfFailed(D3DCreateBlob(indexBytesSize, &model.pCPUIndexBuffer));
    CopyMemory(model.pCPUIndexBuffer->GetBufferPointer(), &indices[0], indexBytesSize);

    model.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], indexBytesSize, &model.pUploaderIndexBuffer);
    model.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], vertexBytesSize, &model.pUploaderVertexBuffer);

    model.nIndexBufferByteSize = indexBytesSize;
    model.nVertexBufferByteSize = vertexBytesSize;
    model.nVertexByteStride = sizeof(Vertex);

    Resource::SubmeshGeometry main;
    main.Bounds = bounds;
    main.nBaseVertexLocation = 0;
    main.nStartIndexLocation = 0;
    main.nIndexCount = (UINT)indices.size() * 3;    

    model.DrawArgs["main"] = main;
    Geos["car"] = model;
}   

void D3DFrame::BuildMaterials()
{
    Light::Material gray, highLight;

    gray.iFramesDirty = 3;
    gray.Name = "gray";
    gray.nCBMaterialIndex = 0;
    gray.nDiffuseSrvHeapIndex = 0;
    gray.nRoughness = 0.0f;
    gray.vec3FresnelR0 = {0.04f, 0.04f, 0.04f};
    gray.vec4DiffuseAlbedo = {0.7f, 0.7f, 0.7f, 1.0f};
    gray.matTransform = MathHelper::Identity4x4();
    
    highLight.iFramesDirty = 3;
    highLight.Name = "highLight";
    highLight.nCBMaterialIndex = 1;
    highLight.nDiffuseSrvHeapIndex = 0;
    highLight.nRoughness = 0.0f;
    highLight.vec3FresnelR0 = {0.06f, 0.06f, 0.06f};
    highLight.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};
    highLight.matTransform = MathHelper::Identity4x4();

    Materials["gray"] = gray;
    Materials["highLight"] = highLight;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
    {
		// 上传缓冲区不应当进行内存对齐
        pFrameResources[i].InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
        Init(pD3dDevice.Get(), sizeof(Constant::MaterialConstant), Materials.size());
    }    

}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry main;
    RenderItem item;
    item.iFramesDirty = 3;
    item.pGeo = &Geos["car"];
    item.pMaterial = &Materials["gray"];
    item.nCBObjectIndex = 0;
    
    main = item.pGeo->DrawArgs["main"];
    item.nIndexCount = main.nIndexCount;
    item.nBaseVertexLocation = main.nBaseVertexLocation;
    item.nStartIndexLocation = main.nStartIndexLocation;
    item.Bounds = main.Bounds;

    XMStoreFloat4x4(&item.matWorld, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    XMStoreFloat4x4(&item.matTexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));

    AllRenderItems.push_back(item);
    RenderItems[RENDER_TYPE_OPEAQUE].push_back(0);

    RenderItem picking;
    picking.nCBObjectIndex = 1;
    picking.pGeo = &Geos["car"];
    picking.pMaterial = &Materials["highLight"];

    picking.nIndexCount = 0;
    picking.nStartIndexLocation = 0;
    picking.nBaseVertexLocation = 0;

    picking.bVisible = 0;
    picking.matTexTransform = MathHelper::Identity4x4();
    picking.matWorld = MathHelper::Identity4x4();

    AllRenderItems.push_back(picking);
    RenderItems["highLight"].push_back(1);

}

void D3DFrame::BuildRootSignatures()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, AllRenderItems.size());
    CD3DX12_DESCRIPTOR_RANGE range[1];
	CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_STATIC_SAMPLER_DESC sampler[1];

    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    params[0].InitAsConstantBufferView(0);
    params[1].InitAsConstantBufferView(1);
    params[2].InitAsShaderResourceView(0, 1);
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

    D3D12_GRAPHICS_PIPELINE_STATE_DESC highLight = desc;

    // 将深度测试改为 <= 方便被选中的图元进行二次绘制
    highLight.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
    transparencyBlendDesc.BlendEnable = 1;
    transparencyBlendDesc.LogicOpEnable = 0;
    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    highLight.BlendState.RenderTarget[0] = transparencyBlendDesc;
    
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&highLight, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates["highLight"] = pPipelineState;

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
    ID3D12Resource* res = Textures["white"].pResource.Get();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = res->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = res->GetDesc().MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
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
            mat.nDiffuseTextureIndex = matItem.nDiffuseSrvHeapIndex;

            pCurrFrameResource->CBMaterial.CopyData(matItem.nCBMaterialIndex, &mat, sizeof(Constant::MaterialConstant));
            --matItem.iFramesDirty;
        }
    }
}

void D3DFrame::UpdateObjects(const GameTimer& t)
{
    for(auto& item: AllRenderItems)
    {
        if(item.bVisible && item.iFramesDirty > 0)
        {
            Constant::ObjectConstant obj;
            XMStoreFloat4x4(&obj.matWorld, XMMatrixTranspose(XMLoadFloat4x4(&item.matWorld)));
            XMStoreFloat4x4(&obj.matTexTransform, XMMatrixTranspose(XMLoadFloat4x4(&item.matTexTransform)));
            obj.nMaterialIndex = item.pMaterial->nCBMaterialIndex;

            pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &obj, sizeof(Constant::ObjectConstant));
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
    if(state & MK_LBUTTON)
    {
        lastPos.x = x;
        lastPos.y = y;

        SetCapture(hwnd);
    }
    else if(state & MK_RBUTTON)
    {
        Pick(x, y);
    }
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
    pCommandList->SetGraphicsRootConstantBufferView(1, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootShaderResourceView(2, pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress());
    pCommandList->SetGraphicsRootDescriptorTable(3, pSRVHeap->GetGPUDescriptorHandleForHeapStart());

    DrawItems(pCommandList.Get(), RenderItems[RENDER_TYPE_OPEAQUE]);

    pCommandList->SetPipelineState(PipelineStates["highLight"].Get());
    
    DrawItems(pCommandList.Get(), RenderItems["highLight"]);

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
        
        if(!item.bVisible)
            continue;
        
        list->IASetPrimitiveTopology(item.emPrimitiveType);
        list->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        list->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        list->SetGraphicsRootConstantBufferView(0, pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress() + CONSTANT_VALUE::nCBObjectByteSize * item.nCBObjectIndex);

        list->DrawIndexedInstanced(item.nIndexCount, item.nInstanceCount, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }
}

void D3DFrame::Pick(int x, int y)
{
    RenderItem* pickedItem = &AllRenderItems[1];

    XMFLOAT4X4 P = camera.GetProj4x4f();

    // 将拾取射线变换到观察空间
    float vx = (2.0f * x / cxClient - 1.0f) / P(0, 0);
    float vy = (-2.0f * y / cyClient + 1.0f) / P(1, 1);

    // 定义拾取射线
    XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

    XMMATRIX V = camera.GetView();
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

    pickedItem->bVisible = 0;

    for(auto index: RenderItems[RENDER_TYPE_OPEAQUE])
    {
        auto& item = AllRenderItems[index];
        
        if(!item.bVisible)
            continue;
        
        // 将拾取射线变换到物体的局部空间
        XMMATRIX W = XMLoadFloat4x4(&item.matWorld);
        XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

        XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

        rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
        rayDir = XMVector3Normalize(XMVector3TransformNormal(rayDir, toLocal));

        float tMin = 0.0f;

        // 如果拾取射线与该物体的碰撞盒相交
        // 则进行下一步的处理
        if(item.Bounds.Intersects(rayOrigin, rayDir, tMin))
        {
            // 获取数据的内存副本
            Vertex* vertices = (Vertex*)item.pGeo->pCPUVertexBuffer->GetBufferPointer();
            UINT16* indices = (UINT16*)item.pGeo->pCPUIndexBuffer->GetBufferPointer();
            UINT triangleCount = item.nIndexCount / 3;

            // 搜索与拾取射线相交的最近三角形图元
            tMin = FLT_MAX;
            for(UINT i = 0; i < triangleCount; ++i)
            {
                // 当前三角形的顶点索引
                UINT i0 = indices[i * 3 + 0];
                UINT i1 = indices[i * 3 + 1];
                UINT i2 = indices[i * 3 + 2];

                // 当前三角形图元顶点
                XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Position);
                XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Position);
                XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Position);
                
                float t = 0;
                // 令拾取射线与所有的三角形图元进行碰撞检测
                // 其中返回的 t 为射线公式: origin + t * direction 中的 t
                if(TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t))
                {
                    if(t < tMin)
                    {
                        tMin = t;

                        pickedItem->bVisible = 1;
                        pickedItem->nIndexCount = 3;
                        pickedItem->nBaseVertexLocation = 0;
                        
                        pickedItem->matWorld = item.matWorld;
                        pickedItem->iFramesDirty = 3;

                        pickedItem->nStartIndexLocation = 3 * i;
                    }
                }
            }
        }
    }
}

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices)
{
    const UINT nVertexCount = 31076, nTriangleCount = 60339;
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pEnd, *pBlock;
    UINT iVertexCounter = 0, iTriangleCounter = 0;
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

    vertices.resize(nVertexCount);
    indices.resize(nTriangleCount);
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
        if(iVertexCounter == nVertexCount)
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

            indices[iTriangleCounter].i[j] = atoi(pBegin);
        }
        
        ++iTriangleCounter;
        if(iTriangleCounter == nTriangleCount)
            break;
    }
    free(pFileBuffer);
}

void LoadCarModel(std::vector<CarModelVertex>& vertices, std::vector<CarModelIndex>& indices)
{
    const UINT nVertexCount = 1860, nTriangleCount = 1850;
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pEnd, *pBlock;
    UINT iVertexCounter = 0, iTriangleCounter = 0;
    UINT j;

//****** Read File
    if((hFile = CreateFileA("C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Models/car.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return;
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = (char*)malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

    vertices.resize(nVertexCount);
    indices.resize(nTriangleCount);
    
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
        if(iVertexCounter == nVertexCount)
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

            indices[iTriangleCounter].i[j] = atoi(pBegin);
        }
        
        ++iTriangleCounter;
        if(iTriangleCounter == nTriangleCount)
            break;
    }

    free(pFileBuffer);
}