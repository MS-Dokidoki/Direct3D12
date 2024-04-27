#pragma once
#include <D3DHelper.h>

class ShadowMap
{
public:
    ShadowMap(){}
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void Init(ID3D12Device* device, UINT width, UINT height);

    UINT Width() const;
    UINT Height() const;
    ID3D12Resource* Resource() const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv() const;

    D3D12_VIEWPORT Viewport() const;
    D3D12_RECT ScissorRect() const;

    void BuildDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);
    void OnResize(UINT width, UINT height);
private:
    void BuildDescriptor();
    void BuildResource();
private:
    ID3D12Device* pD3dDevice = NULL;
    D3D12_VIEWPORT stViewport;
    D3D12_RECT stScissorRect;

    UINT nWidth = 0;
    UINT nHeight = 0;
    DXGI_FORMAT emFormat = DXGI_FORMAT_R24G8_TYPELESS;

    CD3DX12_CPU_DESCRIPTOR_HANDLE sthCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE sthGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE sthCpuDsv;

    Microsoft::WRL::ComPtr<ID3D12Resource> pShadowMap = NULL;
};