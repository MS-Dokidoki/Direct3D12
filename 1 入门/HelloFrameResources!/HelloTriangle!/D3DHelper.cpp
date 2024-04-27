#include "D3DHelper.h"

/************************/
/** Exception */
DXException::DXException(HRESULT hr)
{
    wsprintf(e, TEXT("hr: %ld"), hr);
}

DXException::DXException(HRESULT hr, const char* file, unsigned int line)
{
    TCHAR buffer[DXEXCEPTION_MAXSTRING] = { 0 };

#if defined(UNICODE)
    MultiByteToWideChar(CP_ACP, 0, file, -1, buffer, DXEXCEPTION_MAXSTRING);
#endif
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d"), hr, buffer, line);
}

DXException::DXException(HRESULT hr, ID3DBlob** error, const char* file, unsigned int line)
{
    TCHAR buffer[DXEXCEPTION_MAXSTRINGEX] = {0};
    UINT errorBegin;

#ifdef UNICODE
    errorBegin = MultiByteToWideChar(CP_ACP, 0, file, -1, buffer, DXEXCEPTION_MAXSTRINGEX);
    if(error)
    {
        errorBegin += 2;
        MultiByteToWideChar(CP_ACP, 0, (char*)(*error)->GetBufferPointer(), -1, &buffer[errorBegin], DXEXCEPTION_MAXSTRINGEX - errorBegin);
    }
#endif
    
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d\nlog: %s"), hr, buffer, line, buffer[errorBegin]);
}

DXException::~DXException(){}

TCHAR* DXException::ToString()
{
    return e;
}

/****************************************/
/** D3DHelper */
using namespace Microsoft::WRL;
using namespace D3DHelper;

Microsoft::WRL::ComPtr<ID3DBlob> D3DHelper::CompileShader(LPCWSTR lpShaderFile, const D3D_SHADER_MACRO* pDefines, const char* pEntryPoint, const char* pTarget)
{
    UINT uiCompileFlags = 0;
#ifdef _DEBUG
    uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> pByteCode;
    ComPtr<ID3DBlob> pError;
    ThrowIfFailedEx(D3DCompileFromFile(lpShaderFile, pDefines, NULL, pEntryPoint, pTarget, uiCompileFlags, 0, &pByteCode, &pError), &pError);

    return pByteCode;
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3DHelper::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pComList, 
    const void* pData, UINT nByteSize, ID3D12Resource** pUploader)
{
    ComPtr<ID3D12Resource> pDefaultBuffer;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nByteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL, IID_PPV_ARGS(&pDefaultBuffer)
    ));

    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nByteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(pUploader)
    ));

    void* pBegin;
    (*pUploader)->Map(0, NULL, &pBegin);
    memcpy(pBegin, pData, nByteSize);
    (*pUploader)->Unmap(0, NULL);

    pComList->CopyResource(pDefaultBuffer.Get(), *pUploader);
    pComList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        pDefaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
    ));

    return pDefaultBuffer;
}

UploadBuffer::UploadBuffer()
{

}

UploadBuffer::UploadBuffer(ID3D12Device* pDevice, UINT uiElementByteSize, UINT uiElementCount)
{
    Init(pDevice, uiElementByteSize, uiElementCount);
}

UploadBuffer::~UploadBuffer()
{
    if(pBufferBegin)
        pUploadBuffer->Unmap(0, NULL);
    pBufferBegin = NULL;
}
void UploadBuffer::Init(ID3D12Device* pDevice, UINT uiElementByteSize, UINT uiElementCount)
{
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uiElementByteSize * uiElementCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pUploadBuffer)
    ));
    ThrowIfFailed(pUploadBuffer->Map(0, NULL, (void**)&pBufferBegin));
}

ID3D12Resource* UploadBuffer::Resource() const
{
    return pUploadBuffer.Get();
}

void UploadBuffer::CopyData(int iElementIndex, const void* pData, const UINT uiByteSize)
{
    memcpy(&pBufferBegin[iElementIndex * uiElementByteSize], pData, uiByteSize);
}

/**************************************/
/** GeometryGenerator */
//* c: count; r: radius

using namespace Geometry;
using namespace DirectX;

Mesh GeometryGenerator::CreateCylinder(float rBottom, float rTop, float height, UINT cSlice, UINT cStack)
{
    Mesh mesh;

/* 构建顶点数据*/
    float hStack = height / cStack;     // 每个堆叠层的高度
    float rDiff = rTop - rBottom;       // 端面半径差
    float rStep = rDiff / cStack;       // 从下至上遍历每个堆叠层的半径增量
    UINT cRing = cStack + 1;           // 柱体侧面顶点环数量
    
    float theta = 2.0f * XM_PI / cSlice;

    for(UINT i = 0; i < cRing; ++i)
    {
        float y = -0.5f * height + i * hStack;   // 当前顶点环的 y 值
        float r = rBottom + i * rStep;          // 当前顶点环的半径

        for(UINT j = 0; j <= cSlice; ++j)
        {
            Vertex v;

            float cosX = cosf(j * theta);
            float sinZ = sinf(j * theta);

            v.vec3Position = XMFLOAT3(r * cosX, y, r * sinZ);
            v.vec2TexCoords = XMFLOAT2((float)j / cSlice, 1.0f - (float)i / cStack);

            v.vec3TangentU = XMFLOAT3(-cosX, 0.0f, sinZ);
            
            XMFLOAT3 bitangent(rDiff * cosX, -height, rDiff * sinZ);
            XMVECTOR T = XMLoadFloat3(&v.vec3TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&v.vec3Normal, N);

            mesh.vertices.push_back(v);
        }
    }

    /* 构建顶点索引*/
    UINT cRingVertex = cSlice + 1;

    for(UINT i = 0; i < cStack; ++i)
    {
        for(UINT j = 0; j < cSlice; ++j)
        {
            mesh.indices.push_back(i * cRingVertex + j);
            mesh.indices.push_back((i + 1) * cRingVertex + j);
            mesh.indices.push_back((i + 1) * cRingVertex + j + 1);
            mesh.indices.push_back(i * cRingVertex + j);
            mesh.indices.push_back((i + 1) * cRingVertex + j + 1);
            mesh.indices.push_back(i * cRingVertex + j + 1);
        }
    }

    /** 构建端面顶点和索引*/
    UINT iBase = mesh.vertices.size();
    float y = 0.5f * height;

    // 再创建顶面的顶点环，因为侧面的和顶面的纹理坐标和法线是不同的
    for(UINT i = 0; i <= cSlice; ++i)
    {
        float x = rTop * cosf(i * theta);
        float z = rTop * sinf(i * theta);

        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        mesh.vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    mesh.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    UINT iCenter = mesh.vertices.size() - 1;
    
    for(UINT i = 0; i < cSlice; ++i)
    {
        mesh.indices.push_back(iCenter);
        mesh.indices.push_back(iBase + i + 1);
        mesh.indices.push_back(iBase + i);
    }

    iBase = mesh.vertices.size();
    y = -0.5f * height;
    
    //TODO: 底面纹理坐标和法线还需要修改
    for(UINT i = 0; i <= cSlice; ++i)
    {
        float x = rBottom * cosf(i * theta);
        float z = rBottom * sinf(i * theta);

        float u = x / height - 0.5f;            
        float v = z / height - 0.5f;

        mesh.vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }
    mesh.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    iCenter = mesh.vertices.size() - 1;
    for(UINT i = 0; i < cSlice; ++i)
    {
        mesh.indices.push_back(iCenter);
        mesh.indices.push_back(iBase + i + 1);
        mesh.indices.push_back(iBase + i);
    }

    return mesh;
}

Mesh GeometryGenerator::CreateCube()
{
    Mesh mesh;
    Vertex v[24];
   
    v[0] = Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[1] = Vertex(-1.0f, +1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[2] = Vertex(+1.0f, +1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[3] = Vertex(+1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[4] = Vertex(-1.0f, -1.0f, +1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+1.0f, -1.0f, +1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+1.0f, +1.0f, +1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-1.0f, +1.0f, +1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[8]  = Vertex(-1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-1.0f, +1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+1.0f, +1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[12] = Vertex(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+1.0f, -1.0f, +1.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-1.0f, -1.0f, +1.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[16] = Vertex(-1.0f, -1.0f, +1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-1.0f, +1.0f, +1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-1.0f, +1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    v[20] = Vertex(+1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+1.0f, +1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+1.0f, +1.0f, +1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+1.0f, -1.0f, +1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    mesh.vertices.assign(&v[0], &v[24]);

    UINT16 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

    mesh.indices.assign(&i[0], &i[36]);

    return mesh;
}