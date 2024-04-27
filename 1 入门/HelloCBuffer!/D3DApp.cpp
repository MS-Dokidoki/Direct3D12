#include "D3DApp.h"
#include <vector>
#include <string>
using namespace Microsoft::WRL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static D3DApp* app = (D3DApp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

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

void D3DApp::FlushCommandQueue()
{
    ThrowIfFailed(pCommandQueue->Signal(pFence.Get(), ++uiCurrentFence));

    if(pFence->GetCompletedValue() < uiCurrentFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(uiCurrentFence, hFenceEvent));

        WaitForSingleObject(hFenceEvent, INFINITE);
    }

}

ID3D12Resource* D3DApp::CurrentBackBuffer() const
{
    return ppSwapChainBuffer[iCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(pRTVHeap->GetCPUDescriptorHandleForHeapStart(), iCurrBackBuffer, uiRTVDescriptorSize);
}


void D3DApp::OnResize()
{
    stScissorRect.bottom = (long)cyClient;
    stScissorRect.right = (long)cxClient;

    stScreenViewport.Width = (float)cxClient;
    stScreenViewport.Height = (float)cyClient;
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
        ((MINMAXINFO*)lParam)->ptMaxTrackSize.y = 200;
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
    ThrowIfFailedEx(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
    pDebugController->EnableDebugLayer();
    uiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    // Create Device
    ThrowIfFailedEx(CreateDXGIFactory2(uiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

    hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3dDevice));
    if(FAILED(hr))
    {
        ComPtr<IDXGIFactory> pWarpAdapter;
        ThrowIfFailedEx(pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailedEx(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3dDevice)));
    }

    CreateCommandObjects();
    CreateSwapChain();
    CreateDescriptor();

    // Create Fence
    ThrowIfFailedEx(pD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    uiCurrentFence = 0;
    hFenceEvent = CreateEvent(NULL, 0, 0, NULL);
    
    // Set View and scissor
    stScreenViewport.TopLeftX = 0;
    stScreenViewport.TopLeftY = 0;
    stScreenViewport.Width = cxClient;
    stScreenViewport.Height = cyClient;
    stScreenViewport.MinDepth = 0.0f;
    stScreenViewport.MaxDepth = 1.0f;

    stScissorRect.left = 0;
    stScissorRect.top = 0;
    stScissorRect.right = (float)cxClient;
    stScissorRect.bottom = (float)cyClient;
    FlushCommandQueue();
    return 1;
}

void D3DApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailedEx(pD3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));

    ThrowIfFailedEx(pD3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&pCommandAllocator)
    ));

    ThrowIfFailedEx(pD3dDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        pCommandAllocator.Get(), NULL, IID_PPV_ARGS(&pCommandList)
    ));

    pCommandList->Close();

}

void D3DApp::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = sc_iSwapChainBufferCount;
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
    scd.SampleDesc.Count = 1;           // 先暂时不开启多重采样
    scd.SampleDesc.Quality = 0;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Windowed = 1;

    ThrowIfFailedEx(pDxgiFactory->CreateSwapChain(
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
    heapDesc.NumDescriptors = sc_iSwapChainBufferCount;
    heapDesc.NodeMask = 0;

    ThrowIfFailedEx(pD3dDevice->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&pRTVHeap)
    ));

    uiRTVDescriptorSize = pD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // RTVs
    CD3DX12_CPU_DESCRIPTOR_HANDLE hRTVDescriptor(pRTVHeap->GetCPUDescriptorHandleForHeapStart());

    for(UINT i = 0; i < sc_iSwapChainBufferCount; ++i)
    {
        ThrowIfFailedEx(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&ppSwapChainBuffer[i])));
        pD3dDevice->CreateRenderTargetView(ppSwapChainBuffer[i].Get(), NULL, hRTVDescriptor);
        hRTVDescriptor.Offset(1, uiRTVDescriptorSize);
    }
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