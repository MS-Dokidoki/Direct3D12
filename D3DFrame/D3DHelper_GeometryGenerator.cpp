#include "D3DHelper_GeometryGenerator.h"

using namespace D3DHelper::Geometry;
using namespace DirectX;

Vertex Midpoint(const Vertex* v1, const Vertex* v2)
{
    XMVECTOR p1 = XMLoadFloat3(&v1->vec3Position);
    XMVECTOR p2 = XMLoadFloat3(&v2->vec3Position);

    XMVECTOR t1 = XMLoadFloat2(&v1->vec2TexCoords);
    XMVECTOR t2 = XMLoadFloat2(&v2->vec2TexCoords);
    
    XMVECTOR ta1 = XMLoadFloat3(&v1->vec3TangentU);
    XMVECTOR ta2 = XMLoadFloat3(&v2->vec3TangentU);

    XMVECTOR n1 = XMLoadFloat3(&v1->vec3Normal);
    XMVECTOR n2 = XMLoadFloat3(&v2->vec3Normal);

    Vertex v;
    XMStoreFloat3(&v.vec3Position,  0.5f * (p1 + p2));
    XMStoreFloat3(&v.vec3TangentU,  XMVector3Normalize(0.5f * (ta1 + ta2)));
    XMStoreFloat3(&v.vec3Normal, XMVector3Normalize(0.5f * (n1 + n2)));
    XMStoreFloat2(&v.vec2TexCoords  ,0.5f * (t1 + t2));

    return v;
}

void Subdivide(Mesh& mesh)
{
    Mesh copy = mesh;

    mesh.indices.resize(0);
    mesh.vertices.resize(0);

    UINT num = copy.indices.size() / 3;
    for(UINT i = 0; i < num; ++i)
    {
        Vertex v0 = copy.vertices[copy.indices[i * 3 + 0]];
        Vertex v1 = copy.vertices[copy.indices[i * 3 + 1]];
        Vertex v2 = copy.vertices[copy.indices[i * 3 + 2]];

        Vertex m0 = Midpoint(&v0, &v1);
        Vertex m1 = Midpoint(&v1, &v2);
        Vertex m2 = Midpoint(&v0, &v2);

        mesh.vertices.push_back(v0);
        mesh.vertices.push_back(v1);
        mesh.vertices.push_back(v2);
        mesh.vertices.push_back(m0);
        mesh.vertices.push_back(m1);
        mesh.vertices.push_back(m2);
        
        mesh.indices.push_back(i*6+0);
		mesh.indices.push_back(i*6+3);
		mesh.indices.push_back(i*6+5);

		mesh.indices.push_back(i*6+3);
		mesh.indices.push_back(i*6+4);
		mesh.indices.push_back(i*6+5);

		mesh.indices.push_back(i*6+5);
		mesh.indices.push_back(i*6+4);
		mesh.indices.push_back(i*6+2);

		mesh.indices.push_back(i*6+3);
		mesh.indices.push_back(i*6+1);
		mesh.indices.push_back(i*6+4);
    }
}
Mesh GeometryGenerator::CreateCube(float width, float height, float depth)
{
    Mesh mesh;

    Vertex v[24];

	float w2 = 0.5f*width;
	float h2 = 0.5f*height;
	float d2 = 0.5f*depth;
    
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
    v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    mesh.vertices.assign(&v[0], &v[24]);

    UINT16 i[36];

	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;
    
    mesh.indices.assign(&i[0], &i[36]);
	
    return mesh;
}

Mesh GeometryGenerator::CreateCylinder(float rBottom, float rTop, float height, UINT nSliceCount, UINT nStackCount)
{
    Mesh mesh;

    float stackHeight = height / nStackCount;
    float radiusStep = (rTop - rBottom) / nStackCount;
    UINT ringCount = nStackCount + 1;

    for(UINT i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = rBottom + i * radiusStep;

        float dTheta = 2.0f * XM_PI / nSliceCount;
        for(UINT j = 0; j <= nSliceCount; ++j)
        {
            Vertex vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.vec3Position = XMFLOAT3(r * c, y, r * s);
            vertex.vec2TexCoords = XMFLOAT2((float)j / nSliceCount, 1.0f - (float)i / nStackCount);
            vertex.vec3TangentU = XMFLOAT3(-s, 0.0f, c);
            
            float dr = rBottom - rTop;
            XMFLOAT3 bitangent(dr * c, -height, dr * s);

            XMVECTOR T = XMLoadFloat3(&vertex.vec3TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
            XMStoreFloat3(&vertex.vec3Normal, N);

            mesh.vertices.push_back(vertex);
        }
    }

    UINT ringVertexCount = nSliceCount + 1;
    for(UINT i = 0; i < nStackCount; ++i)
    {
        for(UINT j = 0; j < nSliceCount; ++j)
        {
            mesh.indices.push_back(i * ringVertexCount + j);
            mesh.indices.push_back((i + 1) * ringVertexCount + j);
            mesh.indices.push_back((i + 1) * ringVertexCount + (j + 1));
            mesh.indices.push_back(i * ringVertexCount + j);
            mesh.indices.push_back((i + 1) * ringVertexCount + j + 1);
            mesh.indices.push_back(i * ringVertexCount + j + 1);
            
        }
    }
    {
        UINT baseIndex = mesh.vertices.size();
        
        float y = 0.5f * height;
        float dTheta = 2.0f * XM_PI / nSliceCount;

        for(UINT i = 0; i <= nSliceCount; ++i)
        {
            float x = rTop * cosf(i * dTheta);
            float z = rTop * sinf(i * dTheta);

            float u = x / height + 0.5f;
            float v = z / height + 0.5f;

            mesh.vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
        }

        mesh.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

        UINT centerIndex = mesh.vertices.size() - 1;

        for(UINT i = 0; i < nSliceCount; ++i)
        {
            mesh.indices.push_back(centerIndex);
            mesh.indices.push_back(baseIndex + i + 1);
            mesh.indices.push_back(baseIndex + i);
        }
    }
    {
        UINT baseIndex = mesh.vertices.size();

        float y = -0.5f * height;
        float dTheta = 2.0f * XM_PI / nSliceCount;
        for(UINT i = 0; i <= nSliceCount; ++i)
        {
            float x = rBottom * cosf(i*dTheta);
            float z = rBottom *sinf(i*dTheta);

            float u = x/height + 0.5f;
            float v = z/height + 0.5f;

            mesh.vertices.push_back( Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
        }

        mesh.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
    
        UINT centerIndex = mesh.vertices.size() - 1;

        for(UINT i = 0; i < nSliceCount; ++i)
        {
            mesh.indices.push_back(centerIndex);
            mesh.indices.push_back(baseIndex + i);
            mesh.indices.push_back(baseIndex + i + 1);
        }
    }
    return mesh;

}

Mesh GeometryGenerator::CreateSphere(float radius, UINT sliceCount, UINT stackCount)
{
    Mesh mesh;

    Vertex top(0.0f, radius, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    Vertex bottom(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;
    
    mesh.vertices.push_back(top);
    for(UINT i = 1; i <= stackCount - 1; ++i)
    {
        float phi = i * phiStep;
        for(UINT j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            Vertex v;

            v.vec3Position = XMFLOAT3(radius * sinf(phi) * cosf(theta), radius * cosf(phi), radius * sinf(phi) * sinf(theta));
            v.vec3TangentU = XMFLOAT3(-radius * sinf(phi) * sinf(theta), 0.0f, radius * sinf(phi) * cosf(theta));
            
            XMVECTOR T = XMLoadFloat3(&v.vec3TangentU);
            XMStoreFloat3(&v.vec3TangentU, XMVector3Normalize(T));
            XMVECTOR P = XMLoadFloat3(&v.vec3Position);
            XMStoreFloat3(&v.vec3Normal, XMVector3Normalize(P));

            v.vec2TexCoords = XMFLOAT2(theta / XM_2PI, phi / XM_PI);
            mesh.vertices.push_back(v);
        }
    }
    mesh.vertices.push_back(bottom);
    
/*******************************/
// Indices
    
    for(UINT i = 1; i <= sliceCount; ++i)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i + 1);
        mesh.indices.push_back(i);
    }

    UINT baseIndex = 1;
    UINT ringVertexCount = sliceCount + 1;
    for(UINT i = 0; i < stackCount - 2; ++i)
    {
        for(UINT j = 0; j < sliceCount; ++j)
        {
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    UINT southIndex = mesh.vertices.size() - 1;
    baseIndex = southIndex - ringVertexCount;

    for(UINT i = 0; i < sliceCount; ++i)
    {
        mesh.indices.push_back(southIndex);
        mesh.indices.push_back(baseIndex + i);
        mesh.indices.push_back(baseIndex + i + 1);
    }

    return mesh;
}

Mesh GeometryGenerator::CreateGrid(float width, float depth, UINT m, UINT n)
{
    Mesh mesh;

    UINT nVertexCount = m * n;
    UINT nFaceCount = (m - 1) * (n - 1) * 2;

    float nHalfWidth = 0.5f * width;
    float nHalfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);

    float du = 1.0f / (n - 1);
    float dv = 1.0f / (m - 1);

    mesh.vertices.resize(nVertexCount);
    for(UINT i = 0; i < m; ++i)
    {
        float z = nHalfDepth - i * dz;
        for(UINT j = 0; j < n; ++j)
        {
            float x = -nHalfWidth + j * dx;

            mesh.vertices[i * n + j].vec3Position = XMFLOAT3(x, 0.0f, z);
            mesh.vertices[i * n + j].vec3Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            mesh.vertices[i * n + j].vec3TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
            mesh.vertices[i * n + j].vec2TexCoords = XMFLOAT2(j * du, i * dv); 
        }
    }

    mesh.indices.resize(nFaceCount * 3);
    UINT k = 0;
    for(UINT i = 0; i < m - 1; ++i)
    {
        for(UINT j = 0; j < n - 1; ++j)
        {
            mesh.indices[k] = i * n + j;
            mesh.indices[k + 1] = i * n + j + 1;
            mesh.indices[k + 2] = (i + 1) * n + j;

            mesh.indices[k + 3] = (i + 1) * n + j;
            mesh.indices[k + 4] = i * n + j + 1;
            mesh.indices[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }
    return mesh;
}

Mesh GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
    Mesh mesh;
    mesh.vertices.resize(4);
    mesh.indices.resize(6);

    mesh.vertices[0] = Vertex(
        x, y - h, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f         
    );

    mesh.vertices[1] = Vertex(
        x, y, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f
    );

    mesh.vertices[2] = Vertex(
        x + w, y, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f
    );
    
    mesh.vertices[3] = Vertex(
        x + w, y - h, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f
    );

    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;
    mesh.indices[3] = 0;
    mesh.indices[4] = 2;
    mesh.indices[5] = 3;
    
    return mesh;
}