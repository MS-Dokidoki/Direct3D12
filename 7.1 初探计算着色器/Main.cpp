#include <D3DApp.h>
#include <fstream>
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

#define NUM_ELEMENTS 256

struct Data
{
    XMFLOAT3 v1;
    XMFLOAT2 v2;
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);

    void InitBuffer();
    void BuildRootSignatureAndPipeline();
private:
    ComPtr<ID3D12Resource> InputBufferA, InputBufferB;
    ComPtr<ID3D12Resource> UploadBufferA, UploadBufferB;
    ComPtr<ID3D12Resource> OutputBuffer, ReadBackBuffer;

    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12PipelineState> pPipelineState;
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
    catch(DirectX_Exception& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("DirectX Failed"), MB_OK);
    }
    catch(D3DHelper_Exception& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("D3DHelper Failed"), MB_OK);
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
    
    InitBuffer();
    
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* cmdLists[1] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, cmdLists);

    BuildRootSignatureAndPipeline();
    FlushCommandQueue();

    // Execute Compute Shader
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get()));
    pCommandList->SetComputeRootSignature(pRootSignature.Get());
    pCommandList->SetComputeRootShaderResourceView(0, InputBufferA->GetGPUVirtualAddress());
    pCommandList->SetComputeRootShaderResourceView(1, InputBufferB->GetGPUVirtualAddress());
    pCommandList->SetComputeRootUnorderedAccessView(2, OutputBuffer->GetGPUVirtualAddress());

    pCommandList->Dispatch(1, 1, 1);

    // Read back data from GPU's memory
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        OutputBuffer.Get(), 
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_SOURCE
    ));

    pCommandList->CopyResource(ReadBackBuffer.Get(), OutputBuffer.Get());
	
    ThrowIfFailed(pCommandList->Close());
    pCommandQueue->ExecuteCommandLists(1, cmdLists);
	
	FlushCommandQueue();
    // Map data
    Data* result = NULL;
    ThrowIfFailed(ReadBackBuffer->Map(0, NULL, (void**)&result));
    
    std::ofstream file("result.txt");

    for(int i = 0; i < NUM_ELEMENTS; ++i)
    {
        file << "(" << result[i].v1.x << ", " 
                    << result[i].v1.y << ", "
                    << result[i].v1.z << ")" << "\t(" 
                    << result[i].v2.x << ", "
                    << result[i].v2.y << ")" << std::endl;
    }
    ReadBackBuffer->Unmap(0, NULL);
    return 1;
}

void D3DFrame::InitBuffer()
{
    std::vector<Data> dataA(NUM_ELEMENTS);
    std::vector<Data> dataB(NUM_ELEMENTS);

    for(int i = 0; i < NUM_ELEMENTS; ++i)
    {
        dataA[i].v1 = {(float)i, (float)i, (float)i};
        dataA[i].v2 = {(float)i, 0};

        dataB[i].v1 = {-(float)i, (float)i, 0};
        dataB[i].v2 = {0, -(float)i};
    }

    UINT byteSize = dataA.size() * sizeof(Data);

    InputBufferA = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &dataA[0], byteSize, &UploadBufferA);
    InputBufferB = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &dataB[0], byteSize, &UploadBufferB);
    
    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, NULL, 
        IID_PPV_ARGS(&OutputBuffer)
    ));

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&ReadBackBuffer)
    ));

}

void D3DFrame::BuildRootSignatureAndPipeline()
{
    CD3DX12_ROOT_PARAMETER params[3];
    params[0].InitAsShaderResourceView(0);
    params[1].InitAsShaderResourceView(1);
    params[2].InitAsUnorderedAccessView(0);

    CD3DX12_ROOT_SIGNATURE_DESC desc(3, params);

    ComPtr<ID3DBlob> serializeSign, error;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &serializeSign, &error), &error);

    pD3dDevice->CreateRootSignature(0, serializeSign->GetBufferPointer(), serializeSign->GetBufferSize(), IID_PPV_ARGS(&pRootSignature));

    ComPtr<ID3DBlob> Cs;
    Cs = CompileShader(L"compute.hlsl", NULL, "CsMain", "cs_5_0");

    D3D12_COMPUTE_PIPELINE_STATE_DESC psDesc = {};
    psDesc.pRootSignature = pRootSignature.Get();
    psDesc.CS = {Cs->GetBufferPointer(), Cs->GetBufferSize()};
    psDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ThrowIfFailed(pD3dDevice->CreateComputePipelineState(&psDesc, IID_PPV_ARGS(&pPipelineState)));
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
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));
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
