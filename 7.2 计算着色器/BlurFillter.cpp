#include "BlurFillter.h"

void BlurFillter::Init(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
    Device = device;
    Width = width;
    Height = height;
    Format = format;

    BuildResources();
}

void BlurFillter::Execute(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSign, ID3D12PipelineState* horz, ID3D12PipelineState* vert, ID3D12Resource* input, int blurCount)
{
    auto weights = CalcGaussWeights(2.5f);
    int blurRadius = (int)weights.size() / 2;

    cmdList->SetComputeRootSignature(rootSign);

    cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
    cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), &weights[0], 1);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        input, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE
    ));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        Blur0.Resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
    ));

    cmdList->CopyResource(Blur0.Resource.Get(), input);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        Blur0.Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        Blur1.Resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    ));

    for(int i = 0; i < blurCount; ++i)
    {
        cmdList->SetPipelineState(horz);
        cmdList->SetComputeRootDescriptorTable(1, Blur0.hGPUSrv);
        cmdList->SetComputeRootDescriptorTable(2, Blur1.hGPUUav);

        UINT numGroupsX = (UINT)ceilf(Width / 256.0f);
        cmdList->Dispatch(numGroupsX, Height, 1);

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            Blur0.Resource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS 
        ));

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            Blur1.Resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ
        ));

        cmdList->SetPipelineState(vert);

        cmdList->SetComputeRootDescriptorTable(1, Blur1.hGPUSrv);
        cmdList->SetComputeRootDescriptorTable(2, Blur0.hGPUUav);

        UINT numGroupsY = (UINT)ceilf(Height / 256.0f);
        cmdList->Dispatch(Width, numGroupsX, 1);

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            Blur0.Resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ
        ));

        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            Blur1.Resource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        ));

    }
}

void BlurFillter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDesc, CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUDesc, UINT nDescSize)
{
    Blur0.hCPUSrv = hCPUDesc;
    Blur0.hCPUUav = hCPUDesc.Offset(1, nDescSize);
    Blur1.hCPUSrv = hCPUDesc.Offset(1, nDescSize);
    Blur1.hCPUUav = hCPUDesc.Offset(1, nDescSize);

    Blur0.hGPUSrv = hGPUDesc;
    Blur0.hGPUUav = hGPUDesc.Offset(1, nDescSize);
    Blur1.hGPUSrv = hGPUDesc.Offset(1, nDescSize);
    Blur1.hGPUUav = hGPUDesc.Offset(1, nDescSize);

    BuildDescriptors();
}

void BlurFillter::BuildDescriptors()
{
    assert(Blur0.Resource != NULL && Blur1.Resource != NULL);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; 
    srvDesc.Format = Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    Device->CreateShaderResourceView(Blur0.Resource.Get(), &srvDesc, Blur0.hCPUSrv);
    Device->CreateShaderResourceView(Blur1.Resource.Get(), &srvDesc, Blur1.hCPUSrv);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    Device->CreateUnorderedAccessView(Blur0.Resource.Get(), NULL, &uavDesc, Blur0.hCPUUav);
    Device->CreateUnorderedAccessView(Blur1.Resource.Get(), NULL, &uavDesc, Blur1.hCPUUav);
}

void BlurFillter::BuildResources()
{
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = Format;
    resDesc.Width = Width;
    resDesc.Height = Height;
    resDesc.Alignment = 0;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &resDesc, 
        D3D12_RESOURCE_STATE_COMMON,
        NULL, IID_PPV_ARGS(&Blur0.Resource)
    ));

    ThrowIfFailed(Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &resDesc, 
        D3D12_RESOURCE_STATE_COMMON,
        NULL, IID_PPV_ARGS(&Blur1.Resource)
    ));
}

std::vector<float> BlurFillter::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f * sigma * sigma;
    int blurRadius = (int)ceil(2.0f * sigma);

    assert(blurRadius <= MaxBlurRadius);

    std::vector<float> weights;
    weights.resize(2 * blurRadius + 1);

    float weightSum = 0.0f;
    for(int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = (float)i;
        weights[i + blurRadius] = expf(-x * x / twoSigma2);
        weightSum += weights[i + blurRadius];
    }

    for(int i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
} 

void BlurFillter::Resize(UINT width, UINT height)
{
    if((width != Width || height != Height) && Device)
    {
        Width = width;
        Height = height;

        BuildResources();
        BuildDescriptors();
    }
}

ID3D12Resource* BlurFillter::Output() const
{
    return Blur0.Resource.Get();
}