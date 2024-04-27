#include "CubeRenderTarget.h"

void CubeRenderTarget::Init(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	pDevice = device;
	Width = width;
	Height = height;
	Format = format;
	
	ScissorRect = {0, 0, (LONG)width, (LONG)height};
	Viewport.Width = width;
	Viewport.Height = height;
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	BuildResource();
}

void CubeRenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = 0;
	desc.Width = Width;
	desc.Height = Height;
	desc.DepthOrArraySize = 6;
	desc.MipLevels = 1;
	desc.Format = Format;
	desc.SampleDesc = {1, 0};
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	
	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = Format;
	ZeroMemory(optClear.Color, 4 * sizeof(float));

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&pCubeResource)
	));
}

void CubeRenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvs[6])
{
	hCpuSrv = cpuSrv;
	hGpuSrv = gpuSrv;
	CopyMemory(hRtvs, rtvs, sizeof(CD3DX12_CPU_DESCRIPTOR_HANDLE) * 6);
	
	BuildDescriptors();
}

void CubeRenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	
	pDevice->CreateShaderResourceView(pCubeResource.Get(), &srvDesc, hCpuSrv);
	
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	for(UINT i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		
		pDevice->CreateRenderTargetView(pCubeResource.Get(), &rtvDesc, hRtvs[i]);
	}
}

void CubeRenderTarget::Resize(UINT width, UINT height)
{
	if(Width != width || Height != height && pDevice)
	{
		Width = width;
		Height = height;
		
		BuildDescriptors();
		BuildResource();
	}
}

ID3D12Resource* CubeRenderTarget::Resource() const
{
	return pCubeResource.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE CubeRenderTarget::GetRTV(int index)
{
	return hRtvs[index];
}

CD3DX12_GPU_DESCRIPTOR_HANDLE CubeRenderTarget::GetSRV()
{
	return hGpuSrv;
}

D3D12_RECT CubeRenderTarget::GetScissorRect() const
{
	return ScissorRect;
}

D3D12_VIEWPORT CubeRenderTarget::GetViewport() const
{
	return Viewport;
} 