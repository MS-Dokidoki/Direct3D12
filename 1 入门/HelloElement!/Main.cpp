#include "D3DApp.h"

using namespace Microsoft::WRL;
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


    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12PipelineState> pPipelineState;

    ComPtr<ID3D12Resource> pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW viewVertexBuffer;
    ComPtr<ID3D12Resource> pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW viewIndexBuffer;

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


// Create Vertex Buffer and describe input layout
    Vertex triangles[] =
	{
/*
        { { 0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -0.5f, 0.25f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f} },
        { { -0.25, 0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
        { { 0.25, 0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
        { { 0.5f, 0.25f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.25f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f} },
        { { 0.25, -0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
        { { -0.25, -0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
        { { -0.5f, -0.25f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
*/		
        { { 0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
		{ { 0.5f, 0.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f} },
        { { 0.5f, 0.5f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	};

    UINT16 indices[] = {
    /*
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 5,
        0, 5, 6,
        0, 6, 7,
        0, 7, 8,
        0, 8, 1
    */
    // 顶点要按顺时针定义
    // 注意面剔除!!!!
        0, 1, 2,
        1, 3, 2   
    };

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(triangles)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pVertexBuffer)
    ));

    void* pBegin;
    ThrowIfFailedEx(pVertexBuffer->Map(0, NULL, &pBegin));
    memcpy(pBegin, triangles, sizeof(triangles));
    pVertexBuffer->Unmap(0, NULL);

    viewVertexBuffer.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    viewVertexBuffer.SizeInBytes = sizeof(triangles);
    viewVertexBuffer.StrideInBytes = sizeof(Vertex);

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pIndexBuffer)
    ));

    ThrowIfFailedEx(pIndexBuffer->Map(0, NULL, &pBegin));
    memcpy(pBegin, indices, sizeof(indices));
    pIndexBuffer->Unmap(0, NULL);

    viewIndexBuffer.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
    viewIndexBuffer.Format = DXGI_FORMAT_R16_UINT;
    viewIndexBuffer.SizeInBytes = sizeof(indices);

    D3D12_INPUT_ELEMENT_DESC descInputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

// Load Shader
    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;
#ifdef _DEBUG
    UINT uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT uiCompileFlags = 0;
#endif

    ThrowIfFailedEx(D3DCompileFromFile(
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/入门/HelloElement!/build/Debug/shader.hlsl", NULL, NULL, "VsMain", "vs_5_0", uiCompileFlags, 0, &pVsByteCode, NULL
    ));

    ThrowIfFailedEx(D3DCompileFromFile(
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/入门/HelloElement!/build/Debug/shader.hlsl", NULL, NULL, "PsMain", "ps_5_0", uiCompileFlags, 0, &pPsByteCode, NULL
    ));

// Create root signature
    D3D12_ROOT_SIGNATURE_DESC descRS;
    descRS.NumParameters = 0;
    descRS.pParameters = NULL;
    descRS.NumStaticSamplers = 0;
    descRS.pStaticSamplers = NULL;
    descRS.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> pSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(
        &descRS, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError
    ));

    ThrowIfFailedEx(pD3dDevice->CreateRootSignature(
        0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)
    ));

// Create Graphics Pipeline State
    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipeline = {};
    descPipeline.InputLayout = {descInputLayouts, _countof(descInputLayouts)};
    descPipeline.pRootSignature = pRootSignature.Get();
    descPipeline.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    descPipeline.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    descPipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    descPipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    descPipeline.DepthStencilState.DepthEnable = 0;
    descPipeline.DepthStencilState.StencilEnable = 0;
    descPipeline.SampleMask = UINT_MAX;
    descPipeline.SampleDesc.Count = 1;
    descPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPipeline.NumRenderTargets = 1;
    descPipeline.RTVFormats[0] = emBackBufferFormat;

    ThrowIfFailedEx(pD3dDevice->CreateGraphicsPipelineState(
        &descPipeline, IID_PPV_ARGS(&pPipelineState)
    ));

    return 1;
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailedEx(pCommandAllocator->Reset());
    ThrowIfFailedEx(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));
    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    pCommandList->RSSetViewports(1, &stScreenViewport);
    pCommandList->RSSetScissorRects(1, &stScissorRect);

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 0, NULL);
    float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, NULL);

    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &viewVertexBuffer);
    pCommandList->IASetIndexBuffer(&viewIndexBuffer);
    pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    pCommandList->Close();
	
	ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	
    pSwapChain->Present(1, 0);
    iCurrBackBuffer = (iCurrBackBuffer + 1) % sc_iSwapChainBufferCount;
    FlushCommandQueue();
}

void D3DFrame::Update(const GameTimer& t)
{

}