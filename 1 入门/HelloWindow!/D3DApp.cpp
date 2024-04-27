#include "D3DApp.h"

D3DApp::D3DApp(UINT width, UINT height, TCHAR* caption) : Win32(width, height, caption)
{}

void D3DApp::OnInit()
{
    LoadPipeline();
}

void D3DApp::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif
    ComPtr<IDXGIFactory4> dxgiFactory;
    ThrowIfFailedEx(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    // --------- Create Device
    HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));

    if(FAILED(hr))
    {
        ComPtr<IDXGIAdapter> warpAdapter;

        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
    }

    // --------- Create Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;    // ��ʾ�ö����е����������ֱ����Ⱦ
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailedEx(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    // --------- Create Swap Chain(Simple)
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.BufferCount = SwapChainBufferCount;     // ����������������
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   // �ý���������������ȾĿ��
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Format = mBackBufferFormat;          // ��������������ʽ
    scDesc.Width = GetWidth();
    scDesc.Height = GetHeight();
    scDesc.SampleDesc.Count = 1;        // �������� �����ʾ�����ö��ز���

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailedEx(dxgiFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(),
        GetHWND(),
        &scDesc,
        NULL, NULL,
        &swapChain
    ));

    ThrowIfFailedEx(dxgiFactory->MakeWindowAssociation(GetHWND(), DXGI_MWA_NO_ALT_ENTER));  // �ر�ȫ��
    ThrowIfFailedEx(swapChain.As(&mSwapChain));
    mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();

    //------- Create RTV  Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;      // ��������һ�� RTV ��������
    rtvDesc.NumDescriptors = SwapChainBufferCount;      // RTV ����
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;    
    rtvDesc.NodeMask = 0;

    ThrowIfFailedEx(mDevice->CreateDescriptorHeap(
        &rtvDesc,
        IID_PPV_ARGS(&mRTVHeap)
    ));
    mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //----- Create RTVs
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());

    for(UINT i = 0; i < SwapChainBufferCount; ++i)
    {
        ThrowIfFailedEx(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
        mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), NULL, rtvHandle);
        rtvHandle.Offset(1, mRTVDescriptorSize);
    }


    //------ Create Command Objects

    // Create Command Allocator
    ThrowIfFailedEx(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));

    // Create Command List
    ThrowIfFailed(mDevice->CreateCommandList(
        0,      // �ڵ�����. �ڶ� GPU ϵͳ��, ��ֵΪ���� GPU ����������; �� GPU ��Ϊ 0 (Ҳ���Ե���Ҫ�󶨵��� GPU)
        D3D12_COMMAND_LIST_TYPE_DIRECT, // �洢����������
        mCommandAllocator.Get(),        // �󶨵����������
        NULL, 
        IID_PPV_ARGS(&mCommandList)
    ));

    mCommandList->Close();

    //------ Create Fence
    ThrowIfFailedEx(mDevice->CreateFence(
        0, D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&mFence)
    ));
    
    
    mFenceHandle = CreateEvent(NULL, 0, 0, NULL);
}

void D3DApp::OnUpdate()
{

}

void D3DApp::OnRender()
{   
    // Record command to Command List
    PopulateCommandList();

    // Execute command list
    ID3D12CommandList* ppCommandLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    
    // Present back buffer
    ThrowIfFailed(mSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3DApp::PopulateCommandList()
{
    // ֻ�� GPU ִ���� CommandAllocator �е������ �ſ�������
    // ����, CommadnList �������ύ�� GPU ���κ�ʱ���ڱ�����
    ThrowIfFailed(mCommandAllocator->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get()));

    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            mRenderTargets[mCurrBackBuffer].Get(), 
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    );
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRTVDescriptorSize);
    
    const float ClearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    mCommandList->ClearRenderTargetView(rtv, ClearColor, 0, NULL);

    mCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            mRenderTargets[mCurrBackBuffer].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        )
    );

    ThrowIfFailed(mCommandList->Close());
}

void D3DApp::OnDestroy()
{
    WaitForPreviousFrame();
    CloseHandle(mFenceHandle);
}

void D3DApp::WaitForPreviousFrame()
{
    ++mCurrentFence;

    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));


    if(mFence->GetCompletedValue() < mCurrentFence)
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, mFenceHandle));
        WaitForSingleObject(mFenceHandle, INFINITE);
        
    }

    mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();
}