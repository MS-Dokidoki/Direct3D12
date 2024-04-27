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

    static const UINT uiSwapChainBufferCount = 2;                   // ����������������
    DXGI_FORMAT emDXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;          // ���������������ش洢��ʽ

    ComPtr<IDXGISwapChain3> pSwapChain;                             // ����������
    ComPtr<ID3D12Resource> pRenderTargets[uiSwapChainBufferCount];  // ��������̨������
    int iCurrBackBuffer;                                            // ��ǰ���ֵĻ�����

    ComPtr<ID3D12Device> pDevice;   // Direct3D 12 �豸����
    
    //*** �������
    ComPtr<ID3D12CommandQueue> pCommandQueue;   // ������ж���
    ComPtr<ID3D12CommandAllocator> pCommandAllocator;   // �������������
    ComPtr<ID3D12GraphicsCommandList> pCommandList; // �����б����

    //*** ��Ⱦ�ö���
    ComPtr<ID3D12PipelineState> pPipelineState; // ��Ⱦ����״̬����
    ComPtr<ID3D12RootSignature> pRootSignature; // ��ǩ������
    CD3DX12_RECT stScissorRect;                 // �ü�����
    CD3DX12_VIEWPORT stViewport;                // ��Ⱦ�ӿ�

    ComPtr<ID3D12Resource> pVertexBuffer;       // ���㻺��
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;  // ���㻺����ͼ

    ComPtr<ID3D12Fence> pFence;                 // Χ������
    HANDLE hFenceEvent;                         // Χ���¼����
    UINT uiCurrentFence;                        // ��ǰΧ��ֵ

    ComPtr<ID3D12DescriptorHeap> pRTVHeap;      // ��ȾĿ����ͼ��������
    UINT uiRTVDescriptorSize;                   // ��ȾĿ����ͼ�������ߴ�

    
};