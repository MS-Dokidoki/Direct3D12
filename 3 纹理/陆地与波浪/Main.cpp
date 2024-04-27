#include "D3DApp.h"
#include "waves.h"

#define USE_FRAMERESOURCE           // Ĭ�Ͽ���֡��Դ
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
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

    void DrawItems();
private:
    ComPtr<ID3D12PipelineState> pPipelineState, pPipelineState_WireFrame;    // ��Ⱦ����
    ComPtr<ID3D12RootSignature> pRootSignature;                                 // ��ǩ��
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;                                      // ��ɫ����Դ��������

    std::vector<Render::RenderItem> RenderItems;
    std::unordered_map<std::string, Render::Texture> Textures;  // ��������
    std::unordered_map<std::string, Light::Material> Materials; // ��������
    Render::MeshGeometry Geo;   // ����������
    Render::MeshGeometry WaveGeo;
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

void D3DFrame::DrawItems()
{
    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = {pSRVHeap.Get()};
    pCommandList->SetDescriptorHeaps(1, ppHeaps);

    pCommandList->SetGraphicsRootConstantBufferView(2, pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress());
    for(UINT i = 0; i < RenderItems.size(); ++i)
    {
        Render::RenderItem item = RenderItems[i];

        pCommandList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += CONSTANT_VALUE::nCBMaterialByteSize * item.pMaterial->nCBIndex;
        pCommandList->SetGraphicsRootConstantBufferView(1, addr);

        addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += CONSTANT_VALUE::nCBObjectByteSize * item.nCBObjectIndex;
        pCommandList->SetGraphicsRootConstantBufferView(0, addr);

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(item.pMaterial->nDiffuseSrvHeapIndex, nCbvSrvUavDescriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(3, handle);

        pCommandList->DrawIndexedInstanced(item.nIndexCount, 1, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }   

}

void D3DFrame::LoadTextures()
{
    Render::Texture tex;

    // grass
    tex.Name = "grass";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/grass.dds";
    tex.pResource = LoadDDSTextureFromFile12(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;
    // water
    tex.Name = "water";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/water1.dds";
    tex.pResource = LoadDDSTextureFromFile12(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;
}

void D3DFrame::BuildGeometries()
{
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    Geometry::Mesh Grid = Geometry::GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);
    vertices.resize(Grid.vertices.size());
    for(UINT i = 0; i < Grid.vertices.size(); ++i)
    {
        XMFLOAT3 pos = Grid.vertices[i].vec3Position;
        float y = GetHillHeight(pos.x, pos.z);

        vertices[i].Position = XMFLOAT3(pos.x, y, pos.z);
        vertices[i].Normal = GetHillNormal(pos.x, pos.z);
        vertices[i].TexCoords = Grid.vertices[i].vec2TexCoords;
    }
    
    UINT cbVertices = vertices.size() * sizeof(Vertex);
    UINT cbIndices = Grid.indices.size() * sizeof(UINT16);

    Geo.Name = "Geo";
    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], cbVertices, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &Grid.indices[0], cbIndices, &Geo.pUploaderIndexBuffer);

    Geo.nVertexByteStride = sizeof(Vertex);
    Geo.nIndexBufferByteSize = cbIndices;
    Geo.nVertexBufferByteSize = cbVertices;
    Geo.emIndexFormat = DXGI_FORMAT_R16_UINT;
    
    Render::SubmeshGeometry land;
    land.nIndexCount = Grid.indices.size();
    land.nStartIndexLocation = 0;
    land.nBaseVertexLocation = 0;
    
    Geo.DrawArgs["land"] = land;

    Wave.Init(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
    indices.resize(3 * Wave.TriangleCount());
    int m = Wave.RowCount();
    int n = Wave.ColumnCount();
    int k = 0;

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

    Render::SubmeshGeometry wave;
    wave.nIndexCount = indices.size();
    wave.nBaseVertexLocation = 0;
    wave.nStartIndexLocation = 0;

    WaveGeo.DrawArgs["wave"] = wave;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Render::FrameResource::CBTYPE_VERTEX, sizeof(Vertex), Wave.VertexCount());
}

void D3DFrame::BuildMaterials()
{
	Light::Material material;
	
    material.Name = "grass";
	material.nCBIndex = 0;
    material.nDiffuseSrvHeapIndex = 0;
    material.vec4DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
    material.vec3FresnelR0 = XMFLOAT3(0.01, 0.01, 0.01f);
    material.nRoughness = 0.125f;
    material.iNumFramesDirty = 3;

    Materials[material.Name] = material;

    material.Name = "water";
    material.nCBIndex = 1;
    material.nDiffuseSrvHeapIndex = 1;
    material.vec4DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
    material.vec3FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    material.nRoughness = 0.0f;
    material.iNumFramesDirty = 3;

    Materials[material.Name] = material; 
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Render::FrameResource::CBTYPE_MATERIAL, sizeof(Light::MaterialConstant), Materials.size());

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
}

void D3DFrame::BuildRenderItems()
{
    Render::RenderItem item;
    Render::SubmeshGeometry mesh;

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

    RenderItems.push_back(item);

    // water
    mesh = WaveGeo.DrawArgs["wave"];
    item.nCBObjectIndex = 1;
    item.pGeo = &WaveGeo;
    item.pMaterial = &Materials["water"];
    item.nIndexCount = mesh.nIndexCount;
    item.nStartIndexLocation = mesh.nStartIndexLocation;
    item.nBaseVertexLocation = mesh.nBaseVertexLocation;

    RenderItems.push_back(item);
}

void D3DFrame::BuildRootSignature()
{
    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 1, RenderItems.size());

    CD3DX12_STATIC_SAMPLER_DESC sampler[1];
    sampler[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

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
    D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // Shader
    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", NULL, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", NULL, "PsMain", "ps_5_0");

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

    desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pPipelineState_WireFrame)));
}

void D3DFrame::Update(const GameTimer& t)
{
#ifdef USE_FRAMERESOURCE
    // �ڵ�ǰ֡������֮ǰ���Ƚ���������������л�
    iCurrFrameResourceIndex = (iCurrFrameResourceIndex + 1) % nFrameResourceCount;
    pCurrFrameResource = &pFrameResources[iCurrFrameResourceIndex];
    pCommandAllocator = pCurrFrameResource->pCommandAllocator;

    // ����ǰ֡��Դδִ����ɣ�˵�����������ȫ���� GPU ʹ�ã��ȴ�ִ�����
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
    ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), bUseFrame? pPipelineState_WireFrame.Get(): pPipelineState.Get()));
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

    DrawItems();

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
    for(auto& item: RenderItems)
    {
        if(item.iFramesDirty-- > 0)
        {
            Render::ObjectConstant oc;
            XMMATRIX world = XMLoadFloat4x4(&item.matWorld);
            XMMATRIX texTransform = XMLoadFloat4x4(&item.matTexTransform);
            XMStoreFloat4x4(&oc.matWorld, XMMatrixTranspose(world));
            XMStoreFloat4x4(&oc.matTexTransform, XMMatrixTranspose(texTransform));

            pCurrFrameResource->CBObject.CopyData(item.nCBObjectIndex, &oc, sizeof(oc));
        }
    }
}

void D3DFrame::UpdateMaterials(const GameTimer& t)
{
    for(auto item: Materials)
    {
        Light::Material& mat = item.second;

        if(mat.iNumFramesDirty-- > 0)
        {
            Light::MaterialConstant mc;
            mc.nRoughness = mat.nRoughness;
            mc.vec3FresnelR0 = mat.vec3FresnelR0;
            mc.vec4DiffuseAlbedo = mat.vec4DiffuseAlbedo;
            XMMATRIX transform = XMLoadFloat4x4(&mat.matTransform);
            XMStoreFloat4x4(&mc.matTransform, XMMatrixTranspose(transform));

            pCurrFrameResource->CBMaterial.CopyData(mat.nCBIndex, &mc, sizeof(mc));
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
    Render::SceneConstant sc;

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

    sc.lights[0].vec3Direction = {sinf(fSunPhi)*cosf(fSunTheta), cosf(fSunPhi), sinf(fSunPhi)*sinf(fSunTheta)};
    sc.lights[0].vec3Strength = XMFLOAT3(1.0f, 1.0f, 0.9f);

    pCurrFrameResource->CBScene.CopyData(0, &sc, sizeof(Render::SceneConstant));
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

	mat.iNumFramesDirty = nFrameResourceCount;
}