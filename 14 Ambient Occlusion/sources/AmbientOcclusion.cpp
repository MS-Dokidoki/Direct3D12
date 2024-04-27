#include "AmbientOcclusion.h"
using namespace DirectX;

AmbientOcclusion::AmbientOcclusion(HINSTANCE hInst): D3DApp(hInst)
{}

AmbientOcclusion::~AmbientOcclusion()
{}

bool AmbientOcclusion::Initialize()
{
    if(!D3DApp::Initialize())
        return 0;

    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    LoadTextures();
    BuildGeometries();

    ThrowIfFailed(pCommandList->Close());

    ID3D12CommandList* cmdLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    BuildMaterialsAndSrv();
    BuildRenderItems();
    BuildRootSignatures();
    BuildPipelineStates();

    pCurrFrameResource->InitConstantBuffer(pD3dDevice.Get(), 1, AllRenderItems.size());

    FlushCommandQueue();
    // clear memory
    for(auto& item: Textures)
        item.second.pUploader = nullptr;
    for(auto& item: Geos)
        item.second.DisposeUploader();
    
}

void AmbientOcclusion::Update(const GameTimer& t)
{

}

void AmbientOcclusion::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 0, NULL);
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    const float Colors[] = {1.0f, 0.0f, 1.0f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors, 0, NULL);

     pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    ThrowIfFailed(pCommandList->Close());

    ID3D12CommandList* Lists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(_countof(Lists), Lists);

    ThrowIfFailed(pSwapChain->Present(0, 0));

    iCurrBackBuffer = (iCurrBackBuffer + 1) % nSwapChainBufferCount;
    FlushCommandQueue();
}

void AmbientOcclusion::LoadTextures()
{
    TextureList.push_back(TextureListItem("default", PROJECT("/Resources/white1x1.dds")));
	TextureList.push_back(TextureListItem("deafult_norm", PROJECT("/Resources/default_normal.dds")));
	TextureList.push_back(TextureListItem("grassCube", PROJECT("/Resources/grasscube1024.dds")));
	
    _LoadTextures(pD3dDevice.Get(), pCommandList.Get(), TextureList, Textures);
}

void AmbientOcclusion::BuildGeometries()
{
    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> skullIndices;
    UINT nVertexByteSize, nIndexByteSize;
    Vertex* vertices;
    UINT* indices;

    LoadSkullModel(skullVertices, skullIndices);

    // too big!
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
    skull.Submeshes["Main"] = submesh;

    GeoList["skull"] = skull;
    _BuildGeometries(pD3dDevice.Get(), pCommandList.Get(), Geos, GeoList);
}

void AmbientOcclusion::BuildMaterialsAndSrv()
{
	XMFLOAT4X4 matIdentity = MathHelper::Identity4x4();
    MaterialList.push_back(MaterialListItem(
        D3D12_SRV_DIMENSION_TEXTURE2D,
        "skull",
        {.3f, .3f, .3f, 1.0f},
        {.6f, .6f, .6f},
		matIdentity,
        0.2f, 
        "default", "deafult_norm"
    ));
	
	MaterialList.push_back(MaterialListItem(
		D3D12_SRV_DIMENSION_TEXTURECUBE,
		"sky",
		{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		matIdentity,
		1.0f,
		"grassCube", "deafult_norm"
	));

    _BuildMaterials(MaterialList, SrvList, Textures, Materials, nCBMaterialCounter, nSrvIndexCounter);

    pCurrFrameResource->InitOtherBuffer(Resource::FrameResource::FRAME_RESOURCE_TYPE_MATERIAL).
    Init(pD3dDevice.Get(), CONSTANT_VALUE::nCBMaterialByteSize, Materials.size());

    _BuildSRV(pD3dDevice.Get(), pSRVHeap.Get(), SrvList, nCbvSrvUavDescriptorSize);
}

void AmbientOcclusion::BuildRenderItems()
{
    RenderItem item;
    item.iFramesDirty = 3;
    item.bVisible = 1;
    item.nCBObjectIndex = nCBObjectCounter++;
    item.pGeo = &Geos["skull"];
    item.pMaterial = &Materials["skull"];

    Resource::SubmeshGeometry& submesh = item.pGeo->DrawArgs["Main"];
    item.nBaseVertexLocation = submesh.nBaseVertexLocation;
    item.nStartIndexLocation = submesh.nStartIndexLocation;
    item.nIndexCount = submesh.nIndexCount;

    item.matTexTransform = MathHelper::Identity4x4();
    item.matWorld = MathHelper::Identity4x4();

    AllRenderItems.push_back(item);
    RenderItems[RENDERITEM_TYPE_OPAQUE].push_back(0);
}

void AmbientOcclusion::BuildRootSignatures()
{
    CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_DESCRIPTOR_RANGE range[2];
    CD3DX12_STATIC_SAMPLER_DESC sampler[7];
	
	GetStaticSampler(sampler);
    
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);	// Sky Cube
    range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 16, 1, 0);	// Textures
    
    params[0].InitAsConstantBufferView(0);  // Object
    params[1].InitAsConstantBufferView(1);  // World
	params[2].InitAsShaderResourceView(0, 1);	// Materials
    params[3].InitAsDescriptorTable(2, range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(4, params, 7, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerializeSign, pError;

    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeSign, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeSign->GetBufferPointer(), pSerializeSign->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

}

void AmbientOcclusion::BuildPipelineStates()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> Shaders;

#define SHADER(name) {Shaders[name]->GetBufferPointer(), Shaders[name]->GetBufferSize()}
#define CreatePipelineState(type, desc) ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState))); PipelineStates[type] = pPipelineState

    D3D12_INPUT_ELEMENT_DESC layouts[] ={
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };


    Shaders["VS_Main"] = CompileShader(PROJECT("/shaders/shader.hlsl"), NULL, "VsMain", "vs_5_0");
    Shaders["PS_Main"] = CompileShader(PROJECT("/shaders/shader.hlsl"), NULL, "PsMain", "ps_5_0");
	Shaders["VS_SkyMain"] = CompileShader(PROJECT("/shaders/sky.hlsl"), NULL, "VsMain", "vs_5_0");
	Shaders["PS_SkyMain"] = CompileShader(PROJECT("/shaders/sky.hlsl"), NULL, "PSMain", "ps_5_0");
	
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    desc.InputLayout = {layouts, _countof(layouts)};
    desc.pRootSignature = pRootSignature.Get();
    desc.VS = SHADER("VS_Main");
    desc.PS = SHADER("PS_Main");
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DSVFormat = emDepthStencilFormat;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleMask = UINT_MAX;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = emBackBufferFormat;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    CreatePipelineState(PIPELINESTATE_TYPE_OPAQUE, desc);

}

void AmbientOcclusion::UpdateObjects(const GameTimer& t)
{
    for(UINT i = 0; i < AllRenderItems.size(); ++i)
    {
        RenderItem* item = &AllRenderItems[i];
        
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

void AmbientOcclusion::UpdateScene(const GameTimer& t)
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
    XMStoreFloat4x4(&sc.matShadowTransform, XMMatrixTranspose(view));
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

    sc.lights[0].vec3Direction = {1.0f, -1.0f, 1.0f};
    sc.lights[0].vec3Strength = { 0.6f, 0.6f, 0.6f };
    sc.lights[1].vec3Direction = {-1.0f, -1.0f, 1.0f};
    sc.lights[1].vec3Strength = { 0.3f, 0.3f, 0.3f };
    sc.lights[2].vec3Direction = {0.0f, -1.0f, -1.0f};
    sc.lights[2].vec3Strength = {0.15f, 0.15f, 0.15f};

    pCurrFrameResource->CBScene.CopyData(0, &sc, CONSTANT_VALUE::nCBSceneByteSize);    
}