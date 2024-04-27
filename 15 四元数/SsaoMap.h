#pragma once
#include <D3DHelper.h>

class SsaoMap
{
public:
    SsaoMap(){};
    SsaoMap(const SsaoMap&) = delete;
    SsaoMap& operator=(const SsaoMap&) = delete;

public:
    /// 初始化 SsaoMap
    void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height);

    /// 获取 Ssao 纹理的宽度
    UINT SsaoMapWidth() const;
    /// 获取 Ssao 纹理的高度
    UINT SsaoMapHeight() const;

    /// 获取测试用的投射光线偏移数据, 防止测试用的投射光线全部趋同
    void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

    /// 获取法线纹理
    ID3D12Resource* NormalMap() const;
    /// 获取 Ssao 纹理
    ID3D12Resource* Resource() const;

    /// 获取法线纹理的渲染目标视图
    CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv() const;
    /// 获取法线纹理的着色器资源视图
    CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv() const;
    /// 获取 Ssao 纹理的着色器资源视图
    CD3DX12_GPU_DESCRIPTOR_HANDLE SsaoMapSrv() const;
    
    /// 构建资源描述符
    void BuildDescriptors(ID3D12Resource* depthStencilBuffer,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                          CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
                          UINT cbvSrvUavDescriptorSize,
                          UINT rtvDescriptorSize);

    /// 设置渲染管线
    void SetPipelineState(ID3D12PipelineState* ssao, ID3D12PipelineState* ssaoBlur);

    /// 调整资源尺寸
    void OnResize(UINT width, UINT height);

    void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);
	
	void ComputeSsao(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS ssaoConstantBufferAddress, UINT blurCount);

    void CalcGaussWeights(float sigma, std::vector<float>& weights);

private:

    void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddr, UINT blurCount);
    void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool bHorz);

    void BuildResources();
    void BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList);
    void BuildOffsetVectors();
private:
    static const DXGI_FORMAT emNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;   // 观察空间法线纹理图格式
    static const DXGI_FORMAT emSsaoFormat = DXGI_FORMAT_R16_UNORM;              // 观察空间顶点遮蔽率纹理(Ssao)格式
    static const UINT nBlurMaxRadius = 5;

    Microsoft::WRL::ComPtr<ID3D12Resource> pVectorMap;      // 遮蔽率测试用投射光线
    Microsoft::WRL::ComPtr<ID3D12Resource> pVectorUploader; 
    Microsoft::WRL::ComPtr<ID3D12Resource> pNormalMap;      // 法线纹理
    Microsoft::WRL::ComPtr<ID3D12Resource> pSsaoMap;        // 顶点遮蔽率纹理
    Microsoft::WRL::ComPtr<ID3D12Resource> pSsaoMap1;    // 模糊过程用
private:
    DirectX::XMFLOAT4 pOffsets[14];                          // 测试用投射光线的偏移数据

    CD3DX12_CPU_DESCRIPTOR_HANDLE hNormalCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hNormalGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hNormalCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDepthCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hDepthGpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hVectorCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hVectorGpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hSsaoCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hSsaoGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hSsao1CpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE hSsao1GpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hSsaoCpuRtv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE hSsao1CpuRtv;

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSign; // Ssao 根签名
    ID3D12Device* pD3dDevice = NULL;                        // D3D12 设备
    ID3D12PipelineState* pSsaoPipelineState = NULL;         // Ssao 渲染管线
    ID3D12PipelineState* pSsaoBlurPipelineState = NULL;

    UINT nRenderTargetWidth = 0, nRenderTargetHeight = 0;   // 渲染目标尺寸

    D3D12_RECT stScissorRect;                               // 裁剪矩形
    D3D12_VIEWPORT stViewport;                              // 视口
};