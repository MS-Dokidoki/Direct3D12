#include "D3DApp.h"
#define USE_FRAMERESOURCE           // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;  
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    void OnResize();
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);
    void OnMouseMove(WPARAM, int, int);

    void BuildGeometry();
    void BuildRenderItems();
    void DrawItems();

    void UpdateCamera();
    void UpdateObjects();
    void UpdateScene(const GameTimer&);
private:
    ComPtr<ID3D12PipelineState> pPipelineState, pPipelineState_WareFrame;
    ComPtr<ID3D12RootSignature> pRootSignature;

    ComPtr<ID3D12DescriptorHeap> pCBVHeap;
    Render::MeshGeometry Geo;

    std::vector<Render::RenderItem> RenderItems; 
    UINT nCBObjByteSize;
    UINT nCBSceneByteSize;
    UINT nSceneCBVOffset;

    float mTheta = 1.5f*XM_PI;
    float mPhi = 0.2f*XM_PI;
    float mRadius = 15.0f;

    POINT mLastMousePos;
    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos;
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
    BuildGeometry();
    BuildRenderItems();
// Input layouts
    D3D12_INPUT_ELEMENT_DESC pInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
// Shaders
    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

// Constant Buffer
    UINT nObject = RenderItems.size();
    UINT nDescriptors = (nObject + 1) * sc_cFrameResource; 
    nSceneCBVOffset = nObject * sc_cFrameResource;
    nCBSceneByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Render::SceneConstant));
    nCBObjByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Render::ObjectConstant));

    for(UINT i = 0; i < sc_cFrameResource; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, 2);  // cylinder cube
    
    D3D12_DESCRIPTOR_HEAP_DESC descCBVHeap;
    descCBVHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descCBVHeap.NodeMask = 0;
    descCBVHeap.NumDescriptors = nDescriptors;
    descCBVHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&descCBVHeap, IID_PPV_ARGS(&pCBVHeap)));

    // Create CBVs
    for(UINT frameIndex = 0; frameIndex < sc_cFrameResource; ++frameIndex)
    {
        ID3D12Resource* pCBObj = pFrameResources[frameIndex].CBObject.Resource();
        ID3D12Resource* pCBScene = pFrameResources[frameIndex].CBScene.Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddr;
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
        D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
        int heapIndex;
        for(UINT i = 0; i < nObject; ++i)
        {
            cbAddr = pCBObj->GetGPUVirtualAddress();

            cbAddr += i * nCBObjByteSize;

            heapIndex = frameIndex * nObject + i;
            handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, nCSUDescriptorSize);

            CBVDesc.BufferLocation = cbAddr;
            CBVDesc.SizeInBytes = nCBObjByteSize;
            pD3dDevice->CreateConstantBufferView(&CBVDesc, handle);
        }
        cbAddr = pCBScene->GetGPUVirtualAddress();

        heapIndex = nSceneCBVOffset + frameIndex;
        handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCBVHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, nCSUDescriptorSize);

        CBVDesc.BufferLocation = cbAddr;
        CBVDesc.SizeInBytes = nCBSceneByteSize;
        pD3dDevice->CreateConstantBufferView(&CBVDesc, handle);
    }

// Execute Command List
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCmdLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);

// Root Signature
    CD3DX12_DESCRIPTOR_RANGE CBVTables[2];
    CD3DX12_ROOT_PARAMETER rootParams[2];
    CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc;

    CBVTables[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CBVTables[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    rootParams[0].InitAsDescriptorTable(1, &CBVTables[0]);
    rootParams[1].InitAsDescriptorTable(1, &CBVTables[1]);

    rootSignDesc.Init(2, rootParams, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerializeSign;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeSign, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeSign->GetBufferPointer(), pSerializeSign->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

// Pipeline State
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

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState)));
    descPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState_WareFrame)));

    FlushCommandQueue();

    return 1;
}

void D3DFrame::Update(const GameTimer& t)
{
#ifdef USE_FRAMERESOURCE
    // 在当前帧命令存放之前，先进行命令分配器的切换
    iCurrFrameResourceIndex = (iCurrFrameResourceIndex + 1) % sc_cFrameResource;
    pCurrFrameResource = &pFrameResources[iCurrFrameResourceIndex];
    pCommandAllocator = pCurrFrameResource->pCommandAllocator;

    // 若当前帧资源未执行完成，说明命令分配器全都被 GPU 使用，等待执行完成
    if(pCurrFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrFrameResource->iFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(pCurrFrameResource->iFence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }
#endif
    UpdateCamera();
    UpdateScene(t);
    UpdateObjects();
}

void D3DFrame::UpdateCamera()
{
    vec3EyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
    vec3EyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
    vec3EyePos.y = mRadius*cosf(mPhi);

    XMVECTOR pos = XMVectorSet(vec3EyePos.x, vec3EyePos.y, vec3EyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&matView, view);
}

void D3DFrame::UpdateObjects()
{
    for(auto& e: RenderItems)
    {
        if(e.iFramesDirty-- > 0)
        {
            XMMATRIX matWorld = XMLoadFloat4x4(&e.World);
            Render::ObjectConstant oc;
            XMStoreFloat4x4(&oc.matWorld, XMMatrixTranspose(matWorld));

            pCurrFrameResource->CBObject.CopyData(e.iCBObjectIndex, &oc, sizeof(oc));
        }
    }
}

void D3DFrame::UpdateScene(const GameTimer& t)
{
    Render::SceneConstant sc;
    XMMATRIX view = XMLoadFloat4x4(&matView);
    XMMATRIX proj = XMLoadFloat4x4(&matProj);

    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
    
    XMStoreFloat4x4(&sc.matView, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(invViewProj));
    sc.vec3EyePos = vec3EyePos;
    sc.vec2RenderTargetSize = XMFLOAT2(cxClient, cyClient);
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f / cxClient, 1.0f/ cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();

    pCurrFrameResource->CBScene.CopyData(0, &sc, sizeof(sc));
}

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&matProj, proj);
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
    pCommandList->RSSetScissorRects(1, &stScissorRect);
    pCommandList->RSSetViewports(1, &stScreenViewport);

    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::SteelBlue, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = {pCBVHeap.Get()};
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pCBVHeap->GetGPUDescriptorHandleForHeapStart());
    handle.Offset(nSceneCBVOffset + iCurrFrameResourceIndex, nCSUDescriptorSize);
    pCommandList->SetGraphicsRootDescriptorTable(1, handle);

    DrawItems();

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
    iCurrBackBuffer = (iCurrBackBuffer + 1) % sc_cSwapChainBuffer;
	
#ifdef USE_FRAMERESOURCE
    pCurrFrameResource->iFence = (UINT)++uiCurrentFence;
    pCommandQueue->Signal(pFence.Get(), uiCurrentFence);
#else
    FlushCommandQueue();
#endif
}

void D3DFrame::BuildGeometry()
{
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;
    Geometry::Mesh Cylinder = Geometry::GeometryGenerator::CreateCylinder(0.5f, 0.3, 5.0f, 20, 20);
    Geometry::Mesh Cube = Geometry::GeometryGenerator::CreateCube();

    UINT nCylinderVertexOffset = 0;
    UINT nCubeVertexOffset = Cylinder.vertices.size();

    UINT nCylinderIndexOffset = 0;
    UINT nCubeIndexOffset = Cylinder.indices.size();

    Render::SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.nIndexCount = Cylinder.indices.size();
    cylinderSubmesh.iStartIndexLocation = nCylinderIndexOffset;
    cylinderSubmesh.iBaseVertexLocation = nCylinderVertexOffset;

    Render::SubmeshGeometry cubeSubmesh;
    cubeSubmesh.nIndexCount = Cube.indices.size();
    cubeSubmesh.iStartIndexLocation = nCubeIndexOffset;
    cubeSubmesh.iBaseVertexLocation = nCubeVertexOffset;

    UINT nTotalVertexSize = Cylinder.vertices.size() + Cube.vertices.size();
    
    vertices.resize(nTotalVertexSize);
    UINT k = 0;
    for(UINT i = 0; i < Cylinder.vertices.size(); ++i, ++k)
        vertices[k] = {Cylinder.vertices[i].vec3Position, XMFLOAT4(DirectX::Colors::DarkGreen)};
    for(UINT i = 0; i < Cube.vertices.size(); ++i, ++k)
        vertices[k] = {Cube.vertices[i].vec3Position, XMFLOAT4(DirectX::Colors::ForestGreen)};

    indices.insert(indices.end(), std::begin(Cylinder.indices), std::end(Cylinder.indices));
    indices.insert(indices.end(), std::begin(Cube.indices), std::end(Cube.indices));

// Upload data
    UINT nVertexByteSize = vertices.size() * sizeof(Vertex);
    UINT nIndexByteSize = indices.size() * sizeof(UINT16);

    Geo.Name = "Geometries";

    ThrowIfFailed(D3DCreateBlob(nVertexByteSize, &Geo.pCPUVertexBuffer));
    CopyMemory(Geo.pCPUVertexBuffer->GetBufferPointer(), &vertices[0], nVertexByteSize);
    ThrowIfFailed(D3DCreateBlob(nIndexByteSize, &Geo.pCPUIndexBuffer));
    CopyMemory(Geo.pCPUIndexBuffer->GetBufferPointer(), &indices[0], nIndexByteSize);

    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], nVertexByteSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], nIndexByteSize, &Geo.pUploaderIndexBuffer);

    Geo.nIndexBufferByteSize = nIndexByteSize;
    Geo.nVertexBufferByteSize = nVertexByteSize;
    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.emIndexFormat = DXGI_FORMAT_R16_UINT;

    Geo.DrawArgs["Cylinder"] = cylinderSubmesh;
    Geo.DrawArgs["Cube"] = cubeSubmesh;
}

void D3DFrame::BuildRenderItems()
{
    Render::RenderItem cubeItem;
    XMStoreFloat4x4(&cubeItem.World, XMMatrixScaling(2.0f, 2.0f, 2.0f));
    cubeItem.iCBObjectIndex = 0;
    cubeItem.iFramesDirty = 3;
    cubeItem.pGeo = &Geo;

    Render::SubmeshGeometry cubeSubmesh = cubeItem.pGeo->DrawArgs["Cube"];
    cubeItem.cIndex = cubeSubmesh.nIndexCount;
    cubeItem.iStartIndexLocation = cubeSubmesh.iStartIndexLocation;
    cubeItem.iBaseVertexLocation = cubeSubmesh.iBaseVertexLocation;
    cubeItem.PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    RenderItems.push_back(cubeItem);

    Render::RenderItem cylinderItem;
    XMStoreFloat4x4(&cylinderItem.World, XMMatrixTranslation(0.0f, 5.0f, 0.0f));
    cylinderItem.iCBObjectIndex = 0;
    cylinderItem.iFramesDirty = 3;
    cylinderItem.pGeo = &Geo;

    Render::SubmeshGeometry cylinderSubmesh = cylinderItem.pGeo->DrawArgs["Cylinder"];
    cylinderItem.cIndex = cylinderSubmesh.nIndexCount;
    cylinderItem.iBaseVertexLocation = cylinderSubmesh.iBaseVertexLocation;
    cylinderItem.iStartIndexLocation = cylinderSubmesh.iStartIndexLocation;

    
    RenderItems.push_back(cylinderItem);
}

void D3DFrame::DrawItems()
{
    for(UINT i = 0; i < RenderItems.size(); ++i)
    {
        Render::RenderItem item = RenderItems[i];
        
        pCommandList->IASetPrimitiveTopology(item.PrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->VertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->IndexBufferView());

        UINT heapIndex = iCurrFrameResourceIndex * RenderItems.size() + item.iCBObjectIndex;
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pCBVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, nCSUDescriptorSize);

        pCommandList->SetGraphicsRootDescriptorTable(0, handle);
        pCommandList->DrawIndexedInstanced(item.cIndex, 1, item.iStartIndexLocation, item.iBaseVertexLocation, 0);   
    }
}

void D3DFrame::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

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
        float dx = XMConvertToRadians(0.25f*(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi < 0.1f? mPhi = 0.1f: mPhi > XM_PI - 0.1f? mPhi = XM_PI - 0.1f: 0;

    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius < 5.0? mRadius = 5.0: mRadius > 150.0? mRadius = 150.0: 0;
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

