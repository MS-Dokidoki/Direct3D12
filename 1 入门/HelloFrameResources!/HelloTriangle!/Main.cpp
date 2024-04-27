#include "D3DApp.h"
#define USE_FRAMERESOURCE
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

private:
   ComPtr<ID3D12PipelineState> pPipelineState;
   ComPtr<ID3D12RootSignature> pRootSignature;

    ComPtr<ID3D12DescriptorHeap> pCBVHeap;
    Render::MeshGeometry Quad;

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
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;     
#endif
    if(!D3DApp::Initialize())
        return 0;
     Vertex vertices[] ={
        { { 0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.5f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
		{ { 0.5f, 0.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f} },
        { { 0.5f, 0.5f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	};

    UINT16 indices[] = {
        0, 1, 2,
        1, 3, 2   
    };
    UINT nVertexByteSize = sizeof(vertices);
    UINT nIndexByteSize = sizeof(indices);

    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    ThrowIfFailed(D3DCreateBlob(nVertexByteSize, &Quad.pCPUVertexBuffer));
    CopyMemory(Quad.pCPUVertexBuffer->GetBufferPointer(), vertices, nVertexByteSize);

    ThrowIfFailed(D3DCreateBlob(nVertexByteSize, &Quad.pCPUIndexBuffer));
    CopyMemory(Quad.pCPUIndexBuffer->GetBufferPointer(), indices, nIndexByteSize);

    Quad.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), vertices, nVertexByteSize, &Quad.pUploaderVertexBuffer);
    Quad.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), indices, nIndexByteSize, &Quad.pUploaderIndexBuffer);

    Quad.nVertexBufferByteSize = nVertexByteSize;
    Quad.nIndexBufferByteSize = nIndexByteSize;
    Quad.nVertexByteStride = sizeof(Vertex);

/*
    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pVertexBuffer)
    ));

    void* pBegin;
    ThrowIfFailed(pVertexBuffer->Map(0, NULL, &pBegin));
    memcpy(pBegin, vertices, sizeof(vertices));
    pVertexBuffer->Unmap(0, NULL);

    viewVertexBuffer.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    viewVertexBuffer.SizeInBytes = sizeof(vertices);
    viewVertexBuffer.StrideInBytes = sizeof(Vertex);

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pIndexBuffer)
    ));

    ThrowIfFailed(pIndexBuffer->Map(0, NULL, &pBegin));
    memcpy(pBegin, indices, sizeof(indices));
    pIndexBuffer->Unmap(0, NULL);

    viewIndexBuffer.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
    viewIndexBuffer.Format = DXGI_FORMAT_R16_UINT;
    viewIndexBuffer.SizeInBytes = sizeof(indices);

*/
    D3D12_INPUT_ELEMENT_DESC descInputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

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
    ), &pError);

    ThrowIfFailed(pD3dDevice->CreateRootSignature(
        0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)
    ));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipeline = {};
    descPipeline.InputLayout = {descInputLayouts, _countof(descInputLayouts)};
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

    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(
        &descPipeline, IID_PPV_ARGS(&pPipelineState)
    ));

    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

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

    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &Quad.VertexBufferView());
    pCommandList->IASetIndexBuffer(&Quad.IndexBufferView());

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
	
    ThrowIfFailed(pSwapChain->Present(0, 0));
    iCurrBackBuffer = (iCurrBackBuffer + 1) % sc_cSwapChainBuffer;
	
#ifdef USE_FRAMERESOURCE
    pCurrFrameResource->iFence = (UINT)++uiCurrentFence;
    pCommandQueue->Signal(pFence.Get(), uiCurrentFence);
#else
    FlushCommandQueue();
#endif
}
