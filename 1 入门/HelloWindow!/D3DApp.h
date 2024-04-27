#pragma once
#include "Win32.h"
#include "D3DBase.h"
#include "D3DHelper.h"

using namespace Microsoft::WRL;

class D3DApp : public Win32
{
public:
    D3DApp(UINT, UINT, TCHAR*);
    virtual ~D3DApp(){}

protected:
    virtual void OnInit();
    virtual void OnRender();
    virtual void OnUpdate();
    virtual void OnDestroy();

private:
    void LoadPipeline();
    void PopulateCommandList();
    void WaitForPreviousFrame();
    
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    ComPtr<IDXGISwapChain3> mSwapChain;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12PipelineState> mPipelineState;

    static const UINT SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;
    ComPtr<ID3D12Resource> mRenderTargets[SwapChainBufferCount];

    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    UINT mRTVDescriptorSize;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    HANDLE mFenceHandle;
};