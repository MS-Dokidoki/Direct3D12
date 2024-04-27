#pragma once
#include "D3DAppDefine.h"


/// @brief Direct3D 应用程序框架
class D3DApp
{
protected:
    D3DApp(HINSTANCE);
    D3DApp(const D3DApp&) = delete;
    D3DApp& operator=(const D3DApp&) = delete;
    virtual ~D3DApp();

public:
    HINSTANCE AppInstance() const;

    HWND MainWnd() const;
	
    float AspectRatio() const;

    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);

protected:
    virtual void OnResize();
    virtual void Update(const GameTimer&) = 0;
    virtual void Render(const GameTimer&) = 0;

    virtual void OnMouseDown(WPARAM, int, int){}
    virtual void OnMouseUp(WPARAM, int, int){}
    virtual void OnMouseMove(WPARAM, int, int){}

protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateDescriptor();

    /// @brief 强制 CPU 等待 GPU 处理完所有的命令
    void FlushCommandQueue();

    void CalculateFrameStats();
    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter*);
    void LogOutputDisplayModes(IDXGIOutput*, DXGI_FORMAT);

    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferView() const;
    
protected:
    HINSTANCE hInstance;
    HWND hwnd;
    bool bAppPaused = 0;
    bool bMinimized = 0;
    bool bMaximized = 0;
    bool bResizing = 0;
    bool bFullscreenState = 0;
    GameTimer Timer;

    Microsoft::WRL::ComPtr<IDXGIFactory4> pDxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> pSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> pD3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> pFence;
	HANDLE hFenceEvent;
    UINT64 uiCurrentFence;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;

    static const int sc_iSwapChainBufferCount = Def_SwapChainBufferCount;
    int iCurrBackBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ppSwapChainBuffer[sc_iSwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> pDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pRTVHeap;
    UINT uiRTVDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCBVHeap;

    D3D12_VIEWPORT stScreenViewport;
    D3D12_RECT stScissorRect;
    
    TCHAR szMainWndCaption[Def_WndCaptionMaxSize] = Def_WndCaption;
    D3D_DRIVER_TYPE em_D3D_DriverType = Def_D3D_DriverType;
    DXGI_FORMAT emBackBufferFormat = Def_BackBufferFormat;
    DXGI_FORMAT emDepthStencilFormat = Def_DepthStencilBufferFormat;
    int cxClient = Def_ClientWidth;
    int cyClient = Def_ClientHeight;
    
};