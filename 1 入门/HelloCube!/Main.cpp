#include "D3DApp.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct Scene
{
    XMFLOAT4X4 WorldViewProj;
};
class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);
    virtual void OnMouseMove(WPARAM btnState, int x, int y);

    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12PipelineState> pPipelineState;

    ComPtr<ID3D12DescriptorHeap> pCBVHeap;
    D3DHelper::UploadBuffer pConstantBuffer;
    D3DHelper::MeshGeometry Cube;
    
    float fTheta = 1.5f*XM_PI;
    float fPhi = XM_PIDIV4;
    float fRadius = 5.0f;

    POINT lastMousePoint;
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
    if(!D3DApp::Initialize())
        return 0;

// Cube
    Vertex vertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)  },
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)  },
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)    },
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)  },
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)   },
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)   },
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)}
    };

    UINT16 indices[] = {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    UINT uiVertexByteSize = sizeof(vertices);
    UINT uiIndexByteSize = sizeof(indices);

    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));
    Cube.pGPUVertexBuffer = D3DHelper::CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), vertices, uiVertexByteSize, &Cube.pUploaderVertexBuffer);
    Cube.pGPUIndexBuffer = D3DHelper::CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), indices, uiIndexByteSize, &Cube.pUploaderIndexBuffer);

    Cube.uiIndexBufferByteSize = uiIndexByteSize;
    Cube.uiVertexBufferByteSize = uiVertexByteSize;
    Cube.uiVertexByteStride = sizeof(Vertex);

    D3DHelper::SubmeshGeometry submesh;
    submesh.iBaseVertexLocation = 0;
    submesh.uiStartIndexLocation = 0;
    submesh.uiIndexCount = 36;
    Cube.DrawArgs["Cube"] = submesh;

    D3D12_INPUT_ELEMENT_DESC pInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = D3DHelper::CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = D3DHelper::CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

// Constant Buffer
    D3D12_DESCRIPTOR_HEAP_DESC descCBVHeap;
    descCBVHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descCBVHeap.NodeMask = 0;
    descCBVHeap.NumDescriptors = 1;
    descCBVHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&descCBVHeap, IID_PPV_ARGS(&pCBVHeap)));

    pConstantBuffer.Init(pD3dDevice.Get(), D3DHelper_CalcConstantBufferBytesSize(sizeof(Scene)), 1);

    D3D12_CONSTANT_BUFFER_VIEW_DESC descCBV;
    descCBV.BufferLocation = pConstantBuffer.Resource()->GetGPUVirtualAddress();
    descCBV.SizeInBytes = D3DHelper_CalcConstantBufferBytesSize(sizeof(Scene));

    pD3dDevice->CreateConstantBufferView(&descCBV, pCBVHeap->GetCPUDescriptorHandleForHeapStart());
// Root signature
    CD3DX12_ROOT_PARAMETER rootParam;
    CD3DX12_DESCRIPTOR_RANGE CBVTable;
    CD3DX12_ROOT_SIGNATURE_DESC descRootSign;
    CBVTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    rootParam.InitAsDescriptorTable(1, &CBVTable);
    descRootSign.Init(1, &rootParam, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pRootSign;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&descRootSign, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSign, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pRootSign->GetBufferPointer(), pRootSign->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));

// Pipeline
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
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCommandLists);

    FlushCommandQueue();
    return 1;
}

void D3DFrame::OnMouseDown(WPARAM btnState, int x, int y)
{
    lastMousePoint.x = x;
    lastMousePoint.y = y;
    SetCapture(hwnd);
}

void D3DFrame::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void D3DFrame::OnMouseMove(WPARAM btnState, int x, int y)
{
    if(btnState & MK_LBUTTON)
    {
        float fRadiansX = XMConvertToRadians(0.25f * (float)(x - lastMousePoint.x));
        float fRadiansY = XMConvertToRadians(0.25f * (float)(y - lastMousePoint.y));

        fTheta += fRadiansX;
        fPhi += fRadiansY;

        fPhi < 0.1f? fPhi = 0.1f: 0;
        fPhi > 180.0f - 0.1f? fPhi = 180.0f - 0.1f: 0;
    }
    else if(btnState & MK_RBUTTON)
    {
        float cx = 0.005f * (float)(x - lastMousePoint.x);
        float cy = 0.005f * (float)(y - lastMousePoint.y);

        fRadius += cx - cy;

        fRadius < 3.0f? fRadius = 3.0f: 0;
        fRadius > 15.0f? fRadius = 15.0f: 0;
    }

    lastMousePoint.x = x;
    lastMousePoint.y = y;
}

void D3DFrame::Update(const GameTimer& t)
{
    float x = fRadius*sinf(fPhi)*cosf(fTheta);
    float z = fRadius*sinf(fPhi)*sinf(fTheta);
    float y = fRadius*cosf(fPhi);

    XMMATRIX world, view, proj;
    
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    world = XMMatrixIdentity();
    view = XMMatrixLookAtLH(pos, target, up); 
    proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 1.0f, 1000.0f);
    XMMATRIX worldViewProj = world * view * proj;

    Scene s;
    XMStoreFloat4x4(&s.WorldViewProj, XMMatrixTranspose(worldViewProj));
    pConstantBuffer.CopyData(0, &s.WorldViewProj, sizeof(Scene));
    
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

    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0.0f, 0, NULL);

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 1, &DepthStencilBufferView());
    pCommandList->RSSetScissorRects(1, &stScissorRect);
    pCommandList->RSSetViewports(1, &stScreenViewport);

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = {pCBVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, ppHeaps);
    pCommandList->SetGraphicsRootDescriptorTable(0, pCBVHeap->GetGPUDescriptorHandleForHeapStart());

    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &Cube.VertexBufferView());
    pCommandList->IASetIndexBuffer(&Cube.IndexBufferView());
    pCommandList->DrawIndexedInstanced(Cube.DrawArgs["Cube"].uiIndexCount, 1, 0, 0, 0);

    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    pCommandList->Close();
	
	ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	
    pSwapChain->Present(0, 0);
    iCurrBackBuffer = (iCurrBackBuffer + 1) % sc_iSwapChainBufferCount;
    FlushCommandQueue();
}
