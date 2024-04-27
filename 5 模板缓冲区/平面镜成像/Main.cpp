#include "D3DApp.h"
#define USE_FRAMERESOURCE
using namespace Microsoft::WRL;
using namespace D3DHelper;
using namespace DirectX;


union SkullModelVertex
{
    float v[6];
    struct
    {
        float Position[3];
        float Normal[3];
    };
};

union SkullModelIndex
{
    UINT16 i[3];
    struct
    {
        UINT16 first;
        UINT16 second;
        UINT16 thrid;
    };
};

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoords;
    Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty): Position(px, py, pz), Normal(nx, ny, nz), TexCoords(tx, ty){}
    Vertex(){}
};

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices);
class D3DFrame : public D3DApp
{
    enum RenderType
    {
        Opaque = 0,
        Transparent,
        AlphaTested,
        Calculate,
        WireFrame,
        Mirror,
        Reflected,
        Shadow
    };

public:
    D3DFrame(HINSTANCE);
    
    virtual bool Initialize();

protected:
    virtual void OnResize();
    virtual void Render(const GameTimer&);
    virtual void Update(const GameTimer&);
    virtual void OnMouseUp(WPARAM, int, int);
    virtual void OnMouseMove(WPARAM, int, int);
    virtual void OnMouseDown(WPARAM, int, int);

    void LoadTextures();
    void BuildGeometries();
    void BuildMaterials();
    void BuildSRV();
    void BuildRenderItems();
    void BuildRootSignature();
    void BuildPipelineStates();

    void InputProcess(const GameTimer&);
    void UpdateScene(const GameTimer&);
    void UpdateObjects(const GameTimer&);
    void UpdateMaterials(const GameTimer&);
    void UpdateCamera(const GameTimer&);

    void DrawItems(std::vector<RenderItem>&);

private:
    std::unordered_map<RenderType, ComPtr<ID3D12PipelineState>> PipelineStates;
    ComPtr<ID3D12RootSignature> pRootSignature;
    ComPtr<ID3D12DescriptorHeap> pSRVHeap;

    std::unordered_map<RenderType, std::vector<RenderItem>> RenderItems;
    std::unordered_map<std::string, Resource::Texture> Textures;
    std::unordered_map<std::string, Light::Material> Materials;
    Resource::MeshGeometry Geo;
private:
    float fTheta = 1.5f * XM_PI;
    float fPhi   = XM_PIDIV2 - 0.1f;
    float fRadius = 50.0f;
    POINT lastPos;

    XMFLOAT4X4 matProj;
    XMFLOAT4X4 matView;
    XMFLOAT3 vec3EyePos, skullTranslation = {0.0f, 1.0f, -5.0f};
    RenderItem* reflectedSkull, *skull, *shadowSkull;
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

D3DFrame::D3DFrame(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

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
    ID3D12CommandList* ppCmdLists[] = {pCommandList.Get()};
    pCommandQueue->ExecuteCommandLists(1, ppCmdLists);

    BuildRootSignature();
    BuildPipelineStates();

    FlushCommandQueue();
    return 1;
}

void D3DFrame::LoadTextures()
{
    Resource::Texture tex;
    // checkBoard
    tex.Name = "checkboard";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/checkboard.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;
    // mat
    tex.Name = "brick";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/bricks.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;
    // ice
    tex.Name = "ice";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/ice.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;
    // white
    tex.Name = "white";
    tex.FileName = L"C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Resources/white1x1.dds";
    tex.pResource = LoadDDSFromFile(pD3dDevice.Get(), pCommandList.Get(), tex.FileName.c_str(), &tex.pUploader);

    Textures[tex.Name] = tex;

}

void D3DFrame::BuildSRV()
{
    D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc;
    SRVHeapDesc.NodeMask = 0;
    SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    SRVHeapDesc.NumDescriptors = Textures.size();
    SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailed(pD3dDevice->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&pSRVHeap)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = -1;

    ID3D12Resource* pTexRes = Textures["brick"].pResource.Get();
    desc.Format = pTexRes->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexRes, &desc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    pTexRes = Textures["ice"].pResource.Get();
    desc.Format = pTexRes->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexRes, &desc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    pTexRes = Textures["checkboard"].pResource.Get();
    desc.Format = pTexRes->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexRes, &desc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

    pTexRes = Textures["white"].pResource.Get();
    desc.Format = pTexRes->GetDesc().Format;
    pD3dDevice->CreateShaderResourceView(pTexRes, &desc, handle);
    handle.Offset(1, nCbvSrvUavDescriptorSize);

}

void D3DFrame::BuildGeometries()
{
    Vertex roomVertices[] = {
        Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
    };
    UINT16 roomIndices[] = {
        // Floor
		0, 1, 2,	
		0, 2, 3,

		// Walls
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
    };
    std::vector<SkullModelVertex> skullVertices;
    std::vector<SkullModelIndex> skullIndices;
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;

    LoadSkullModel(skullVertices, skullIndices);

    UINT k = _countof(roomVertices);
    vertices.resize(_countof(roomVertices) + skullVertices.size());
    CopyMemory(&vertices[0], roomVertices, sizeof(roomVertices));
    for(UINT i = 0; i < skullVertices.size(); ++i, ++k)
    {
        vertices[k].Position = XMFLOAT3(skullVertices[i].Position);
        vertices[k].Normal = XMFLOAT3(skullVertices[i].Normal);
        vertices[k].TexCoords = {0.0, 0.0};
    }

    indices.resize(_countof(roomIndices) + skullIndices.size() * 3);
    CopyMemory(&indices[0], roomIndices, sizeof(roomIndices));
    CopyMemory(&indices[_countof(roomIndices)], &skullIndices[0], 3 * skullIndices.size() * sizeof(UINT16));

    Resource::SubmeshGeometry floor;
    floor.nIndexCount = 6;
    floor.nBaseVertexLocation = 0;
    floor.nStartIndexLocation = 0;

    Resource::SubmeshGeometry wall;
    wall.nIndexCount = 18;
    wall.nStartIndexLocation = 6;
    wall.nBaseVertexLocation = 0;

    Resource::SubmeshGeometry mirror;
    mirror.nIndexCount = 6;
    mirror.nStartIndexLocation = 24;
    mirror.nBaseVertexLocation = 0;

    Resource::SubmeshGeometry skull;
    skull.nIndexCount = skullIndices.size() * 3;
    skull.nStartIndexLocation = _countof(roomIndices);
    skull.nBaseVertexLocation = _countof(roomVertices);

    UINT nVertexBytesSize = vertices.size() * sizeof(Vertex);
    UINT nIndexBytesSize = indices.size() * sizeof(UINT16);

    Geo.Name = "geo";
    Geo.pGPUVertexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &vertices[0], nVertexBytesSize, &Geo.pUploaderVertexBuffer);
    Geo.pGPUIndexBuffer = CreateDefaultBuffer(pD3dDevice.Get(), pCommandList.Get(), &indices[0], nIndexBytesSize, &Geo.pUploaderIndexBuffer);
    
    Geo.nIndexBufferByteSize = nIndexBytesSize;
    Geo.nVertexBufferByteSize = nVertexBytesSize;
    Geo.nVertexByteStride = sizeof(Vertex);

    Geo.DrawArgs["floor"] = floor;
    Geo.DrawArgs["wall"] = wall;
    Geo.DrawArgs["mirror"] = mirror;
    Geo.DrawArgs["skull"] = skull;

}   

void D3DFrame::BuildMaterials()
{
    Light::Material mat;
    mat.iFramesDirty = 3;
    mat.Name = "brick";
    mat.nCBMaterialIndex = 0;
    mat.nDiffuseSrvHeapIndex = 0;
    mat.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 1.0f};
    mat.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    mat.nRoughness = 0.25f;
    mat.matTransform = MathHelper::Identity4x4();
    Materials[mat.Name] = mat;

    mat.Name = "checkBoard";
    mat.nCBMaterialIndex = 2;
    mat.nDiffuseSrvHeapIndex = 2;
    mat.vec3FresnelR0 = {0.07f, 0.07f, 0.07f};
    mat.nRoughness = 0.3f;
    Materials[mat.Name] = mat;

    mat.Name = "skull";
    mat.nCBMaterialIndex = 3;
    mat.nDiffuseSrvHeapIndex = 3;
    mat.vec3FresnelR0 = {0.05f, 0.05f, 0.05f};
    mat.nRoughness = 0.3f;
    Materials[mat.Name] = mat;
	
    mat.Name = "iceMirror";
    mat.nCBMaterialIndex = 1;
    mat.nDiffuseSrvHeapIndex = 1;
    mat.vec4DiffuseAlbedo = {1.0f, 1.0f, 1.0f, 0.3f};
	mat.vec3FresnelR0 = {0.1f, 0.1f, 0.1f};
    mat.nRoughness = 0.5f;
    Materials[mat.Name] = mat;

    mat.Name = "skullShadow";
    mat.nCBMaterialIndex = 4;
    mat.nDiffuseSrvHeapIndex = 3;
    mat.vec4DiffuseAlbedo = {0.0f, 0.0f, 0.0f, 0.5f};
    mat.vec3FresnelR0 = {0.001f, 0.001f, 0.001f};
    mat.nRoughness = 0.0f;
    Materials[mat.Name] = mat;

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitOtherConstantBuffer(pD3dDevice.Get(), Resource::FrameResource::CBTYPE_MATERIAL, 0, Materials.size());
}

void D3DFrame::BuildRootSignature()
{
    CD3DX12_STATIC_SAMPLER_DESC AnisotropicWrap;
    AnisotropicWrap.Init(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 8);

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    CD3DX12_DESCRIPTOR_RANGE range[1];
    CD3DX12_ROOT_PARAMETER params[4];

    range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    params[0].InitAsConstantBufferView(0);  // Object
    params[1].InitAsConstantBufferView(1);  // Material
    params[2].InitAsConstantBufferView(2);  // Scene
    params[3].InitAsDescriptorTable(1, range, D3D12_SHADER_VISIBILITY_PIXEL);

    desc.Init(4, params, 1, &AnisotropicWrap, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> pSerializeRootSignature;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializeRootSignature, &pError), &pError);
    ThrowIfFailed(pD3dDevice->CreateRootSignature(0, pSerializeRootSignature->GetBufferPointer(), pSerializeRootSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
}

void D3DFrame::BuildRenderItems()
{
    RenderItem item;
    item.iFramesDirty = 3;
    item.nCBObjectIndex = 0;
    item.pMaterial = &Materials["checkBoard"];
    item.pGeo = &Geo;
    item.nIndexCount = item.pGeo->DrawArgs["floor"].nIndexCount;
    item.nBaseVertexLocation = item.pGeo->DrawArgs["floor"].nBaseVertexLocation;
    item.nStartIndexLocation = item.pGeo->DrawArgs["floor"].nStartIndexLocation;
    item.matWorld = MathHelper::Identity4x4();
    item.matTexTransform = MathHelper::Identity4x4();
    RenderItems[Opaque].push_back(item);

    item.nCBObjectIndex = 1;
    item.pMaterial = &Materials["brick"];
    item.nIndexCount = item.pGeo->DrawArgs["wall"].nIndexCount;
    item.nBaseVertexLocation = item.pGeo->DrawArgs["wall"].nBaseVertexLocation;
    item.nStartIndexLocation = item.pGeo->DrawArgs["wall"].nStartIndexLocation;
    RenderItems[Opaque].push_back(item);

    item.nCBObjectIndex = 2;
    item.pMaterial = &Materials["skull"];
    item.nIndexCount = item.pGeo->DrawArgs["skull"].nIndexCount;
    item.nBaseVertexLocation = item.pGeo->DrawArgs["skull"].nBaseVertexLocation;
    item.nStartIndexLocation = item.pGeo->DrawArgs["skull"].nStartIndexLocation;
    RenderItems[Opaque].push_back(item);
    skull = &RenderItems[Opaque][RenderItems[Opaque].size() - 1];

    item.nCBObjectIndex = 3;
    RenderItems[Reflected].push_back(item);
    reflectedSkull = &RenderItems[Reflected][RenderItems[Reflected].size() - 1];

    item.nCBObjectIndex = 4;
    item.pMaterial = &Materials["skullShadow"];
    RenderItems[Shadow].push_back(item);
    shadowSkull = &RenderItems[Shadow][RenderItems[Shadow].size() - 1];

    item.nCBObjectIndex = 5;
    item.pMaterial = &Materials["iceMirror"];
    item.nIndexCount = item.pGeo->DrawArgs["mirror"].nIndexCount;
    item.nBaseVertexLocation = item.pGeo->DrawArgs["mirror"].nBaseVertexLocation;
    item.nStartIndexLocation = item.pGeo->DrawArgs["mirror"].nStartIndexLocation;
    RenderItems[Mirror].push_back(item);
    RenderItems[Transparent].push_back(item);

    for(UINT i = 0; i < nFrameResourceCount; ++i)
        pFrameResources[i].InitConstantBuffer(pD3dDevice.Get(), 2, 6);
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
    UpdateCamera(t);
    UpdateScene(t);
    UpdateMaterials(t);
    UpdateObjects(t);
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

    // Normal
    D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBScene.Resource()->GetGPUVirtualAddress();
    pCommandList->SetGraphicsRootConstantBufferView(2, addr);
    DrawItems(RenderItems[Opaque]);

    // Mirror 
    pCommandList->OMSetStencilRef(1);
    pCommandList->SetPipelineState(PipelineStates[Mirror].Get());
    DrawItems(RenderItems[Mirror]);

    // Reflected
    pCommandList->SetGraphicsRootConstantBufferView(2, addr + CONSTANT_VALUE::nCBSceneByteSize);
    pCommandList->SetPipelineState(PipelineStates[Reflected].Get());
    DrawItems(RenderItems[Reflected]);

    // Transparent
    pCommandList->SetGraphicsRootConstantBufferView(2, addr);
    pCommandList->OMSetStencilRef(0);
    pCommandList->SetPipelineState(PipelineStates[Transparent].Get());
    DrawItems(RenderItems[Transparent]);

    // Shadow
    pCommandList->SetPipelineState(PipelineStates[Shadow].Get());
    DrawItems(RenderItems[Shadow]);

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

void D3DFrame::DrawItems(std::vector<RenderItem>& items)
{
    for(UINT i = 0; i < items.size(); ++i)
    {
        RenderItem item = items[i];
        
        pCommandList->IASetPrimitiveTopology(item.emPrimitiveType);
        pCommandList->IASetVertexBuffers(0, 1, &item.pGeo->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&item.pGeo->GetIndexBufferView());

        // Objects
        D3D12_GPU_VIRTUAL_ADDRESS addr = pCurrFrameResource->CBObject.Resource()->GetGPUVirtualAddress();
        addr += item.nCBObjectIndex * CONSTANT_VALUE::nCBObjectByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(0, addr);

        // Materials
        addr = pCurrFrameResource->CBMaterial.Resource()->GetGPUVirtualAddress();
        addr += item.pMaterial->nCBMaterialIndex * CONSTANT_VALUE::nCBMaterialByteSize;
        pCommandList->SetGraphicsRootConstantBufferView(1, addr);

        // Textures
        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(item.pMaterial->nDiffuseSrvHeapIndex, nCbvSrvUavDescriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(3, handle);
        
        pCommandList->DrawIndexedInstanced(item.nIndexCount, 1, item.nStartIndexLocation, item.nBaseVertexLocation, 0);
    }
}

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices)
{
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pEnd, *pBlock;
    UINT iVertexCounter = 0, iIndexCounter = 0;
    UINT j;

//****** Read File
    if((hFile = CreateFileA("C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Models/skull.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return;
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = (char*)malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

    vertices.resize(31076);
    indices.resize(60339);
//****** Load Data
    pBlock = pFileBuffer;

    // vertex
    while(*pBlock++ != '{');

    while(*pBlock != '}' || *pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9') || *pBlock == '-')){++pBlock;}
        // 一组默认有 6 个浮点数
        for(j = 0; j < 6; ++j)
        {
            pBegin = pBlock;
            while(((*pBlock != ' ') && (*pBlock != '\r'))){++pBlock;}
            pEnd = pBlock++;      // ' '
            *pEnd = '\0';

            vertices[iVertexCounter].v[j] = atof(pBegin);
        }
        ++iVertexCounter;
        if(iVertexCounter == 31076)
            break;
    }

    // indices
    while(*pBlock++ != '{');

    while(*pBlock)
    {
        while(!((*pBlock >= '0' && *pBlock <= '9'))){++pBlock;}

        for(j = 0; j < 3; ++j)
        {
            pBegin = pBlock;
            while(((*pBlock != ' ') && (*pBlock != '\r'))){++pBlock;}
            pEnd = pBlock++;      // ' '
            *pEnd = '\0';

            indices[iIndexCounter].i[j] = atoi(pBegin);
        }
        
        ++iIndexCounter;
        if(iIndexCounter == 60339)
            break;
    }
    free(pFileBuffer);
}

void D3DFrame::BuildPipelineStates()
{
    ComPtr<ID3D12PipelineState> pPipelineState;
    D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

    ComPtr<ID3DBlob> pVsByteCode;
    ComPtr<ID3DBlob> pPsByteCode;

    pVsByteCode = CompileShader(L"shader.hlsl", defines, "VsMain", "vs_5_0");
    pPsByteCode = CompileShader(L"shader.hlsl", defines, "PsMain", "ps_5_0");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC normal = {};
    normal.InputLayout = {inputLayouts, _countof(inputLayouts)};
    normal.pRootSignature = pRootSignature.Get();
    normal.VS = {pVsByteCode->GetBufferPointer(), pVsByteCode->GetBufferSize()};
    normal.PS = {pPsByteCode->GetBufferPointer(), pPsByteCode->GetBufferSize()};
    normal.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    normal.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    normal.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    normal.NumRenderTargets = 1;
    normal.RTVFormats[0] = emBackBufferFormat;
    normal.DSVFormat = emDepthStencilFormat;
    normal.SampleMask = UINT_MAX;
    normal.SampleDesc.Count = 1;
    normal.SampleDesc.Quality = 0;
    normal.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&normal, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Opaque] = pPipelineState;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC trans;
    D3D12_RENDER_TARGET_BLEND_DESC transBlendDesc;
    transBlendDesc.BlendEnable = 1;
    transBlendDesc.LogicOpEnable = 0;
    transBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    trans = normal;
    trans.BlendState.RenderTarget[0] = transBlendDesc;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&trans, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Transparent] = pPipelineState;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC mirror;
    D3D12_DEPTH_STENCIL_DESC mirrorDSDesc;
    mirrorDSDesc.DepthEnable = 1;
    mirrorDSDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    mirrorDSDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    mirrorDSDesc.StencilEnable = 1;
    mirrorDSDesc.StencilWriteMask = 0xff;
    mirrorDSDesc.StencilReadMask = 0xff;

    mirrorDSDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    mirrorDSDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    mirrorDSDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    mirrorDSDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    
    mirror = normal;
    mirror.BlendState.RenderTarget->RenderTargetWriteMask = 0;
    mirror.DepthStencilState = mirrorDSDesc;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&mirror, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Mirror] = pPipelineState;

    // Reflected
    D3D12_GRAPHICS_PIPELINE_STATE_DESC reflected;
    D3D12_DEPTH_STENCIL_DESC reflectedDSDesc;
    reflectedDSDesc.DepthEnable = 1;
    reflectedDSDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    reflectedDSDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    reflectedDSDesc.StencilEnable = 1;
    reflectedDSDesc.StencilWriteMask = 0xff;
    reflectedDSDesc.StencilReadMask = 0xff;

    reflectedDSDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

    reflectedDSDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    reflectedDSDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    
    reflected = normal;
    reflected.DepthStencilState = reflectedDSDesc;
    reflected.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    reflected.RasterizerState.FrontCounterClockwise = 1;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&reflected, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Reflected] = pPipelineState;

    // Shadow
    D3D12_DEPTH_STENCIL_DESC shadowDDS;
    shadowDDS.DepthEnable = 1;
    shadowDDS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    shadowDDS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    shadowDDS.StencilEnable = 1;
    shadowDDS.StencilWriteMask = 0xff;
    shadowDDS.StencilReadMask = 0xff;

    shadowDDS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDDS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDDS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
    shadowDDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

    shadowDDS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDDS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDDS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
    shadowDDS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadow = trans;
    shadow.DepthStencilState = shadowDDS;
    ThrowIfFailed(pD3dDevice->CreateGraphicsPipelineState(&shadow, IID_PPV_ARGS(&pPipelineState)));
    PipelineStates[Shadow] = pPipelineState;

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
    sc.vec4FogColor = {0.7f, 0.7f, 0.7f, 0.5f};
    sc.fFogStart = 5.0f;
    sc.fFogRange = 150.0f;
    sc.lights[0].vec3Direction = { 0.57735f, -0.57735f, 0.57735f };
    sc.lights[0].vec3Strength = { 0.6f, 0.6f, 0.6f };
    sc.lights[1].vec3Direction = { -0.57735f, -0.57735f, 0.57735f };
    sc.lights[1].vec3Strength = { 0.3f, 0.3f, 0.3f };
    sc.lights[2].vec3Direction = { 0.0f, -0.707f, -0.707f };
    sc.lights[2].vec3Strength = {0.15f, 0.15f, 0.15f};

    pCurrFrameResource->CBScene.CopyData(0, &sc,CONSTANT_VALUE::nCBSceneByteSize);

    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);

    for(UINT i = 0; i < 3; ++i)
    {
        XMVECTOR lightDir = XMLoadFloat3(&sc.lights[i].vec3Direction);
        XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&sc.lights[i].vec3Direction, reflectedLightDir);
    }

    pCurrFrameResource->CBScene.CopyData(1, &sc, CONSTANT_VALUE::nCBSceneByteSize);
}

void D3DFrame::InputProcess(const GameTimer& t)
{
    const float dt = t.DeltaTime();

    if(GetAsyncKeyState('A') & 0x8000)
        skullTranslation.x -= 1.0f * dt;
    if(GetAsyncKeyState('D') & 0x8000)
        skullTranslation.x += 1.0f * dt;
    if(GetAsyncKeyState('W') & 0x8000)
        skullTranslation.y += 1.0f * dt;
    if(GetAsyncKeyState('S') & 0x8000)
        skullTranslation.y -= 1.0f * dt;
    
    skullTranslation.y = max(skullTranslation.y, 0.0f);

    XMMATRIX RSkull = XMMatrixRotationY(0.5f * XM_PI);
    XMMATRIX SSkull = XMMatrixScaling(0.45f, 0.45f, 0.45f);
    XMMATRIX OSkull = XMMatrixTranslation(skullTranslation.x, skullTranslation.y, skullTranslation.z);
    XMMATRIX WSkull = RSkull * SSkull * OSkull;
    XMStoreFloat4x4(&skull->matWorld, WSkull);

    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);
    XMStoreFloat4x4(&reflectedSkull->matWorld, WSkull * R);

    XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR mainLight = -XMVectorSet(0.57735f, -0.57735f, 0.57735f, 0.0f);
    XMMATRIX S = XMMatrixShadow(shadowPlane, mainLight);
    S = S * XMMatrixTranslation(0.0f, 0.001f, 0.0f);
    XMStoreFloat4x4(&shadowSkull->matWorld, WSkull * S);

    skull->iFramesDirty = 3;
    reflectedSkull->iFramesDirty = 3;
    shadowSkull->iFramesDirty = 3;
}