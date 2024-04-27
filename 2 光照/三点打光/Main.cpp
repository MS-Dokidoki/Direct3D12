#include "D3DApp.h"
#define USE_FRAMERESOURCE           // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Normal;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    virtual void OnResize();
    void OnMouseMove(WPARAM, int, int);
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);
    
    void BuildGeometries();
    void BuildRenderItems();

    void InputProcess(const GameTimer&);
    void UpdateCamera(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateScene(const GameTimer&);

private:
    ComPtr<ID3D12PipelineState> pPipelineState_Normal, pPipelineState_WireFrame;
    ComPtr<ID3D12RootSignature> pRootSignature;

    std::vector<Render::RenderItem> RenderItems;
    Render::MeshGeometry Geo;

    float fTheta = 1.5f * XM_PI;
    float fPhi   = XM_PIDIV2 - 0.1f;
    float fRadius = 50.0f;
    POINT lastPos;

    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos;
    bool bUseWireFrame = 0;

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
{

}

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

    D3D12_INPUT_ELEMENT_DESC pInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCml[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCml);
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, RenderItems.size());
    
    CD3DX12_ROOT_PARAMETER pSlotParams[3];
    CD3DX12_ROOT_SIGNATURE_DESC descRS;

    pSlotParams[0].InitAsConstantBufferView(0);
    pSlotParams[1].InitAsConstantBufferView(1);
    descRS.Init(2, pSlotParams, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerializeSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&descRS, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeSignature, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeSignature->GetBufferPointer(), pSerializeSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipeline;
    ZeroMemory(&descPipeline, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    descPipeline.InputLayout = {pInputElements, _countof(pInputElements)};
    descPipeline.pRootSignature = pRootSignature.Get();
    descPipeline.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    descPipeline.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    descPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    descPipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    descPipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    descPipeline.SampleMask = UINT_MAX;
    descPipeline.SampleDesc.Count = 1;
    descPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPipeline.NumRenderTargets = 1;
    descPipeline.RTVFormats[0] = emBackBufferFormat;
    descPipeline.DSVFormat = emDepthStencilFormat;

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState_Normal)));

    descPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState_WireFrame)));

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
    InputProcess(t);
    
    UpdateCamera(t);
    UpdateScene(t);
    UpdateObjects(t);
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), bUseWireFrame? pPipelineState_WireFrame.Get(): pPipelineState_Normal.Get()));
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
    pCommandList->SetGraphicsRootConstantBufferView(1, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    
    for(UINT i = 0; i < RenderItems.size(); ++i)
    {
        Render::RenderItem item = RenderItems[i];

        pCommandList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += CONSTANT_VALUE::nCBObjectByteSize * item.nCBObjectIndex;
        pCommandList->SetGraphicsRootConstantBufferView(0, addr);

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

void D3DFrame::InputProcess(const GameTimer& t)
{
    if(GetAsyncKeyState('1') & 0x8000)
    {
        bUseWireFrame = 1;
        Sleep(300);
    } 
}

void D3DFrame::UpdateCamera(const GameTimer& t)
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

void D3DFrame::UpdateObjects(const GameTimer& t)
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
    sc.vec2RenderTargetSize = XMFLOAT2(cxClient, cyClient);
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f/ cxClient, 1.0f/ cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();

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
    Geometry::Mesh Box = Geometry::GeometryGenerator::CreateBox(1.5f, 0.5f, 1.5f);
    Geometry::Mesh Grid = Geometry::GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
    Geometry::Mesh Sphere = Geometry::GeometryGenerator::CreateSphere(0.5f, 20, 20);
    Geometry::Mesh Cylinder = Geometry::GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    UINT nVertexOff_Box = 0;
    UINT nVertexOff_Grid = Box.vertices.size();
    UINT nVertexOff_Sphere = nVertexOff_Grid + Grid.vertices.size();
    UINT nVertexOff_Cylinder = nVertexOff_Sphere + Sphere.vertices.size();

    UINT nIndexOff_Box = 0;
    UINT nIndexOff_Grid = Box.indices.size();
    UINT nIndexOff_Sphere = nIndexOff_Grid + Grid.indices.size();
    UINT nIndexOff_Cylinder = nIndexOff_Sphere + Sphere.indices.size();

    Render::SubmeshGeometry SM_Box;
    SM_Box.nIndexCount = Box.indices.size();
    SM_Box.nStartIndexLocation = nIndexOff_Box;
    SM_Box.nBaseVertexLocation = nVertexOff_Box;

    Render::SubmeshGeometry SM_Grid;
    SM_Grid.nIndexCount = Grid.indices.size();
    SM_Grid.nStartIndexLocation = nIndexOff_Grid;
    SM_Grid.nBaseVertexLocation = nVertexOff_Grid;
    
    Render::SubmeshGeometry SM_Sphere;
    SM_Sphere.nIndexCount = Sphere.indices.size();
    SM_Sphere.nStartIndexLocation = nIndexOff_Sphere;
    SM_Sphere.nBaseVertexLocation = nVertexOff_Sphere;
    
    Render::SubmeshGeometry SM_Cylinder;
    SM_Cylinder.nIndexCount = Cylinder.indices.size();
    SM_Cylinder.nStartIndexLocation = nIndexOff_Cylinder;
    SM_Cylinder.nBaseVertexLocation = nVertexOff_Cylinder;

    UINT nVertexCount = Box.vertices.size() + Cylinder.vertices.size() + Sphere.vertices.size() + Grid.vertices.size();
    std::vector<Vertex> vertices(nVertexCount);

    UINT k = 0;
    for(UINT i = 0; i < Box.vertices.size(); ++i, ++k)
        vertices[k] = {Box.vertices[i].vec3Position, XMFLOAT4(Colors::ForestGreen)};
    
    for(UINT i = 0; i < Grid.vertices.size(); ++i, ++k)
        vertices[k] = {Grid.vertices[i].vec3Position, XMFLOAT4(DirectX::Colors::DarkGreen)};
    
    for(UINT i = 0; i < Sphere.vertices.size(); ++i, ++k)
        vertices[k] = {Sphere.vertices[i].vec3Position, XMFLOAT4(DirectX::Colors::Crimson)};
    
    for(UINT i = 0; i < Cylinder.vertices.size(); ++i, ++k)
        vertices[k] = {Cylinder.vertices[i].vec3Position, XMFLOAT4(DirectX::Colors::SteelBlue)};
    
    std::vector<UINT16> indices;
    indices.insert(indices.end(), std::begin(Box.indices), std::end(Box.indices));
    indices.insert(indices.end(), std::begin(Grid.indices), std::end(Grid.indices));
    indices.insert(indices.end(), std::begin(Sphere.indices), std::end(Sphere.indices));
    indices.insert(indices.end(), std::begin(Cylinder.indices), std::end(Cylinder.indices));

    const UINT nIndexByteSize = indices.size() * sizeof(UINT16);
    const UINT nVertexByteSize = vertices.size() * sizeof(Vertex);

    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], nVertexByteSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], nIndexByteSize, &Geo.pUploaderIndexBuffer);

    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.nVertexBufferByteSize = nVertexByteSize;
    Geo.nIndexBufferByteSize = nIndexByteSize;
    
    Geo.DrawArgs["Box"] = SM_Box;
    Geo.DrawArgs["Cylinder"] = SM_Cylinder;
    Geo.DrawArgs["Grid"] = SM_Grid;
    Geo.DrawArgs["Sphere"] = SM_Sphere;
}

void D3DFrame::BuildRenderItems()
{
    Render::SubmeshGeometry SM;
    SM = Geo.DrawArgs["Box"];

    Render::RenderItem box;
    box.iFramesDirty = 3;
    box.nCBObjectIndex = 0;
    box.pGeo = &Geo;
    box.nIndexCount = SM.nIndexCount;
    box.nStartIndexLocation = SM.nStartIndexLocation;
    box.nBaseVertexLocation = SM.nBaseVertexLocation;
    XMStoreFloat4x4(&box.matWorld, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));

    RenderItems.push_back(box);

    SM = Geo.DrawArgs["Grid"];
    Render::RenderItem grid;
    grid.iFramesDirty = 3;
    grid.nCBObjectIndex = 1;
    grid.pGeo = &Geo;
    grid.nIndexCount = SM.nIndexCount;
    grid.nStartIndexLocation = SM.nStartIndexLocation;
    grid.nBaseVertexLocation = SM.nBaseVertexLocation;
    grid.matWorld = MathHelper::Identity4x4();

    RenderItems.push_back(grid);
    UINT nCBIndex = 2;
    for(UINT i = 0; i < 5; ++i)
    {
        Render::RenderItem LCyl, RCyl, LSph, RSph;

        XMMATRIX matLCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
        XMMATRIX matRCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);
        XMMATRIX matLSphWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
        XMMATRIX matRSphWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);

        SM = Geo.DrawArgs["Cylinder"];
        LCyl.iFramesDirty = 3;
        LCyl.nCBObjectIndex = nCBIndex++;
        LCyl.pGeo = &Geo;
        LCyl.nIndexCount = SM.nIndexCount;
        LCyl.nStartIndexLocation = SM.nStartIndexLocation;
        LCyl.nBaseVertexLocation = SM.nBaseVertexLocation;
        XMStoreFloat4x4(&LCyl.matWorld, matLCylWorld);

        RCyl.iFramesDirty = 3;
        RCyl.nCBObjectIndex = nCBIndex++;
        RCyl.pGeo = &Geo;
        RCyl.nIndexCount = SM.nIndexCount;
        RCyl.nStartIndexLocation = SM.nStartIndexLocation;
        RCyl.nBaseVertexLocation = SM.nBaseVertexLocation;
        XMStoreFloat4x4(&RCyl.matWorld, matRCylWorld);


        SM = Geo.DrawArgs["Sphere"];
        LSph.iFramesDirty = 3;
        LSph.nCBObjectIndex = nCBIndex++;
        LSph.pGeo = &Geo;
        LSph.nIndexCount = SM.nIndexCount;
        LSph.nStartIndexLocation = SM.nStartIndexLocation;
        LSph.nBaseVertexLocation = SM.nBaseVertexLocation;
        XMStoreFloat4x4(&LSph.matWorld, matLSphWorld);

        RSph.iFramesDirty = 3;
        RSph.nCBObjectIndex = nCBIndex++;
        RSph.pGeo = &Geo;
        RSph.nIndexCount = SM.nIndexCount;
        RSph.nStartIndexLocation = SM.nStartIndexLocation;
        RSph.nBaseVertexLocation = SM.nBaseVertexLocation;
        XMStoreFloat4x4(&RSph.matWorld, matRSphWorld);
        
        RenderItems.push_back(LCyl);
        RenderItems.push_back(RCyl);
        RenderItems.push_back(LSph);
        RenderItems.push_back(RSph);

    }
}