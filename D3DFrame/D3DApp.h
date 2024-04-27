#pragma once
#include "D3DHelper.h"
#include "GameTimer.h"

#define UNICODE
#define _UNICODE

#define Def_WndCaptionMaxSize 32
#define Def_WndCaption TEXT("D3D App")
#define Def_D3DDriverType D3D_DRIVER_TYPE_HARDWARE
#define Def_BackBufferFormat DXGI_FORMAT_R8G8B8A8_UNORM 
#define Def_DepthStencilBufferFormat DXGI_FORMAT_D24_UNORM_S8_UINT
#define Def_BufferMaxSize 128
#define Def_SwapChainBufferCount 2
#define Def_ClientWidth 800
#define Def_ClientHeight 600
#define Def_FrameResouceCount 3

#ifdef _M_X64
#define PTR_ADDRESS unsigned long long
#else
#define PTR_ADDRESS unsigned int
#endif
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

    virtual void CreateDescriptor();
protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();

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
    UINT64 iCurrentFence;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator;                   
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;

    static const UINT nSwapChainBufferCount = Def_SwapChainBufferCount;
    int iCurrBackBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> ppSwapChainBuffer[nSwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> pDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pRTVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDSVHeap;
    UINT nRTVDescriptorSize;
    UINT nDSVDescriptorSize;
    UINT nCbvSrvUavDescriptorSize;

    D3D12_VIEWPORT ScreenViewport;
    D3D12_RECT ScissorRect;
    
    TCHAR szMainWndCaption[Def_WndCaptionMaxSize] = Def_WndCaption;
    D3D_DRIVER_TYPE emD3DDriverType = Def_D3DDriverType;
    DXGI_FORMAT emBackBufferFormat = Def_BackBufferFormat;
    DXGI_FORMAT emDepthStencilFormat = Def_DepthStencilBufferFormat;
    int cxClient = Def_ClientWidth;
    int cyClient = Def_ClientHeight;

    bool bUseFrameResource;
    static const UINT nFrameResourceCount = Def_FrameResouceCount;
    UINT iCurrFrameResourceIndex = 0;
    D3DHelper::Resource::FrameResource pFrameResources[nFrameResourceCount];
    D3DHelper::Resource::FrameResource* pCurrFrameResource = NULL; 
};