#pragma once
#include "D3DBase.h"
#include "Win32.h"

using namespace Microsoft::WRL;

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color;
};

class D3DApp : public Win32
{
public:
    D3DApp();

protected:
    virtual void OnInit();
    virtual void OnDestroy();
    virtual void OnUpdate();
    virtual void OnRender();
private:
    void LoadPipeline();
    void PopulateCommandList();
    void WaitForPreviousFrame();

    static const UINT uiSwapChainBufferCount = 2;                   // 交换链缓冲区数量
    DXGI_FORMAT emDXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;          // 交换链缓冲区像素存储格式

    ComPtr<IDXGISwapChain3> pSwapChain;                             // 交换链对象
    ComPtr<ID3D12Resource> pRenderTargets[uiSwapChainBufferCount];  // 交换链后台缓冲区
    int iCurrBackBuffer;                                            // 当前呈现的缓冲区

    ComPtr<ID3D12Device> pDevice;   // Direct3D 12 设备对象
    
    //*** 命令对象
    ComPtr<ID3D12CommandQueue> pCommandQueue;   // 命令队列对象
    ComPtr<ID3D12CommandAllocator> pCommandAllocator;   // 命令分配器对象
    ComPtr<ID3D12GraphicsCommandList> pCommandList; // 命令列表对象

    //*** 渲染用对象
    ComPtr<ID3D12PipelineState> pPipelineState; // 渲染管线状态对象
    ComPtr<ID3D12RootSignature> pRootSignature; // 根签名对象
    CD3DX12_RECT stScissorRect;                 // 裁剪矩形
    CD3DX12_VIEWPORT stViewport;                // 渲染视口

    ComPtr<ID3D12Resource> pVertexBuffer;       // 顶点缓冲
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;  // 顶点缓冲视图

    ComPtr<ID3D12Fence> pFence;                 // 围栏对象
    HANDLE hFenceEvent;                         // 围栏事件句柄
    UINT uiCurrentFence;                        // 当前围栏值

    ComPtr<ID3D12DescriptorHeap> pRTVHeap;      // 渲染目标视图描述符堆
    UINT uiRTVDescriptorSize;                   // 渲染目标视图描述符尺寸

    
};