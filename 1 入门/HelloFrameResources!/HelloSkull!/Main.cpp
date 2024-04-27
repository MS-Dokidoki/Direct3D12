#include "D3DApp.h"
#include <fstream>
#include <string>
#include <sstream>
#define USE_FRAMERESOURCE           // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

union SkullModelVertex
{
    float v[6];
    struct
    {
        float Position[3];
        float Normal[3];
    };
};

union SkullModelIndex
{
    UINT16 i[3];
    struct
    {
        UINT16 first;
        UINT16 second;
        UINT16 thrid;
    };
};

struct Vertex
{
    XMFLOAT3 Position;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void OnResize();
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    void OnMouseMove(WPARAM, int, int);
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);

    void BuildVertices();
    void BuildRenderItems();

    void UpdateCamera();
    void UpdateObjects();
    void UpdateScene(const GameTimer&);

private:
    ComPtr<ID3D12PipelineState> pPipelineState_WareFrame, pPipelineState;
    ComPtr<ID3D12RootSignature> pRootSignature;

    Render::MeshGeometry Geo;
    std::vector<Render::RenderItem> RenderItems; 
    float fTheta = 1.5f * XM_PI;
    float fPhi   = 0.2 * XM_PI;
    float fRadius = 15.0f;

    POINT lastPos;

    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos;

    UINT nCBObjByteSize = D3DHelper_CalcConstantBufferBytesSize(sizeof(Render::ObjectConstant));
	bool bUseWareFrame = 0;
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
    BuildVertices();
    BuildRenderItems();
/**** */
    D3D12_INPUT_ELEMENT_DESC pInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");
/**** */
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCml[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCml);
    
/**** */
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, 1);

    CD3DX12_ROOT_PARAMETER pSlotParams[2];
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

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState)));

    descPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&descPipeline, IID_PPV_ARGS(&pPipelineState_WareFrame)));

    FlushCommandQueue();
    Geo.DisposeUploader();
    return 1;
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
	if(GetAsyncKeyState('1') >> 4)
    {
		bUseWareFrame = !bUseWareFrame;
        Sleep(100);
    }

    UpdateCamera();
    UpdateObjects();
    UpdateScene(t);
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), bUseWareFrame? pPipelineState_WareFrame.Get(): pPipelineState.Get()));
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
        addr += item.nCBObjectIndex * nCBObjByteSize;

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

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&matProj, proj);
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
    if((hFile = CreateFileA("C:\\Users\\Administrator.PC-20191006TRUC\\source\\repos\\DirectX\\Models\\skull.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
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

void D3DFrame::BuildVertices()
{
    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> indices;
    std::vector<Vertex> vertices;

    LoadSkullModel(skullVertices, indices);

    vertices.resize(skullVertices.size());
    for(UINT i = 0; i < skullVertices.size(); ++i)
        vertices[i].Position = XMFLOAT3(skullVertices[i].Position);
    
    UINT nIndexByteSize = indices.size() * sizeof(SkullModelIndex);
    UINT nVertexByteSize = vertices.size() * sizeof(Vertex);

    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], nVertexByteSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], nIndexByteSize, &Geo.pUploaderIndexBuffer);

    Geo.nIndexBufferByteSize = nIndexByteSize;
    Geo.nVertexBufferByteSize = nVertexByteSize;
    Geo.nVertexByteStride = sizeof(Vertex);

    Render::SubmeshGeometry def;
    def.nIndexCount = indices.size() * 3;
    def.nBaseVertexLocation = 0;
    def.nStartIndexLocation = 0;

    Geo.DrawArgs["Skull"] = def;
}

void D3DFrame::BuildRenderItems()
{
    Render::RenderItem skull;
    skull.iFramesDirty = 3;
    skull.nCBObjectIndex = 0;
    skull.pGeo = &Geo;

    Render::SubmeshGeometry subMeshSkull = skull.pGeo->DrawArgs["Skull"];
    skull.nIndexCount = subMeshSkull.nIndexCount;
    skull.nStartIndexLocation = subMeshSkull.nStartIndexLocation;
    skull.nBaseVertexLocation = subMeshSkull.nBaseVertexLocation;
    skull.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    XMStoreFloat4x4(&skull.matWorld, XMMatrixIdentity());

    RenderItems.push_back(skull);
}

void D3DFrame::UpdateCamera()
{
    // 本程序采用左手坐标系
    // 本程序的球坐标系是以 y 轴为极轴
    // fTheta θ: 球极向量与极轴的夹角
    // fPhi   Φ: 球极向量在 x-z 平面上的投影与 x 轴的夹角
    vec3EyePos.x = fRadius * sinf(fPhi)*cosf(fTheta);
    vec3EyePos.y = fRadius * sinf(fPhi)*sinf(fTheta);
    vec3EyePos.z = fRadius * cosf(fPhi);;

    XMVECTOR vecEyePos = XMVectorSet(vec3EyePos.x, vec3EyePos.y, vec3EyePos.z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR target = XMVectorZero();

    XMMATRIX view = XMMatrixLookAtLH(vecEyePos, target, up);
    XMStoreFloat4x4(&matView, view);
}

void D3DFrame::UpdateObjects()
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