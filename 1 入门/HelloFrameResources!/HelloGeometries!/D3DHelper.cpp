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
    if(*error)
    {
        errorBegin += 1;
        MultiByteToWideChar(CP_ACP, 0, (char*)(*error)->GetBufferPointer(), -1, &buffer[errorBegin], DXEXCEPTION_MAXSTRINGEX - errorBegin);
    }
#endif
    
    wsprintf(e, TEXT("hr: %ld\nfile: %s\nline: %d\nlog: %s"), hr, buffer, line, &buffer[errorBegin]);
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

UploadBuffer::UploadBuffer(ID3D12Device* pDevice, UINT nElementByteSize, UINT nElementsCount)
{
    Init(pDevice, nElementByteSize, nElementsCount);
}

UploadBuffer::~UploadBuffer()
{
    if(pBufferBegin)
        pUploadBuffer->Unmap(0, NULL);
    pBufferBegin = NULL;
}
void UploadBuffer::Init(ID3D12Device* pDevice, UINT nByteSize, UINT nElementsCount)
{
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(nByteSize * nElementsCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, IID_PPV_ARGS(&pUploadBuffer)
    ));
    ThrowIfFailed(pUploadBuffer->Map(0, NULL, (void**)&pBufferBegin));

    nElementByteSize = nByteSize;
}

ID3D12Resource* UploadBuffer::Resource() const
{
    return pUploadBuffer.Get();
}

void UploadBuffer::CopyData(int iElementIndex, const void* pData, const UINT uiByteSize)
{
    memcpy(&pBufferBegin[iElementIndex * nElementByteSize], pData, uiByteSize);
}

/**************************************/
/** GeometryGenerator */
//* c: count; r: radius

using namespace Geometry;
using namespace DirectX;

Mesh GeometryGenerator::CreateCylinder(float rBottom, float rTop, float height, UINT nSliceCount, UINT nStackCount)
{
	Mesh mesh;

    float fStackHeight = height / nStackCount;
    float fRadiusDifference = rTop - rBottom;
    float fRadiusStep = fRadiusDifference / nStackCount;

    UINT nRingCount = nStackCount + 1;

    for(UINT i = 0; i < nRingCount; ++i)
    {
        float y = -0.5f * height + i * fStackHeight;
        float r = rBottom + i * fRadiusStep;

        float fTheta = XM_2PI / nSliceCount;
        for(UINT j = 0; j <= nSliceCount; ++j)
        {
            Vertex vertex;
            float nCos = cosf(j * fTheta);
            float nSin = sinf(j * fTheta);

            vertex.vec3Position = XMFLOAT3(r * nCos, y, r * nSin);
            vertex.vec2TexCoords = XMFLOAT2(j /nSliceCount, 1.0f - i / nStackCount);
            vertex.vec3TangentU = XMFLOAT3(-nSin, 0.0f, nCos);

            XMFLOAT3 bitangent((-fRadiusDifference) * nCos, -height, (-fRadiusDifference) * nSin);

            XMVECTOR T = XMLoadFloat3(&vertex.vec3TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.vec3Normal, N);

            mesh.vertices.push_back(vertex);
        }
    }

    UINT nRingVertexCount = nSliceCount + 1;
    for(UINT i = 0; i < nStackCount; ++i)
    {
        for(UINT j = 0; j < nSliceCount; ++j)
        {
            mesh.indices.push_back(i * nRingVertexCount + j);
            mesh.indices.push_back((i + 1) * nRingVertexCount + j);
            mesh.indices.push_back((i + 1) * nRingVertexCount + j + 1);

            mesh.indices.push_back(i * nRingVertexCount + j);
            mesh.indices.push_back((i + 1) * nRingVertexCount + j + 1);
            mesh.indices.push_back(i * nRingVertexCount + j + 1);
        }
    }

/***************************/
// Build top vertices and bottom's
    UINT nBaseIndex = mesh.vertices.size();
    float fHalfHeight = 0.5f * height;
    float fTheta = XM_2PI / nSliceCount;

    for(UINT i = 0; i <= nSliceCount; ++i)
    {
        float x = rTop * cosf(fTheta * i);
        float z = rTop * sinf(fTheta * i);

        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        mesh.vertices.push_back(Vertex(x, fHalfHeight, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    mesh.vertices.push_back(Vertex(0.0f, fHalfHeight, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    UINT nCentreIndex = mesh.vertices.size() - 1;
    for(UINT i = 0; i < nSliceCount; ++i)
    {
        mesh.indices.push_back(nCentreIndex);
        mesh.indices.push_back(nBaseIndex + i + 1);
        mesh.indices.push_back(nBaseIndex + i);
    }

    nBaseIndex = mesh.vertices.size();
    fHalfHeight = -0.5f * height;

    for(UINT i = 0; i <= nSliceCount; ++i)
    {
        float x = rBottom * cosf(i * fTheta);
        float z = rBottom * sinf(i * fTheta);

        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        mesh.vertices.push_back(Vertex(x, fHalfHeight, z, 0.0, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    mesh.vertices.push_back(Vertex(0.0f, fHalfHeight, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
    nCentreIndex = mesh.vertices.size() - 1;

    for(UINT i =  0; i < nSliceCount; ++i)
    {
        mesh.indices.push_back(nCentreIndex);
        mesh.indices.push_back(nCentreIndex + i);
        mesh.indices.push_back(nCentreIndex + i + 1);
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