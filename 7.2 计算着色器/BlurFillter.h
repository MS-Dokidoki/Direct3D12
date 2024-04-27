#pragma once
#include <D3DHelper.h>

class BlurFillter
{
struct BlurObject
{
    D3D12_CPU_DESCRIPTOR_HANDLE hCPUSrv, hCPUUav;
    D3D12_GPU_DESCRIPTOR_HANDLE hGPUSrv, hGPUUav;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = NULL;
};
public:
    void Init(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
    void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUDescriptor, UINT DescriptorSize);
    void Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSign, ID3D12PipelineState* horz, ID3D12PipelineState* vert, ID3D12Resource* input, int blurCount);
    void Resize(UINT width, UINT height);
    ID3D12Resource* Output() const;
    
private:
    void BuildDescriptors();
    void BuildResources();
    std::vector<float> CalcGaussWeights(float sigma);

private:
    const UINT MaxBlurRadius = 5;

    ID3D12Device* Device = NULL;
    UINT Width, Height;
    DXGI_FORMAT Format;
    
    BlurObject Blur0, Blur1;
};