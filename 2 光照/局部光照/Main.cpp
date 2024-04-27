#include "D3DApp.h"
#define USE_FRAMERESOURCE           // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void OnResize();
    virtual void OnMouseDown(WPARAM, int, int);
    virtual void OnMouseMove(WPARAM, int, int);
    virtual void OnMouseUp(WPARAM, int, int);
    
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);

    void BuildGeometries();
    void BuildRenderItems();
    void BuildMaterials();
    
    void UpdateCamera(const GameTimer&);
    void UpdateScene(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateMaterials(const GameTimer&);

private:
    ComPtr<ID3D12PipelineState> pPipelineState;
    ComPtr<ID3D12RootSignature> pRootSignature;

    Render::MeshGeometry Geo;
    std::vector<Render::RenderItem> RenderItems;
    std::unordered_map<std::string, Light::Material> Materials;

    float fTheta = 1.5f * XM_PI;
    float fPhi   = 0.2 * XM_PI;
    float fRadius = 15.0f;

    POINT lastPos;

    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos;

    const UINT nCBObjectByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Render::ObjectConstant));
    const UINT nCBMaterialByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Light::Material));

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    try
    {
        D3DFrame app(hInstance);

        if(!app.Initialize())
            return 0;
        return app.Run();
    }
    catch(DXException& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("HR Failed"), MB_OK);
    }

    return 0;
}

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{}

bool D3DFrame::Initialize()
{
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;     
#endif
    if(!D3DApp::Initialize())
        return 0;
    
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));
    BuildGeometries();
    BuildRenderItems();
    BuildMaterials();
    
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCmls[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCmls);

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, RenderItems.size());
    
    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

    D3D12_INPUT_ELEMENT_DESC pInputLayouts[] ={
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    CD3DX12_ROOT_PARAMETER params[3];
    CD3DX12_ROOT_SIGNATURE_DESC rsd;

    params[0].InitAsConstantBufferView(0); // Object
    params[1].InitAsConstantBufferView(1); // Material
    params[2].InitAsConstantBufferView(2); // Scene

    rsd.Init(3, params, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> pSerializeRootSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeRootSignature, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeRootSignature->GetBufferPointer(), pSerializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
    gpsd.InputLayout = {pInputLayouts, _countof(pInputLayouts)};
    gpsd.pRootSignature = pRootSignature.Get();
    gpsd.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    gpsd.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    gpsd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gpsd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    gpsd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gpsd.NumRenderTargets = 1;
    gpsd.RTVFormats[0] = emBackBufferFormat;
    gpsd.DSVFormat = emDepthStencilFormat;
    gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsd.SampleDesc = {1, 0};
    gpsd.SampleMask = UINT_MAX;
    
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&pPipelineState)));

    FlushCommandQueue();
    return 1;
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
    UpdateCamera(t);
    UpdateScene(t);
    UpdateObjects(t);
    UpdateMaterials(t);
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));
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

//
    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    pCommandList->SetGraphicsRootConstantBufferView(2, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    
    for(UINT i = 0; i < RenderItems.size(); ++i)
    {
        Render::RenderItem item = RenderItems[i];

        pCommandList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item.nCBObjectIndex * nCBObjectByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(0, addr);
        addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += item.pMaterial->nCBIndex * nCBMaterialByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(1, addr);

        pCommandList->DrawIndexedInstanced(item.nIndexCount, 1, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }
//

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

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&matProj, proj);
}

void D3DFrame::UpdateCamera(const GameTimer&)
{
    // 本程序采用左手坐标系
    // 本程序的球坐标系是以 y 轴为极轴
    // fTheta θ: 球极向量与极轴的夹角
    // fPhi   Φ: 球极向量在 x-z 平面上的投影与 x 轴的夹角
    vec3EyePos.x = fRadius * sinf(fPhi)*cosf(fTheta);
    vec3EyePos.z = fRadius * sinf(fPhi)*sinf(fTheta);
    vec3EyePos.y = fRadius * cosf(fPhi);;

    XMVECTOR vecEyePos = XMVectorSet(vec3EyePos.x, vec3EyePos.y, vec3EyePos.z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR target = XMVectorZero();

    XMMATRIX view = XMMatrixLookAtLH(vecEyePos, target, up);
    XMStoreFloat4x4(&matView, view);
}

void D3DFrame::UpdateObjects(const GameTimer&)
{
    for(auto item: RenderItems)
    {
        if(item.iFramesDirty-- > 0)
        {
            Render::ObjectConstant oc;
            XMMATRIX matWorld = XMLoadFloat4x4(&item.matWorld);
            XMStoreFloat4x4(&oc.matWorld, XMMatrixTranspose(matWorld));

            pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &oc, sizeof(oc));
        }
    }
}

void D3DFrame::UpdateMaterials(const GameTimer& t)
{
    for(auto item: Materials)
    {
        Light::Material& mat = item.second;
        if(mat.iNumFramesDirty-- > 0)
        {
            Light::MaterialConstant mc;
            mc.vec4DiffuseAlbedo = mat.vec4DiffuseAlbedo;
            mc.vec3FresnelR0 = mat.vec3FresnelR0;
            mc.nRoughness = mat.nRoughness;
            XMStoreFloat4x4(&mc.matTransform, XMMatrixIdentity());

            pCurrFrameResource->CBMaterial.CopyData(mat.nCBIndex, &mc, sizeof(mc));
        }
    }
}

void D3DFrame::UpdateScene(const GameTimer& t)
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
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f/ cxClient, 1.0f/ cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();
    
    sc.lights[0].vec3Direction = XMFLOAT3(sin(t.TotalTime()), cos(t.TotalTime()), -0.3f);
    sc.lights[0].vec3Strength = XMFLOAT3(1.0f, 1.0f, 0.9f);
	sc.vec4AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	
    pCurrFrameResource->CBScene.CopyData(0, &sc, sizeof(Render::SceneConstant));
}


void D3DFrame::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*(x - lastPos.x));
        float dy = XMConvertToRadians(0.25f*(y - lastPos.y));

        // Update angles based on input to orbit camera around box.
        fTheta += dx;
        fPhi += dy;

        // Restrict the angle fPhi.
        fPhi < 0.1f? fPhi = 0.1f: fPhi > XM_PI - 0.1f? fPhi = XM_PI - 0.1f: 0;

    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - lastPos.x);
        float dy = 0.05f*static_cast<float>(y - lastPos.y);

        // Update the camera radius based on input.
        fRadius += dx - dy;

        // Restrict the radius.
        fRadius < 5.0? fRadius = 5.0: fRadius > 150.0? fRadius = 150.0: 0;
    }

    lastPos.x = x;
    lastPos.y = y;
}

void D3DFrame::BuildGeometries()
{
    std::vector<Vertex> vertices;

    Geometry::Mesh cylinder = Geometry::GeometryGenerator::CreateCylinder(3, 3, 10, 20, 20);

    vertices.resize(cylinder.vertices.size());
    for(UINT i = 0; i < cylinder.vertices.size(); ++i)
        vertices[i] = {cylinder.vertices[i].vec3Position, cylinder.vertices[i].vec3Normal};
    
    UINT nVertexByteSize = vertices.size() * sizeof(Vertex);
    UINT nIndexByteSize = cylinder.indices.size() * sizeof(UINT16);

    Geo.Name = "geo";
    
    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], nVertexByteSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &cylinder.indices[0], nIndexByteSize, &Geo.pUploaderIndexBuffer);

    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.nVertexBufferByteSize = nVertexByteSize;
    Geo.nIndexBufferByteSize = nIndexByteSize;

    Render::SubmeshGeometry subMesh;
    subMesh.nIndexCount = cylinder.indices.size();
    subMesh.nBaseVertexLocation = 0;
    subMesh.nStartIndexLocation = 0;
    Geo.DrawArgs["Cylinder"] = subMesh;
}

void D3DFrame::BuildMaterials()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Render::FrameResource::CBTYPE_MATERIAL, 0, 1);
    
    Light::Material mat;
    mat.Name = "water";
    mat.iNumFramesDirty = 3;
    mat.nCBIndex = 0;
    mat.vec4DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    mat.vec3FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    mat.nRoughness = 0.0f;
    
    Materials["water"] = mat;
}

void D3DFrame::BuildRenderItems()
{
    Render::RenderItem item;
    item.iFramesDirty = 3;
    item.nCBObjectIndex = 0;
    item.pGeo = &Geo;

    Render::SubmeshGeometry subMesh = item.pGeo->DrawArgs["Cylinder"];
    item.nIndexCount = subMesh.nIndexCount;
    item.nBaseVertexLocation = subMesh.nBaseVertexLocation;
    item.nStartIndexLocation = subMesh.nStartIndexLocation;
    item.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    item.pMaterial = &Materials["water"];

    XMStoreFloat4x4(&item.matWorld, XMMatrixIdentity());
    RenderItems.push_back(item);
}