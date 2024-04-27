#include "SsaoMap.h"
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace D3DHelper;
using namespace DirectX::PackedVector;

void SsaoMap::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height)
{
    pD3dDevice = device;

    OnResize(width, height);
    
    BuildOffsetVectors();
    BuildRandomVectorTexture(cmdList);
}

void SsaoMap::BuildOffsetVectors()
{
    pOffsets[0] = {+1.0f, +1.0f, +1.0f, 0.0f};
    pOffsets[1] = {-1.0f, -1.0f, -1.0f, 0.0f};
    pOffsets[2] = {-1.0f, +1.0f, +1.0f, 0.0f};
    pOffsets[3] = {+1.0f, -1.0f, -1.0f, 0.0f};
    pOffsets[4] = {+1.0f, +1.0f, -1.0f, 0.0f};
    pOffsets[5] = {-1.0f, -1.0f, +1.0f, 0.0f};
    pOffsets[6] = {-1.0f, +1.0f, -1.0f, 0.0f};
    pOffsets[7] = {+1.0f, -1.0f, +1.0f, 0.0f};
    pOffsets[8] = {-1.0f, 0.0f, 0.0f, 0.0f};
    pOffsets[9] = {+1.0f, 0.0f, 0.0f, 0.0f};
    pOffsets[10] = {0.0f, -1.0f, 0.0f, 0.0f};
    pOffsets[11] = {0.0f, +1.0f, 0.0f, 0.0f};
    pOffsets[12] = {0.0f, 0.0f, -1.0f, 0.0f};
    pOffsets[13] = {0.0f, 0.0f, +1.0f, 0.0f};

    for(UINT i = 0; i < 14; ++i)
    {
        float s = MathHelper::RandomF(.25f, 1.0f);
        XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&pOffsets[i]));

        XMStoreFloat4(&pOffsets[i], v);
    }
}

void SsaoMap::SetPipelineState(ID3D12PipelineState* ssao, ID3D12PipelineState* ssaoBlur)
{
    pSsaoPipelineState = ssao;
    pSsaoBlurPipelineState = ssaoBlur;
}

void SsaoMap::BuildDescriptors(ID3D12Resource* depthStencilBuffer,
                            CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                            CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                            CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
                            UINT cbvSrvUavDescriptorSize,
                            UINT rtvDescriptorSize)
{
    hSsaoCpuSrv = hCpuSrv;
    hSsao1CpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hNormalCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hDepthCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hVectorCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    hSsaoGpuSrv = hGpuSrv;
    hSsao1GpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hNormalGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hDepthGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    hVectorGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    hNormalCpuRtv = hCpuRtv;
    hSsaoCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
    hSsao1CpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

    RebuildDescriptors(depthStencilBuffer);
}

void SsaoMap::RebuildDescriptors(ID3D12Resource* depthStencilBuffer)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = emNormalFormat;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    pD3dDevice->CreateShaderResourceView(pNormalMap.Get(), &srvDesc, hNormalCpuSrv);

    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    pD3dDevice->CreateShaderResourceView(depthStencilBuffer, &srvDesc, hDepthCpuSrv);

    srvDesc.Format = emSsaoFormat;
    pD3dDevice->CreateShaderResourceView(pSsaoMap.Get(), &srvDesc, hSsaoCpuSrv);

    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pD3dDevice->CreateShaderResourceView(pVectorMap.Get(), &srvDesc, hVectorCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = emNormalFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    pD3dDevice->CreateRenderTargetView(pNormalMap.Get(), &rtvDesc, hNormalCpuRtv);

    rtvDesc.Format = emSsaoFormat;
    pD3dDevice->CreateRenderTargetView(pSsaoMap.Get(), &rtvDesc, hSsaoCpuRtv);
    pD3dDevice->CreateRenderTargetView(pSsaoMap1.Get(), &rtvDesc, hSsao1CpuRtv);
}

void SsaoMap::BuildResources()
{
    pNormalMap = nullptr;
    pSsaoMap = nullptr;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.DepthOrArraySize = 1;
    desc.Alignment = 0;
    desc.Width = nRenderTargetWidth;
    desc.Height = nRenderTargetHeight;
    desc.Format = emNormalFormat;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float clearColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = emNormalFormat;
    CopyMemory(optClear.Color, clearColor, 4 * sizeof(float));

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&pNormalMap)
    ));

    float ssaoClearColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    desc.Width = nRenderTargetWidth / 2;
    desc.Height = nRenderTargetHeight / 2;
    desc.Format = emSsaoFormat;
    optClear.Format = emSsaoFormat;
    CopyMemory(optClear.Color, ssaoClearColor, 4 * sizeof(float));

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&pSsaoMap)
    ));

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&pSsaoMap1)
    ));
}

void SsaoMap::OnResize(UINT width, UINT height)
{
    if(width != nRenderTargetWidth || height != nRenderTargetHeight)
    {
        nRenderTargetWidth = width;
        nRenderTargetHeight = height;

        stViewport.TopLeftX = 0.0f;
        stViewport.TopLeftY = 0.0f;
        stViewport.Width = nRenderTargetWidth / 2.0f;
        stViewport.Height = nRenderTargetHeight / 2.0f;
        stViewport.MinDepth = 0.0f;
        stViewport.MaxDepth = 1.0f;

        stScissorRect = {0, 0, (long)nRenderTargetWidth / 2, (long)nRenderTargetHeight / 2};
        BuildResources();
    }
}

void SsaoMap::ComputeSsao(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS addrCBScene, UINT blurCount)
{
    cmdList->SetPipelineState(pSsaoPipelineState);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pSsaoMap.Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    float clearValue[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    cmdList->ClearRenderTargetView(hSsaoCpuRtv, clearValue, 0, NULL);
    cmdList->OMSetRenderTargets(1, &hSsaoCpuRtv, 1, NULL);

    cmdList->SetGraphicsRootConstantBufferView(0, addrCBScene);
    cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    cmdList->SetGraphicsRootDescriptorTable(2, hNormalGpuSrv);
    cmdList->SetGraphicsRootDescriptorTable(3, hVectorGpuSrv);

    cmdList->IASetVertexBuffers(0, 0, NULL);
    cmdList->IASetIndexBuffer(NULL);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(6, 1, 0, 0);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pSsaoMap.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    BlurAmbientMap(cmdList, addrCBScene, blurCount);
}

void SsaoMap::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS addrCBScene, UINT blurCount)
{
    cmdList->SetPipelineState(pSsaoBlurPipelineState);
    cmdList->SetGraphicsRootConstantBufferView(0, addrCBScene);

    for(UINT i = 0; i < blurCount; ++i)
    {
        BlurAmbientMap(cmdList, 1);
        BlurAmbientMap(cmdList, 0);
    }
}

void SsaoMap::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool bHorz)
{
    ID3D12Resource* output;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hInputSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hOutputRtv;

    if(bHorz)
    {
        output = pSsaoMap1.Get();
        hInputSrv = hSsaoGpuSrv;
        hOutputRtv = hSsao1CpuRtv;

        cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
    }
    else
    {
        output = pSsaoMap.Get();
        hInputSrv = hSsao1GpuSrv;
        hOutputRtv = hSsaoCpuRtv;

        cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
    }

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        output,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    float clear[] = {1.0f, 1.0f, 1.0f, 1.0f};
    cmdList->OMSetRenderTargets(1, &hOutputRtv, 0, NULL);
    cmdList->ClearRenderTargetView(hOutputRtv, clear, 0, NULL);
    
    cmdList->SetGraphicsRootDescriptorTable(2, hNormalGpuSrv);
    cmdList->SetGraphicsRootDescriptorTable(3, hInputSrv);

    cmdList->IASetVertexBuffers(0, 0, NULL);
    cmdList->IASetIndexBuffer(NULL);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(6, 1, 0, 0);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        output,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));
}

void SsaoMap::CalcGaussWeights(float sigma, std::vector<float>& weights)
{
    float twoSigma = 2.0f * sigma * sigma;
    float weightSum = 0.0f;
    int radius = ceil(2.0f * sigma);

    assert(radius <= nBlurMaxRadius);
    
    // G(x) = exp(-x * x / 2 * sigma * sigma)
    weights.resize(radius * 2 + 1);
    for(int x = -radius; x <= radius; ++x)
    {
        weights[x + radius] = expf(-x * x / twoSigma);
        weightSum += weights[x + radius];
    }

    for(int i = 0; i < weights.size(); ++i)
        weights[i] /= weightSum;
}

UINT SsaoMap::SsaoMapWidth() const
{
    return nRenderTargetWidth / 2;
}

UINT SsaoMap::SsaoMapHeight() const
{
    return nRenderTargetHeight / 2;
}

void SsaoMap::GetOffsetVectors(XMFLOAT4 offsets[14])
{
    CopyMemory(offsets, pOffsets, 14 * sizeof(XMFLOAT4));
}

ID3D12Resource* SsaoMap::NormalMap() const
{
    return pNormalMap.Get();
}

ID3D12Resource* SsaoMap::Resource() const
{
    return pSsaoMap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SsaoMap::NormalMapRtv() const
{
    return hNormalCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SsaoMap::NormalMapSrv() const
{
    return hNormalGpuSrv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SsaoMap::SsaoMapSrv() const
{
    return hSsaoGpuSrv;
}

void SsaoMap::BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.DepthOrArraySize = 1;
    desc.Alignment = 0;
    desc.MipLevels = 1;
    desc.Width = 256;
    desc.Height = 256;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&pVectorMap)
    ));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(pVectorMap.Get(), 0, 1);

    ThrowIfFailed(pD3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pVectorUploader)
    ));

    XMCOLOR data[256 * 256];

    for(UINT i = 0; i < 256; ++i)
        for(UINT j = 0; j < 256; ++j)
            data[i * 256 + j] = XMCOLOR(MathHelper::Random(), MathHelper::Random(), MathHelper::Random(), 0.0f);
    
    D3D12_SUBRESOURCE_DATA subResource;
    subResource.pData = data;
    subResource.RowPitch = 256 * sizeof(XMCOLOR);
    subResource.SlicePitch = 256 * subResource.RowPitch;

    UpdateSubresources(cmdList, pVectorMap.Get(), pVectorUploader.Get(), 0, 0, 1, &subResource);
    
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pVectorMap.Get(), 
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ
    ));
     
}
