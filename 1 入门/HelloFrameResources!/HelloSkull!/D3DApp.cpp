#include "D3DApp.h"
#include <vector>
#include <string>
using namespace Microsoft::WRL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    D3DApp* app = (D3DApp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        if(app)
            return app->MsgProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//*************************************
// D3DApp

D3DApp::D3DApp(HINSTANCE hInst) : hInstance(hInst)
{

}

D3DApp::~D3DApp()
{
	if(!pD3dDevice)
    {
        FlushCommandQueue();
        CloseHandle(hFenceEvent);
    }
}

void D3DApp::OnResize()
{
    FlushCommandQueue();
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));

    for(UINT i = 0; i < nSwapChainBufferCount; ++i)
        ppSwapChainBuffer[i].Reset();
    pDepthStencilBuffer.Reset();

    ThrowIfFailed(pSwapChain->ResizeBuffers(
        nSwapChainBufferCount, cxClient, cyClient, emBackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    ));

    iCurrBackBuffer = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for(UINT i = 0; i < nSwapChainBufferCount; ++i)
    {
        ThrowIfFailed(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&ppSwapChainBuffer[i])));
        pD3dDevice->CreateRenderTargetView(ppSwapChainBuffer[i].Get(), NULL, rtvHandle);
        rtvHandle.Offset(1, nRTVDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Width = cxClient;
    depthStencilDesc.Height = cyClient;
    depthStencilDesc.Format = emDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    optClear.Format = emDepthStencilFormat;

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear, IID_PPV_ARGS(&pDepthStencilBuffer)
    ));

    pD3dDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), NULL, DepthStencilBufferView());

    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE
    ));
    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCommandLists);

    ScreenViewport.TopLeftX = 0;
    ScreenViewport.TopLeftY = 0;
    ScreenViewport.Width = (float)cxClient;
    ScreenViewport.Height = (float)cyClient;
    ScreenViewport.MinDepth = 0.0f;
    ScreenViewport.MaxDepth = 1.0f;

    ScissorRect.top = 0;
    ScissorRect.left = 0;
    ScissorRect.right = cxClient;
    ScissorRect.bottom = cyClient;

    FlushCommandQueue();
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_ACTIVATE:
        if(LOWORD(wParam) == WA_INACTIVE)
        {
            bAppPaused = 1;
            Timer.Stop();
        }
        else
        {
            bAppPaused = 0;
            Timer.Start();
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        bAppPaused = 1;
        bResizing = 1;
        Timer.Stop();
        return 0;
    case WM_SIZE:
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);
        return 0;
    case WM_EXITSIZEMOVE:
        bAppPaused = 0;
        bResizing = 0;
        Timer.Start();
        OnResize();
        return 0;
    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        OnMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        OnMouseUp(wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;
    }   

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::Initialize()
{
    if(!InitMainWindow())
        return 0;
    return InitDirect3D();
}

bool D3DApp::InitMainWindow()
{
    WNDCLASSEX wc;
    RECT rect = {0, 0, (long)cxClient, (long)cyClient};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = (HBRUSH)GetStockObject(COLOR_WINDOW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("D3D App");
    
    if(!RegisterClassEx(&wc))
        return 0;

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);
    hwnd = CreateWindowEx(0, TEXT("D3D App"), szMainWndCaption, WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, hInstance, this);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return 1;
}

bool D3DApp::InitDirect3D()
{
    UINT uiFactoryFlags = 0;
    HRESULT hr;

#ifdef _DEBUG
    ComPtr<ID3D12Debug> pDebugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
    pDebugController->EnableDebugLayer();
    uiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    // Create Device
    ThrowIfFailed(CreateDXGIFactory2(uiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

    hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3dDevice));
    if(FAILED(hr))
    {
        ComPtr<IDXGIFactory> pWarpAdapter;
        ThrowIfFailed(pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3dDevice)));
    }

    CreateCommandObjects();
    CreateSwapChain();
    CreateDescriptor();

    // Create Fence
    ThrowIfFailed(pD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    iCurrentFence = 0;
    hFenceEvent = CreateEvent(NULL, 0, 0, NULL);
    
    OnResize();
    return 1;
}

void D3DApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(pD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));
    
    if(bUseFrameResource)
    {
        for(UINT i = 0; i < nFrameResourceCount; ++i)
            ThrowIfFailed(pD3dDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pFrameResources[i].pCommandAllocator)
            ));

        pCurrFrameResource = &pFrameResources[0];
        pCommandAllocator = pFrameResources[0].pCommandAllocator;
    }
    else
    {
        ThrowIfFailed(pD3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&pCommandAllocator)
        ));
    }

    ThrowIfFailed(pD3dDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        pCommandAllocator.Get(), NULL, IID_PPV_ARGS(&pCommandList)
    ));

    pCommandList->Close();

}

void D3DApp::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = nSwapChainBufferCount;
    scd.BufferDesc.Format = emBackBufferFormat;
    scd.BufferDesc.RefreshRate.Numerator = 1;
    scd.BufferDesc.RefreshRate.Denominator = 60;
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scd.BufferDesc.Width = cxClient;
    scd.BufferDesc.Height = cyClient;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;     // 当程序切换为全屏模式时, 它将自动选择合适的显示模式
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;        
    scd.SampleDesc.Quality = 0;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Windowed = 1;

    ThrowIfFailed(pDxgiFactory->CreateSwapChain(
        pCommandQueue.Get(), &scd, &pSwapChain
    ));

    iCurrBackBuffer = 0;
}

void D3DApp::CreateDescriptor()
{
    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = nSwapChainBufferCount;
    heapDesc.NodeMask = 0;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&pRTVHeap)
    ));

    // DSV heap
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.NumDescriptors = 1;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&pDSVHeap)
    ));

    nRTVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    nDSVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    nCbvSrvUavDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

int D3DApp::Run()
{
    MSG msg = {0};

    Timer.Reset();

    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Timer.Tick();

            if(!bAppPaused)
            {
                CalculateFrameStats();
                Update(Timer);
                Render(Timer);
            }
            else
            {
                Sleep(100);
            }
        }
    }

    return msg.wParam;
}


void D3DApp::LogAdapters()
{
    // 枚举所有的适配器并打印相关信息
    UINT i = 0;
    IDXGIAdapter* adapter;
    std::vector<IDXGIAdapter*> adapterList;

    while(pDxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        TCHAR text[Def_BufferMaxSize] = {0};
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        wsprintf(text, TEXT("***Adapter: %s\n"), desc.Description);

        OutputDebugString(text);
        ++i;
    }

    for(unsigned int i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        adapterList[i]->Release();
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    // 枚举适配器的显示输出设备并打印其信息
    UINT i = 0;
    IDXGIOutput* output;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        TCHAR text[Def_BufferMaxSize] = { 0 };
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        wsprintf(text, TEXT("***Output: %s\n"), desc.DeviceName);
        OutputDebugString(text);

        LogOutputDisplayModes(output, DXGI_FORMAT_R8G8B8A8_UNORM);
        output->Release();
        ++i;
    }
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    // 获取显示输出设备的相关信息并打印
    UINT count = 0;
    UINT flags = 0;

    // 获取符合条件的显示模式的个数
    output->GetDisplayModeList(format, flags, &count, NULL);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        TCHAR text[Def_BufferMaxSize];
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;

        wsprintf(text, TEXT(
            "Width = %d Height = %d Refresh = %d / %d\n"
        ), x.Width, x.Height, n, d);
        OutputDebugString(text);
    }
}

void D3DApp::CalculateFrameStats()
{
    static int frameCount = 0;
    static float timeElapsed = 0.0f;

    ++frameCount;

    if((Timer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = (float)frameCount;
        float mspf = 1000.0f / fps;

        TCHAR text[Def_BufferMaxSize] = {0};
        wsprintf(text, TEXT("%s fps: %s mspf: %s"), szMainWndCaption, std::to_wstring(fps).c_str(), std::to_wstring(mspf).c_str());
        SetWindowText(hwnd, text);

        frameCount = 0;
        timeElapsed += 1.0f;
    }
}

float D3DApp::AspectRatio() const
{
    return (float)cxClient / (float)cyClient;
}

HWND D3DApp::MainWnd() const
{
    return hwnd;
}

HINSTANCE D3DApp::AppInstance() const
{
    return hInstance;
}

void D3DApp::FlushCommandQueue()
{
    ThrowIfFailed(pCommandQueue->Signal(pFence.Get(), ++iCurrentFence));

    if(pFence->GetCompletedValue() < iCurrentFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(iCurrentFence, hFenceEvent));

        WaitForSingleObject(hFenceEvent, INFINITE);
    }

}

ID3D12Resource* D3DApp::CurrentBackBuffer() const
{
    return ppSwapChainBuffer[iCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(pRTVHeap->GetCPUDescriptorHandleForHeapStart(), iCurrBackBuffer, nRTVDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilBufferView() const
{
    return pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}