#pragma once
#include <D3DHelper.h>

class SsaoMap
{
public:
    SsaoMap(){};
    SsaoMap(const SsaoMap&) = delete;
    SsaoMap& operator=(const SsaoMap&) = delete;

public:
    /// ��ʼ�� SsaoMap
    void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height);

    /// ��ȡ Ssao ����Ŀ��
    UINT SsaoMapWidth() const;
    /// ��ȡ Ssao ����ĸ߶�
    UINT SsaoMapHeight() const;

    /// ��ȡ�����õ�Ͷ�����ƫ������, ��ֹ�����õ�Ͷ�����ȫ����ͬ
    void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

    /// ��ȡ��������
    ID3D12Resource* NormalMap() const;
    /// ��ȡ Ssao ����
    ID3D12Resource* Resource() const;

    /// ��ȡ�����������ȾĿ����ͼ
    CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv() const;
    /// ��ȡ�����������ɫ����Դ��ͼ
    CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv() const;
    /// ��ȡ Ssao �������ɫ����Դ��ͼ
    CD3DX12_GPU_DESCRIPTOR_HANDLE SsaoMapSrv() const;
    
    /// ������Դ������
    void BuildDescriptors(ID3D12Resource* depthStencilBuffer,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                          CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                          CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
                          UINT cbvSrvUavDescriptorSize,
                          UINT rtvDescriptorSize);

    /// ������Ⱦ����
    void SetPipelineState(ID3D12PipelineState* ssao, ID3D12PipelineState* ssaoBlur);

    /// ������Դ�ߴ�
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
    static const DXGI_FORMAT emNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;   // �۲�ռ䷨������ͼ��ʽ
    static const DXGI_FORMAT emSsaoFormat = DXGI_FORMAT_R16_UNORM;              // �۲�ռ䶥���ڱ�������(Ssao)��ʽ
    static const UINT nBlurMaxRadius = 5;

    Microsoft::WRL::ComPtr<ID3D12Resource> pVectorMap;      // �ڱ��ʲ�����Ͷ�����
    Microsoft::WRL::ComPtr<ID3D12Resource> pVectorUploader; 
    Microsoft::WRL::ComPtr<ID3D12Resource> pNormalMap;      // ��������
    Microsoft::WRL::ComPtr<ID3D12Resource> pSsaoMap;        // �����ڱ�������
    Microsoft::WRL::ComPtr<ID3D12Resource> pSsaoMap1;    // ģ��������
private:
    DirectX::XMFLOAT4 pOffsets[14];                          // ������Ͷ����ߵ�ƫ������

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
    Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSign; // Ssao ��ǩ��
    ID3D12Device* pD3dDevice = NULL;                        // D3D12 �豸
    ID3D12PipelineState* pSsaoPipelineState = NULL;         // Ssao ��Ⱦ����
    ID3D12PipelineState* pSsaoBlurPipelineState = NULL;

    UINT nRenderTargetWidth = 0, nRenderTargetHeight = 0;   // ��ȾĿ��ߴ�

    D3D12_RECT stScissorRect;                               // �ü�����
    D3D12_VIEWPORT stViewport;                              // �ӿ�
};