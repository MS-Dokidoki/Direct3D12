#include "D3DHelper.h"

using namespace D3DHelper::Geometry;
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

Mesh GeometryGenerator::CreateSphere(float radius, UINT nSliceCount, UINT nStackCount)
{
    Mesh mesh;

    Vertex top(0.0f, radius, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    Vertex bottom(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    // 利用了三角函数的非线性原理
    float fThetaStep = XM_2PI / nSliceCount;    // 顶点环每个顶点的平均角度步长
    float fPhiStep = XM_PI / nStackCount;       // 顶点环与顶点环之间的平均角度步长
    
    mesh.vertices.push_back(top);
    for(UINT i = 1; i < nStackCount - 1; ++i)
    {
        float fPhi = i * fPhiStep;  
        for(UINT j = 0; j < nSliceCount; ++j)
        {
            float fTheta = j * fThetaStep;
            Vertex vertex;
            vertex.vec3Position = XMFLOAT3(radius * sinf(fPhi) * cosf(fTheta), radius * cosf(fPhi), radius * sinf(fPhi) * sinf(fTheta));
            vertex.vec3TangentU = XMFLOAT3(-radius * sinf(fPhi) * sinf(fTheta), 0.0f, radius * sinf(fPhi) * cosf(fTheta));
            vertex.vec2TexCoords = XMFLOAT2(fTheta / XM_2PI, fPhi / XM_PI);

            XMVECTOR T = XMLoadFloat3(&vertex.vec3TangentU);
            XMVECTOR P = XMLoadFloat3(&vertex.vec3Position);
            XMStoreFloat3(&vertex.vec3TangentU, XMVector3Normalize(T));
            XMStoreFloat3(&vertex.vec3Normal, XMVector3Normalize(P));

            mesh.vertices.push_back(vertex);
        }
    }
    mesh.vertices.push_back(bottom);

/*******************************/
// Indices
    for(UINT i = 0; i <= nSliceCount; ++i)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i);
    }

    UINT nBaseIndex = 1;
    UINT nRingVertexCount = nSliceCount + 1;
    for(UINT i = 0; i < nStackCount - 2; ++i)
    {
        for(UINT j = 0; j < nSliceCount; ++j)
        {
            mesh.indices.push_back(nBaseIndex + i * nRingVertexCount + j);
            mesh.indices.push_back(nBaseIndex + i * nRingVertexCount + j + 1);
            mesh.indices.push_back(nBaseIndex + (i + 1) * nRingVertexCount + j);

            mesh.indices.push_back(nBaseIndex + (i + 1) * nRingVertexCount + j);
            mesh.indices.push_back(nBaseIndex + i * nRingVertexCount + j + 1);
            mesh.indices.push_back(nBaseIndex + (i + 1) * nRingVertexCount + j + 1);
        }
    }

    UINT southPoleIndex = mesh.vertices.size() - 1;
    nBaseIndex = southPoleIndex - nRingVertexCount;
    for(UINT i = 0; i < nSliceCount; ++i)
    {
        mesh.indices.push_back(southPoleIndex);
        mesh.indices.push_back(nBaseIndex + i);
        mesh.indices.push_back(nBaseIndex + i + 1);
    }

    return mesh;
}