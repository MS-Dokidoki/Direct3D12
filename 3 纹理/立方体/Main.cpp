#include "D3DApp.h"

#define USE_FRAMERESOURCE // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);

    virtual bool Initialize();

protected:
    virtual void Render(const GameTimer &);
    virtual void Update(const GameTimer &);
    virtual void OnResize();
    void OnMouseMove(WPARAM, int, int);
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);

    void LoadTextures();
    void BuildGeometries();
    void BuildMaterials();
    void BuildRenderItems();
    void BuildSRV();
    void BuildRootSignature();

    void UpdateCamera(const GameTimer &);
    void UpdateObjects(const GameTimer &);
    void UpdateScene(const GameTimer &);
    void UpdateMaterials(const GameTimer &);

private:
    ComPtr<ID3D12PipelineState> pPipelineState;
    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;

    std::vector<Render::RenderItem> RenderItems;
    Render::MeshGeometry Geo;

    std::unordered_map<std::string, Render::Texture> Textures;
    std::unordered_map<std::string, Light::Material> Materials;

    float fTheta = 1.5f * XM_PI;
    float fPhi = XM_PIDIV2 - 0.1f;
    float fRadius = 20.0f;
    POINT lastPos;

    XMFLOAT3 vec3EyePos;
    XMFLOAT4X4 matView, matProj;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    try
    {
        D3DFrame app(hInstance);

        if (!app.Initialize())
            return 0;
        return app.Run();
    }
    catch (DXException &e)
    {
        MessageBox(NULL, e.ToString(), TEXT("HR Failed"), MB_OK);
    }

    return 0;
}

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

bool D3DFrame::Initialize()
{
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;
#endif
    if (!D3DApp::Initialize())
        return 0;

    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    LoadTextures();

    BuildGeometries();
    BuildMaterials();
    BuildRenderItems();
    BuildSRV();
    BuildRootSignature();

    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList *ppCmdLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCmdLists);
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, RenderItems.size());

    // Shader
    ComPtr<ID3DBlob> pVsByteCode, pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    // Pipeline
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
    pipeline_desc.InputLayout = {inputElements, _countof(inputElements)};
    pipeline_desc.pRootSignature = pRootSignature.Get();
    pipeline_desc.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    pipeline_desc.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    pipeline_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pipeline_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipeline_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pipeline_desc.DSVFormat = emDepthStencilFormat;
    pipeline_desc.NumRenderTargets = 1;
    pipeline_desc.RTVFormats[0] = emBackBufferFormat;
    pipeline_desc.SampleMask = UINT_MAX;
    pipeline_desc.SampleDesc = {1, 0};
    pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&pPipelineState)));
    FlushCommandQueue();
    return 1;
}

void D3DFrame::Update(const GameTimer &t)
{
#ifdef USE_FRAMERESOURCE
    // 在当前帧命令存放之前，先进行命令分配器的切换
    iCurrFrameResourceIndex = (iCurrFrameResourceIndex + 1) % nFrameResourceCount;
    pCurrFrameResource = &pFrameResources[iCurrFrameResourceIndex];
    pCommandAllocator = pCurrFrameResource->pCommandAllocator;

    // 若当前帧资源未执行完成，说明命令分配器全都被 GPU 使用，等待执行完成
    if (pCurrFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrFrameResource->iFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(pCurrFrameResource->iFence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }
#endif

    UpdateCamera(t);
    UpdateObjects(t);
    UpdateMaterials(t);
    UpdateScene(t);
}

void D3DFrame::Render(const GameTimer &t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));
    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
               ppSwapChainBuffer[iCurrBackBuffer].Get(),
               D3D12_RESOURCE_STATE_PRESENT,
               D3D12_RESOURCE_STATE_RENDER_TARGET));

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 1, &DepthStencilBufferView());
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    //
    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap *ppHeaps[] = {pSRVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, ppHeaps);
    pCommandList->SetGraphicsRootConstantBufferView(3, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());

    for (UINT i = 0; i < RenderItems.size(); ++i)
    {
        Render::RenderItem item = RenderItems[i];

        pCommandList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item.nCBObjectIndex * CONSTANT_VALUE::nCBObjectByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(1, addr);           // Object

        addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += item.pMaterial->nCBIndex * CONSTANT_VALUE::nCBMaterialByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(2, addr);           // Material

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(item.pMaterial->nDiffuseSrvHeapIndex, nCbvSrvUavDescriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(0, handle);

        pCommandList->DrawIndexedInstanced(item.nIndexCount, 1, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }

    //

    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
               ppSwapChainBuffer[iCurrBackBuffer].Get(),
               D3D12_RESOURCE_STATE_RENDER_TARGET,
               D3D12_RESOURCE_STATE_PRESENT));
    pCommandList->Close();

    ID3D12CommandList *ppCommandLists[] = {pCommandList.Get()};
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

void D3DFrame::LoadTextures()
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    std::unique_ptr<UINT8[]> ddsData;

    Render::Texture wood;
    wood.Name = "wood";
    wood.FileName = L"C:\\Users\\Administrator.PC-20191006TRUC\\source\\repos\\DirectX\\Resources\\woodBox.dds";
    
    LoadDDSTextureFromFile(pD3dDevice.Get(), wood.FileName.c_str(), &wood.pResource, ddsData, subresources);
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(wood.pResource.Get(), 0, (UINT)subresources.size());

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&wood.pUploader)));

    UpdateSubresources(pCommandList.Get(), wood.pResource.Get(), wood.pUploader.Get(), 0, 0, subresources.size(), &subresources[0]);

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        wood.pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    Textures[wood.Name] = wood;
}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = Textures.size();
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NodeMask = 0;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap)));

    ID3D12Resource *pTexRes = Textures["wood"].pResource.Get();
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = pTexRes->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = pTexRes->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    pD3dDevice->CreateShaderResourceView(pTexRes, &srvDesc, handle);
}

void D3DFrame::BuildGeometries()
{
    Geometry::Mesh Box = Geometry::GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f);

    std::vector<Vertex> vertices(Box.vertices.size());
    for (UINT i = 0; i < Box.vertices.size(); ++i)
        vertices[i] = {Box.vertices[i].vec3Position, Box.vertices[i].vec3Normal, Box.vertices[i].vec2TexCoords};

    UINT cbVertex = vertices.size() * sizeof(Vertex);
    UINT cbIndex = Box.indices.size() * sizeof(UINT16);

    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], cbVertex, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &Box.indices[0], cbIndex, &Geo.pUploaderIndexBuffer);

    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.nVertexBufferByteSize = cbVertex;
    Geo.nIndexBufferByteSize = cbIndex;

    Render::SubmeshGeometry submesh;
    submesh.nIndexCount = Box.indices.size();
    submesh.nBaseVertexLocation = 0;
    submesh.nStartIndexLocation = 0;

    Geo.DrawArgs["Box"] = submesh;
}

void D3DFrame::BuildRenderItems()
{
    Render::RenderItem box;
    Render::SubmeshGeometry submesh = Geo.DrawArgs["Box"];

    box.iFramesDirty = 3;
    box.pGeo = &Geo;
    box.pMaterial = &Materials["wood"];
    box.nCBObjectIndex = 0;
    box.nIndexCount = submesh.nIndexCount;
    box.nBaseVertexLocation = submesh.nBaseVertexLocation;
    box.nStartIndexLocation = submesh.nStartIndexLocation;
    box.matTexTransform = MathHelper::Identity4x4();
    box.matWorld = MathHelper::Identity4x4();

    RenderItems.push_back(box);
}

void D3DFrame::BuildMaterials()
{
    Light::Material wood;
    wood.iNumFramesDirty = 3;
    wood.Name = "wood";
    wood.nCBIndex = 0;
    wood.nDiffuseSrvHeapIndex = 0;
    wood.vec4DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    wood.vec3FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    wood.nRoughness = 0.2f;

    Materials[wood.Name] = wood;

    for (UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Render::FrameResource::CBTYPE_MATERIAL, 0, Materials.size());
}

void D3DFrame::BuildRootSignature()
{
    CD3DX12_STATIC_SAMPLER_DESC sampler;
    sampler.Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    CD3DX12_DESCRIPTOR_RANGE ranges[1];
    CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_ROOT_SIGNATURE_DESC desc;

    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    params[0].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_PIXEL);
    params[1].InitAsConstantBufferView(0); // Object;
    params[2].InitAsConstantBufferView(1); // Material;
    params[3].InitAsConstantBufferView(2); // Scene

    desc.Init(4, params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerializeRootSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeRootSignature, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeRootSignature->GetBufferPointer(), pSerializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
}

void D3DFrame::UpdateCamera(const GameTimer &t)
{
    // 本程序采用左手坐标系
    // 本程序的球坐标系是以 y 轴为极轴
    // fTheta θ: 球极向量与极轴的夹角
    // fPhi   Φ: 球极向量在 x-z 平面上的投影与 x 轴的夹角
    vec3EyePos.x = fRadius * sinf(fPhi) * cosf(fTheta);
    vec3EyePos.z = fRadius * sinf(fPhi) * sinf(fTheta);
    vec3EyePos.y = fRadius * cosf(fPhi);

    XMVECTOR vecEyePos = XMVectorSet(vec3EyePos.x, vec3EyePos.y, vec3EyePos.z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR target = XMVectorZero();

    XMMATRIX view = XMMatrixLookAtLH(vecEyePos, target, up);
    XMStoreFloat4x4(&matView, view);
}

void D3DFrame::UpdateObjects(const GameTimer &t)
{
    for (auto item : RenderItems)
    {
        if (item.iFramesDirty-- > 0)
        {
            Render::ObjectConstant oc;
            XMMATRIX matWorld = XMLoadFloat4x4(&item.matWorld);
            XMMATRIX matTexTransform = XMLoadFloat4x4(&item.matTexTransform);
            XMStoreFloat4x4(&oc.matWorld, XMMatrixTranspose(matWorld));
            XMStoreFloat4x4(&oc.matTexTransform, XMMatrixTranspose(matTexTransform));

            pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &oc, sizeof(oc));
        }
    }
}

void D3DFrame::UpdateMaterials(const GameTimer& t)
{
    for(auto item: Materials)
    {
        auto& mat = item.second;

        if(mat.iNumFramesDirty-- > 0)
        {
            Light::MaterialConstant mc;
            mc.matTransform = mat.matTransform;
            mc.nRoughness = mat.nRoughness;
            mc.vec3FresnelR0 = mat.vec3FresnelR0;
            mc.vec4DiffuseAlbedo = mat.vec4DiffuseAlbedo;

            pCurrFrameResource->CBMaterial.CopyData(mat.nCBIndex, &mc, sizeof(mc));
        }
    }
}

void D3DFrame::UpdateScene(const GameTimer &t)
{
    Render::SceneConstant sc;

    XMMATRIX view = XMLoadFloat4x4(&matView);
    XMMATRIX proj = XMLoadFloat4x4(&matProj);
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
    sc.vec3EyePos = vec3EyePos;
    sc.vec2RenderTargetSize = XMFLOAT2((float)cxClient, (float)cyClient);
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f / cxClient, 1.0f / cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();
    sc.vec4AmbientLight = {0.25f, 0.25f, 0.35f, 1.0f};
    sc.lights[0].vec3Direction = {0.57735f, -0.57735f, 0.57735f};
    sc.lights[0].vec3Strength = {0.6f, 0.6f, 0.6f};
    sc.lights[1].vec3Direction = {-0.57735f, -0.57735f, 0.57735f};
    sc.lights[1].vec3Strength = {0.3f, 0.3f, 0.3f};
    sc.lights[2].vec3Direction = {0.0f, -0.707f, -0.707f};
    sc.lights[2].vec3Strength = {0.15f, 0.15f, 0.15f};

    pCurrFrameResource->CBScene.CopyData(0, &sc, sizeof(Render::SceneConstant));
}

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&matProj, proj);
}

void D3DFrame::OnMouseDown(WPARAM btnState, int x, int y)
{
    lastPos.x = x;
    lastPos.y = y;

    SetCapture(hwnd);
}

void D3DFrame::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void D3DFrame::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * (x - lastPos.x));
        float dy = XMConvertToRadians(0.25f * (y - lastPos.y));

        // Update angles based on input to orbit camera around box.
        fTheta += dx;
        fPhi += dy;

        // Restrict the angle fPhi.
        fPhi<0.1f ? fPhi = 0.1f : fPhi> XM_PI - 0.1f ? fPhi = XM_PI - 0.1f : 0;
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f * static_cast<float>(x - lastPos.x);
        float dy = 0.05f * static_cast<float>(y - lastPos.y);

        // Update the camera radius based on input.
        fRadius += dx - dy;

        // Restrict the radius.
        fRadius<5.0 ? fRadius = 5.0 : fRadius> 150.0 ? fRadius = 150.0 : 0;
    }

    lastPos.x = x;
    lastPos.y = y;
}