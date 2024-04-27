#include "ShadowMap.h"

void ShadowMap::Init(ID3D12Device* device, UINT width, UINT height)
{
    pD3dDevice = device;
    nWidth = width;
    nHeight = height;

    stViewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
    stScissorRect = {0, 0, (long)width, (long)height};

    BuildResource();
}

void ShadowMap::BuildResource()
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = nWidth;
    desc.Height = nHeight;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = emFormat;
    desc.SampleDesc = {1, 0};
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&pShadowMap)
    ));
}

void ShadowMap::BuildDescriptor(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
    sthCpuSrv = hCpuSrv;
    sthGpuSrv = hGpuSrv;
    sthCpuDsv = hCpuDsv;

    BuildDescriptor();
}

void ShadowMap::BuildDescriptor()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    pD3dDevice->CreateShaderResourceView(pShadowMap.Get(), &srvDesc, sthCpuSrv);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Texture2D.MipSlice = 0;

    pD3dDevice->CreateDepthStencilView(pShadowMap.Get(), &dsvDesc, sthCpuDsv);
}

void ShadowMap::OnResize(UINT width, UINT height)
{
    if(width != nWidth || height != nHeight)
    {
        nWidth = width;
        nHeight = height;

        BuildResource();
        BuildDescriptor();
    }
}

UINT ShadowMap::Width() const
{
    return nWidth;
}

UINT ShadowMap::Height() const
{
    return nHeight;
}

D3D12_RECT ShadowMap::ScissorRect() const
{
    return stScissorRect;
}

D3D12_VIEWPORT ShadowMap::Viewport() const
{
    return stViewport;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv() const
{
    return sthGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv() const
{
    return sthCpuDsv;
}

ID3D12Resource* ShadowMap::Resource() const
{
    return pShadowMap.Get();
}