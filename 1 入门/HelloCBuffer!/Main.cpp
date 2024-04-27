#include "D3DApp.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT4 Color;
};

struct Transform
{
    XMFLOAT4X4 Model;
    XMFLOAT4X4 Projection;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();
protected:
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);

    void CreateMatrix(Transform*, const GameTimer&);
    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12PipelineState> pPipelineState;

    ComPtr<ID3D12Resource> pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW viewVertexBuffer;
    ComPtr<ID3D12Resource> pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW viewIndexBuffer;
    ComPtr<ID3D12Resource> pVertexUploader;
    ComPtr<ID3D12Resource> pIndexUploader;

    ComPtr<ID3D12DescriptorHeap> pCBVHeap;
    ComPtr<ID3D12Resource> pConstantBuffer;
    void* pConstantBegin;


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

void D3DFrame::CreateMatrix(Transform* tf, const GameTimer& t)
{
    XMMATRIX model, proj;
    model = DirectX::XMMatrixRotationZ(t.TotalTime());
    model *= DirectX::XMMatrixScaling(0.5f, 0.5f, 1.0f);
    
    proj = DirectX::XMMatrixIdentity();

    DirectX::XMStoreFloat4x4(&tf->Model, model);
    DirectX::XMStoreFloat4x4(&tf->Projection, proj);
}
bool D3DFrame::Initialize()
{
    if(!D3DApp::Initialize())
        return 0;

    ThrowIfFailedEx(pCommandAllocator->Reset());
    ThrowIfFailedEx(pCommandList->Reset(pCommandAllocator.Get(), NULL));
// Create Vertex Buffer and describe input layout
    Vertex triangles[] =
	{

        { { 0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f},   { 0.0f, 0.0f, 1.0f, 1.0f}},
		{ { 1.0f, 0.0f, 0.0f }, {  0.0f, 1.0f, 0.0f, 1.0f} },
        { { 1.0f, 1.0f, 0.0f}, { 1.0f, 0.0f, 0.0f, 1.0f } },
	};

    UINT16 indices[] = {
    // 顶点要按顺时针定义
    // 注意面剔除!!!!
        0, 1, 2,
        1, 3, 2   
    };

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(triangles)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&pVertexBuffer)
    ));

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(triangles)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pVertexUploader)
    ));

    void* pBegin;
    ThrowIfFailedEx(pVertexUploader->Map(0, NULL, &pBegin));
    memcpy(pBegin, triangles, sizeof(triangles));
    pVertexUploader->Unmap(0, NULL);

    pCommandList->CopyResource(pVertexBuffer.Get(), pVertexUploader.Get());
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pVertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    viewVertexBuffer.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    viewVertexBuffer.SizeInBytes = sizeof(triangles);
    viewVertexBuffer.StrideInBytes = sizeof(Vertex);

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&pIndexBuffer)
    ));

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pIndexUploader)
    ));

    ThrowIfFailedEx(pIndexUploader->Map(0, NULL, &pBegin));
    memcpy(pBegin, indices, sizeof(indices));
    pIndexUploader->Unmap(0, NULL);
    
    pCommandList->CopyResource(pIndexBuffer.Get(), pIndexUploader.Get());
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pVertexBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    viewIndexBuffer.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
    viewIndexBuffer.Format = DXGI_FORMAT_R16_UINT;
    viewIndexBuffer.SizeInBytes = sizeof(indices);

    ThrowIfFailedEx(pCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCommandLists);

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
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/入门/HelloCBuffer!/build/Debug/shader.hlsl", NULL, NULL, "VsMain", "vs_5_0", uiCompileFlags, 0, &pVsByteCode, NULL
    ));

    ThrowIfFailedEx(D3DCompileFromFile(
        L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/入门/HelloCBuffer!/build/Debug/shader.hlsl", NULL, NULL, "PsMain", "ps_5_0", uiCompileFlags, 0, &pPsByteCode, NULL
    ));
// Create constant buffer
    UINT uiConstantBufferBytesSize = D3DHelper::CalcConstantBufferBytesSize(sizeof(Transform));

    ThrowIfFailedEx(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uiConstantBufferBytesSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pConstantBuffer)
    ));

    ThrowIfFailedEx(pConstantBuffer->Map(0, NULL, &pConstantBegin));

// Create CBV and heap
    D3D12_DESCRIPTOR_HEAP_DESC descCBVHeap;
    descCBVHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descCBVHeap.NodeMask = 0;
    descCBVHeap.NumDescriptors = 1;
    descCBVHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailedEx(pD3dDevice->CreateDescriptorHeap(
        &descCBVHeap, IID_PPV_ARGS(&pCBVHeap)
    ));

    D3D12_CONSTANT_BUFFER_VIEW_DESC descCBV;
    descCBV.BufferLocation = pConstantBuffer->GetGPUVirtualAddress();
    descCBV.SizeInBytes = uiConstantBufferBytesSize;

    pD3dDevice->CreateConstantBufferView(&descCBV, pCBVHeap->GetCPUDescriptorHandleForHeapStart());


// Create root signature
    CD3DX12_ROOT_PARAMETER rootParameter;

    CD3DX12_DESCRIPTOR_RANGE CBVTable;
    CBVTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    rootParameter.InitAsDescriptorTable(1, &CBVTable);

    D3D12_ROOT_SIGNATURE_DESC descRS;
    descRS.NumParameters = 1;
    descRS.pParameters = &rootParameter;
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

    FlushCommandQueue();
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

    pCommandList->RSSetViewports(1, &stScreenViewport);
    pCommandList->RSSetScissorRects(1, &stScissorRect);

    ID3D12DescriptorHeap* pDescriptorHeaps[] = {pCBVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, pDescriptorHeaps);
    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    pCommandList->SetGraphicsRootDescriptorTable(0, pCBVHeap->GetGPUDescriptorHandleForHeapStart());

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
	
    pSwapChain->Present(0, 0);
    iCurrBackBuffer = (iCurrBackBuffer + 1) % sc_iSwapChainBufferCount;
    FlushCommandQueue();
}

void D3DFrame::Update(const GameTimer& t)
{
    Transform tf;
    CreateMatrix(&tf, t);
    memcpy(pConstantBegin, &tf, D3DHelper::CalcConstantBufferBytesSize(sizeof(Transform)));
}