#pragma once
#include <D3DHelper.h>

class CubeRenderTarget
{
public:
	void Init(ID3D12Device* pDevice, UINT width, UINT height, DXGI_FORMAT format);
	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hRtvs[6]);
	void Resize(UINT width, UINT height);
	
	ID3D12Resource* Resource() const;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTV(int index);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();

	D3D12_RECT GetScissorRect() const;
	D3D12_VIEWPORT GetViewport() const;

private:
	void BuildDescriptors();
	void BuildResource();
	
private:
	ID3D12Device* pDevice = NULL;
	UINT Width, Height;
	DXGI_FORMAT Format;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRtvs[6];
	Microsoft::WRL::ComPtr<ID3D12Resource> pCubeResource;
	
	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;
};