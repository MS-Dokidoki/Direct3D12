#include "D3DApp.h"
#include "waves.h"

#define USE_FRAMERESOURCE           // 默认开启帧资源
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
};

enum RenderType
{
    Opaque = 0,
	Transparent,
	AlphaTested,
	Calculate,
    WireFrame
};

class D3DFrame : public D3DApp
{
public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void OnResize();
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    void OnMouseMove(WPARAM, int, int);
    void OnMouseDown(WPARAM, int, int);
    void OnMouseUp(WPARAM, int, int);
    
    void InputProcess(const GameTimer&);
    void UpdateCamera(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateScene(const GameTimer&);
    void UpdateWaves(const GameTimer&);
    void UpdateMaterials(const GameTimer&);
    void AnimateMaterials(const GameTimer&);

    void LoadTextures();
    void BuildMaterials();
    void BuildGeometries();
    void BuildRenderItems();
    void BuildSRV();
    void BuildRootSignature();
    void BuildPipelineState();

    void DrawItems(ID3D12GraphicsCommandList*, std::vector<RenderItem>& );
private:
    std::unordered_map<UINT, ComPtr<ID3D12PipelineState>> PipelineStates;    // 渲染管线
    ComPtr<ID3D12RootSignature> pRootSignature;                                 // 根签名
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;                                      // 着色器资源描述符堆

    std::unordered_map<UINT, std::vector<RenderItem>> RenderItems;
    std::unordered_map<std::string, Resource::Texture> Textures;  // 纹理数据
    std::unordered_map<std::string, Light::Material> Materials; // 材质数据
    Resource::MeshGeometry Geo;   // 几何体数据
    Resource::MeshGeometry WaveGeo;
    Waves Wave;

private:
    bool bUseFrame = 0;
    float fTheta = 1.5f * XM_PI;
    float fPhi   = XM_PIDIV2 - 0.1f;
    float fRadius = 50.0f;
    float fSunTheta = 1.25f * XM_PI;
    float fSunPhi = XM_PIDIV4;

    POINT lastPos;

    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos;
    
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    try
    {
        D3DFrame app(hInstance);

        if(!app.Initialize())
            return 0;
        return app.Run();
    }
    catch(DXException& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("HR Failed"), MB_OK);
    }

    return 0;
}

#define GetHillHeight(x, z) (0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z)))
#define RAND(a, b) a + rand() % ((b - a) + 1)
#define RANDF(a, b) a + ((float)(rand()) / (float)RAND_MAX) * (b - a)

inline XMFLOAT3 GetHillNormal(float x, float z) 
{
    XMVECTOR vecN;
    XMFLOAT3 n;
    vecN = XMVector3Normalize(XMVectorSet(-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 
                      1.0f, 
                      -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z), 0.0f));
    XMStoreFloat3(&n, vecN);
    return n;
}

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance){}

bool D3DFrame::Initialize()
{
#ifdef USE_FRAMERESOURCE
    bUseFrameResource = 1;     
#endif
    if(!D3DApp::Initialize())
        return 0;
    
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), NULL));
    LoadTextures();
    BuildGeometries();
    BuildMaterials();
    BuildSRV();
    BuildRenderItems();

    ThrowIfFailed(pCommandList->Close());
    ID3D12CommandList* ppCList[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCList);

    BuildRootSignature();
    BuildPipelineState();

    FlushCommandQueue();
    return 1;
}

void D3DFrame::DrawItems(ID3D12GraphicsCommandList* pCmdList, std::vector<RenderItem>& items)
{
    for(UINT i = 0; i < items.size(); ++i)
    {
        RenderItem item = items[i];

        pCmdList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCmdList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCmdList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += CONSTANT_VALUE::nCBMaterialByteSize * item.pMaterial->nCBMaterialIndex;
        pCmdList->SetGraphicsRootConstantBufferView(1, addr);

        addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += CONSTANT_VALUE::nCBObjectByteSize * item.nCBObjectIndex;
        pCmdList->SetGraphicsRootConstantBufferView(0, addr);

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(item.pMaterial->nDiffuseSrvHeapIndex, nCbvSrvUavDescriptorSize);
        pCmdList->SetGraphicsRootDescriptorTable(3, handle);

        pCmdList->DrawIndexedInstanced(item.nIndexCount, 1, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }   

}

void D3DFrame::LoadTextures()
{
    Resource::Texture tex;

    // grass
    tex.Name = "grass";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/grass.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures[tex.Name] = tex;

    // water
    tex.Name = "water";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/water1.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures[tex.Name] = tex;

    tex.Name = "wireFence";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/WireFence.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);
    Textures[tex.Name] = tex;

}

void D3DFrame::BuildGeometries()
{
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    Geometry::Mesh Grid = Geometry::GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);
    Geometry::Mesh Box = Geometry::GeometryGenerator::CreateBox(5.0, 5.0, 5.0);
    int k = 0;

    vertices.resize(Grid.vertices.size() + Box.vertices.size());
    for(UINT i = 0; i < Grid.vertices.size(); ++i, ++k)
    {
        XMFLOAT3 pos = Grid.vertices[i].vec3Position;
        float y = GetHillHeight(pos.x, pos.z);

        vertices[k].Position = XMFLOAT3(pos.x, y, pos.z);
        vertices[k].Normal = GetHillNormal(pos.x, pos.z);
        vertices[k].TexCoords = Grid.vertices[i].vec2TexCoords;
    }
    
    for(UINT i = 0; i < Box.vertices.size(); ++i, ++k)
    {
        vertices[k].Position = Box.vertices[i].vec3Position;
        vertices[k].Normal = Box.vertices[i].vec3Normal;
        vertices[k].TexCoords = Box.vertices[i].vec2TexCoords;
    }

    indices.insert(std::end(indices), std::begin(Grid.indices), std::end(Grid.indices));
    indices.insert(std::end(indices), std::begin(Box.indices), std::end(Box.indices));

    UINT cbVertices = vertices.size() * sizeof(Vertex);
    UINT cbIndices = indices.size() * sizeof(UINT16);

    Geo.Name = "Geo";
    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], cbVertices, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], cbIndices, &Geo.pUploaderIndexBuffer);

    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.nIndexBufferByteSize = cbIndices;
    Geo.nVertexBufferByteSize = cbVertices;
    Geo.emIndexFormat = DXGI_FORMAT_R16_UINT;
    
    Resource::SubmeshGeometry land;
    land.nIndexCount = Grid.indices.size();
    land.nStartIndexLocation = 0;
    land.nBaseVertexLocation = 0;
    
    Resource::SubmeshGeometry box;
    box.nIndexCount = Box.indices.size();
    box.nStartIndexLocation = Grid.indices.size();
    box.nBaseVertexLocation = Grid.vertices.size();
    Geo.DrawArgs["land"] = land;
    Geo.DrawArgs["box"] = box;

    Wave.Init(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
    indices.resize(3 * Wave.TriangleCount());
    int m = Wave.RowCount();
    int n = Wave.ColumnCount();
    k = 0;

    for(UINT i = 0; i < m - 1; ++i)
    {
        for(UINT j = 0; j < n - 1; ++j)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;
            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; 
        }
    }

    cbIndices = indices.size() * sizeof(UINT16);

    WaveGeo.Name = "Wave";
    WaveGeo.pCPUVertexBuffer = nullptr;
    WaveGeo.pGPUVertexBuffer = nullptr;
    WaveGeo.pUploaderVertexBuffer = nullptr;

    WaveGeo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), indices.data(), cbIndices, &WaveGeo.pUploaderIndexBuffer);
    WaveGeo.nVertexByteStride = sizeof(Vertex);
    WaveGeo.nVertexBufferByteSize = sizeof(Vertex) * Wave.VertexCount();
    WaveGeo.nIndexBufferByteSize = cbIndices;

    Resource::SubmeshGeometry wave;
    wave.nIndexCount = indices.size();
    wave.nBaseVertexLocation = 0;
    wave.nStartIndexLocation = 0;

    WaveGeo.DrawArgs["wave"] = wave;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Resource::FrameResource::CBTYPE_VERTEX, sizeof(Vertex), Wave.VertexCount());
}

void D3DFrame::BuildMaterials()
{
	Light::Material material;
	
    material.Name = "grass";
	material.nCBMaterialIndex = 0;
    material.nDiffuseSrvHeapIndex = 0;
    material.vec4DiffuseAlbedo = XMFLOAT4(1.0, 1.0, 1.0, 1.0f);
    material.vec3FresnelR0 = XMFLOAT3(0.01, 0.01, 0.01f);
    material.nRoughness = 0.125f;
    material.matTransform = MathHelper::Identity4x4();
    material.iFramesDirty = 3;

    Materials[material.Name] = material;

    material.Name = "water";
    material.nCBMaterialIndex = 1;
    material.nDiffuseSrvHeapIndex = 1;
    material.vec4DiffuseAlbedo = XMFLOAT4(1.0, 1.0, 1.0, 0.5f);
    material.vec3FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    material.nRoughness = 0.0f;
    material.iFramesDirty = 3;

    Materials[material.Name] = material;

    material.Name = "wireFence";
    material.nCBMaterialIndex = 2;
    material.nDiffuseSrvHeapIndex = 2;
    material.nRoughness = 0.25f;

    Materials[material.Name] = material;
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Resource::FrameResource::CBTYPE_MATERIAL, sizeof(Constant::MaterialConstant), Materials.size());

}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    desc.NumDescriptors = Materials.size();
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_SHADER_RESOURCE_VIEW_DESC resDesc = {};
    ID3D12Resource* pTexture;

    pTexture = Textures["grass"].pResource.Get();
    resDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    resDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    resDesc.Format = pTexture->GetDesc().Format;
    resDesc.Texture2D.MostDetailedMip = 0;
    resDesc.Texture2D.MipLevels = -1;
    pD3dDevice->CreateShaderResourceView(pTexture, &resDesc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    pTexture = Textures["water"].pResource.Get();
    resDesc.Format = pTexture->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexture, &resDesc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    pTexture = Textures["wireFence"].pResource.Get();
    resDesc.Format = pTexture->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexture, &resDesc, handle);
}

void D3DFrame::BuildRenderItems()
{
    Resource::SubmeshGeometry mesh;
    RenderItem item;

    // land
    mesh = Geo.DrawArgs["land"];
    item.iFramesDirty = 3;
    item.nCBObjectIndex = 0;
    item.pGeo = &Geo;
    item.pMaterial = &Materials["grass"];
    item.nIndexCount = mesh.nIndexCount;
    item.nBaseVertexLocation = mesh.nBaseVertexLocation;
    item.nStartIndexLocation = mesh.nStartIndexLocation;
    item.emPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    item.matWorld = MathHelper::Identity4x4();
    XMStoreFloat4x4(&item.matTexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));

    RenderItems[Opaque].push_back(item);

    mesh = Geo.DrawArgs["box"];
    item.nCBObjectIndex = 1;
    item.pMaterial = &Materials["wireFence"];
    item.nIndexCount = mesh.nIndexCount;
    item.nStartIndexLocation = mesh.nStartIndexLocation;
    item.nBaseVertexLocation = mesh.nBaseVertexLocation;
    item.matTexTransform = MathHelper::Identity4x4();
    XMStoreFloat4x4(&item.matWorld, XMMatrixTranslation(0.0, 5.0, 0.0f));

    RenderItems[AlphaTested].push_back(item);

    // water
    mesh = WaveGeo.DrawArgs["wave"];
    item.nCBObjectIndex = 2;
    item.pGeo = &WaveGeo;
    item.pMaterial = &Materials["water"];
    item.nIndexCount = mesh.nIndexCount;
    item.nStartIndexLocation = mesh.nStartIndexLocation;
    item.nBaseVertexLocation = mesh.nBaseVertexLocation;
    item.matWorld = MathHelper::Identity4x4();
    XMStoreFloat4x4(&item.matTexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));

    RenderItems[Transparent].push_back(item);
    
}

void D3DFrame::BuildRootSignature()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, RenderItems.size());

    CD3DX12_STATIC_SAMPLER_DESC sampler[1];
    sampler[0].Init(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);

    CD3DX12_DESCRIPTOR_RANGE range[1];
    CD3DX12_ROOT_PARAMETER params[4];
    CD3DX12_ROOT_SIGNATURE_DESC desc;

    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    params[0].InitAsConstantBufferView(0);
    params[1].InitAsConstantBufferView(1);
    params[2].InitAsConstantBufferView(2);
    params[3].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_PIXEL);

    desc.Init(4, params, 1, sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerialize;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerialize, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerialize->GetBufferPointer(), pSerialize->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
}

void D3DFrame::BuildPipelineState()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
    D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Shader
    D3D_SHADER_MACRO NormalDefine[] = {
        "FOG", "1",
        NULL, NULL
    };
    D3D_SHADER_MACRO AlphaTestedDefine[] = {
        "FOG", "1",
        "ALPHA_TESTED", "1",
        NULL, NULL
    };

    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;
    ComPtr<ID3DBlob> pPsByteCode_AlphaTested;

    pVsByteCode = CompileShader(L"shader.hlsl", NormalDefine, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NormalDefine, "PsMain", "ps_5_0");
    pPsByteCode_AlphaTested = CompileShader(L"shader.hlsl", AlphaTestedDefine, "PsMain", "ps_5_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.InputLayout = {inputLayouts, _countof(inputLayouts)};
    desc.pRootSignature = pRootSignature.Get();
    desc.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    desc.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = emBackBufferFormat;
    desc.DSVFormat = emDepthStencilFormat;
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Opaque] = pPipelineState;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTest = desc;
    alphaTest.PS = {pPsByteCode_AlphaTested->GetBufferPointer(), pPsByteCode_AlphaTested->GetBufferSize()};
    alphaTest.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&alphaTest, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[AlphaTested] = pPipelineState;

    D3D12_RENDER_TARGET_BLEND_DESC BlendDesc = {};
    BlendDesc.BlendEnable = 1;
    BlendDesc.LogicOpEnable = 0;
    BlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    BlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    BlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    BlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    BlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    BlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    BlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    BlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    desc.BlendState.RenderTarget[0] = BlendDesc;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Transparent] = pPipelineState;


}

void D3DFrame::Update(const GameTimer& t)
{
#ifdef USE_FRAMERESOURCE
    // 在当前帧命令存放之前，先进行命令分配器的切换
    iCurrFrameResourceIndex = (iCurrFrameResourceIndex + 1) % nFrameResourceCount;
    pCurrFrameResource = &pFrameResources[iCurrFrameResourceIndex];
    pCommandAllocator = pCurrFrameResource->pCommandAllocator;

    // 若当前帧资源未执行完成，说明命令分配器全都被 GPU 使用，等待执行完成
    if(pCurrFrameResource->iFence != 0 && pFence->GetCompletedValue() < pCurrFrameResource->iFence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(pCurrFrameResource->iFence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }
#endif

    InputProcess(t);
    AnimateMaterials(t);
    UpdateCamera(t);
    UpdateScene(t);
    UpdateMaterials(t);
    UpdateObjects(t);
    UpdateWaves(t);
    
}

void D3DFrame::Render(const GameTimer& t)
{
    ThrowIfFailed(pCommandAllocator->Reset());
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), PipelineStates[Opaque].Get()));
    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));

    pCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 1, &DepthStencilBufferView());
    pCommandList->RSSetScissorRects(1, &ScissorRect);
    pCommandList->RSSetViewports(1, &ScreenViewport);

    float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
    pCommandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, NULL);
    pCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = {pSRVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, ppHeaps);

    pCommandList->SetGraphicsRootConstantBufferView(2, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());

    DrawItems(pCommandList.Get(), RenderItems[Opaque]);
    pCommandList->SetPipelineState(PipelineStates[Transparent].Get());
    DrawItems(pCommandList.Get(), RenderItems[Transparent]);
    pCommandList->SetPipelineState(PipelineStates[AlphaTested].Get());
    DrawItems(pCommandList.Get(), RenderItems[AlphaTested]);
    
    pCommandList->ResourceBarrier(
    1, &CD3DX12_RESOURCE_BARRIER::Transition(
        ppSwapChainBuffer[iCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    ));
    pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = {pCommandList.Get()};
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	
    ThrowIfFailed(pSwapChain->Present(0, 0));
    iCurrBackBuffer = (iCurrBackBuffer + 1) % nSwapChainBufferCount;
	
#ifdef USE_FRAMERESOURCE
    pCurrFrameResource->iFence = (UINT)++iCurrentFence;
    pCommandQueue->Signal(pFence.Get(), iCurrentFence);
#else
    FlushCommandQueue();
#endif
}

void D3DFrame::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX proj = XMMatrixPerspectiveFovLH(45.0f, AspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&matProj, proj);
}

void D3DFrame::OnMouseDown(WPARAM btnState, int x, int y)
{
    lastPos.x = x;
    lastPos.y = y;

    SetCapture(hwnd);
}

void D3DFrame::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void D3DFrame::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*(x - lastPos.x));
        float dy = XMConvertToRadians(0.25f*(y - lastPos.y));

        // Update angles based on input to orbit camera around box.
        fTheta += dx;
        fPhi += dy;

        // Restrict the angle fPhi.
        fPhi < 0.1f? fPhi = 0.1f: fPhi > XM_PI - 0.1f? fPhi = XM_PI - 0.1f: 0;

    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - lastPos.x);
        float dy = 0.05f*static_cast<float>(y - lastPos.y);

        // Update the camera radius based on input.
        fRadius += dx - dy;

        // Restrict the radius.
        fRadius < 5.0? fRadius = 5.0: fRadius > 150.0? fRadius = 150.0: 0;
    }

    lastPos.x = x;
    lastPos.y = y;
}


void D3DFrame::UpdateCamera(const GameTimer &t)
{
    vec3EyePos.x = fRadius * sinf(fPhi) * cosf(fTheta);
    vec3EyePos.z = fRadius * sinf(fPhi) * sinf(fTheta);
    vec3EyePos.y = fRadius * cosf(fPhi);

    XMVECTOR vecEyePos = XMVectorSet(vec3EyePos.x, vec3EyePos.y, vec3EyePos.z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR target = XMVectorZero();

    XMMATRIX view = XMMatrixLookAtLH(vecEyePos, target, up);
    XMStoreFloat4x4(&matView, view);
}


void D3DFrame::UpdateObjects(const GameTimer& t)
{
    for(auto& itemMap: RenderItems)
    {
        for(auto& item: itemMap.second)
        {
            if(item.iFramesDirty-- > 0)
            {
                Constant::ObjectConstant oc;
                XMMATRIX world = XMLoadFloat4x4(&item.matWorld);
                XMMATRIX texTransform = XMLoadFloat4x4(&item.matTexTransform);
                XMStoreFloat4x4(&oc.matWorld, XMMatrixTranspose(world));
                XMStoreFloat4x4(&oc.matTexTransform, XMMatrixTranspose(texTransform));

                pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &oc, sizeof(oc));
            }
        }
    }
}

void D3DFrame::UpdateMaterials(const GameTimer& t)
{
    for(auto item: Materials)
    {
        Light::Material& mat = item.second;

        if(mat.iFramesDirty-- > 0)
        {
            Constant::MaterialConstant mc;
            mc.nRoughness = mat.nRoughness;
            mc.vec3FresnelR0 = mat.vec3FresnelR0;
            mc.vec4DiffuseAlbedo = mat.vec4DiffuseAlbedo;
            XMMATRIX transform = XMLoadFloat4x4(&mat.matTransform);
            XMStoreFloat4x4(&mc.matTransform, XMMatrixTranspose(transform));

            pCurrFrameResource->CBMaterial.CopyData(mat.nCBMaterialIndex, &mc, sizeof(mc));
        }
    }
}

void D3DFrame::InputProcess(const GameTimer& t)
{
    const float dt = t.DeltaTime();

    if(GetAsyncKeyState(VK_LEFT) & 0x8000)
        fSunTheta -= 1.0f * dt;
    if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
        fSunTheta += 1.0f * dt;
    if(GetAsyncKeyState(VK_UP) & 0x8000)
        fSunPhi -= 1.0f * dt;
    if(GetAsyncKeyState(VK_DOWN) & 0x8000)
        fSunPhi += 1.0f * dt;
    fSunPhi = fSunPhi > 0.1f? fSunPhi < XM_PIDIV2? fSunPhi: XM_PIDIV2: 0.1f;
}

void D3DFrame::UpdateWaves(const GameTimer& gt)
{
    static float fBaseTime = 0.0f;
    if((gt.TotalTime() - fBaseTime) >= 0.25f)
    {
        fBaseTime += 0.25f;

        int i = RAND(4, Wave.RowCount() - 5);
        int j = RAND(4, Wave.ColumnCount() - 5);

        float r = RANDF(0.2f, 0.5f);

        Wave.Disturb(i, j, r);
    }

    Wave.Update(gt.DeltaTime());

    UploadBuffer& currWaveVertex = pCurrFrameResource->CBVertex;

    for(int i = 0; i < Wave.VertexCount(); ++i)
    {
        Vertex v;

        v.Position = Wave.Position(i);
        v.Normal = Wave.Normal(i);
        v.TexCoords = {0.5f + v.Position.x / Wave.Width(), 0.5f - v.Position.z / Wave.Depth()};

        currWaveVertex.CopyData(i, &v, sizeof(Vertex));
    }

    WaveGeo.pGPUVertexBuffer = currWaveVertex.Resource();
}

void D3DFrame::UpdateScene(const GameTimer& t)
{
    Constant::SceneConstant sc;

    XMMATRIX view = XMLoadFloat4x4(&matView);
    XMMATRIX proj = XMLoadFloat4x4(&matProj);
    XMMATRIX matInvView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX matInvProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX matViewProj = XMMatrixMultiply(view, proj);
    XMMATRIX matInvViewProj = XMMatrixInverse(&XMMatrixDeterminant(matViewProj), matViewProj);
    
    XMStoreFloat4x4(&sc.matView, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sc.matInvView, XMMatrixTranspose(matInvView));
    XMStoreFloat4x4(&sc.matProj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&sc.matInvProj, XMMatrixTranspose(matInvProj));
    XMStoreFloat4x4(&sc.matViewProj, XMMatrixTranspose(matViewProj));
    XMStoreFloat4x4(&sc.matInvViewProj, XMMatrixTranspose(matInvViewProj));
    sc.vec3EyePos = vec3EyePos;
    sc.vec2RenderTargetSize = XMFLOAT2(cxClient, cyClient);
    sc.vec2InvRenderTargetSize = XMFLOAT2(1.0f/ cxClient, 1.0f/ cyClient);
    sc.fNearZ = 0.1f;
    sc.fFarZ = 1000.0f;
    sc.fDeltaTime = t.DeltaTime();
    sc.fTotalTime = t.TotalTime();
    sc.vec4AmbientLight = {0.25f, 0.25f, 0.35f, 1.0f};
    sc.vec4FogColor = {0.7f, 0.7f, 0.7f, 1.0f};
	sc.fFogStart = 5.0f;
	sc.fFogRange = 150.0f;
    sc.lights[0].vec3Direction = {sinf(fSunPhi)*cosf(fSunTheta), cosf(fSunPhi), sinf(fSunPhi)*sinf(fSunTheta)};
    sc.lights[0].vec3Strength = XMFLOAT3(1.0f, 1.0f, 0.9f);

    pCurrFrameResource->CBScene.CopyData(0, &sc, sizeof(Constant::SceneConstant));
}

void D3DFrame::AnimateMaterials(const GameTimer& t)
{
    Light::Material& mat = Materials["water"];

	float& tu = mat.matTransform(3, 0);
	float& tv = mat.matTransform(3, 1);

	tu += 0.1f * t.DeltaTime();
	tv += 0.02f * t.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;

	mat.matTransform(3, 0) = tu;
	mat.matTransform(3, 1) = tv;

	mat.iFramesDirty = nFrameResourceCount;
}