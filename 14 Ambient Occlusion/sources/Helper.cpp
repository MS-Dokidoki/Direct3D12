#include "Helper.h"
#define SKULL_MODE_PATH "C:/Users/Administrator.PC-20191006TRUC/source/repos/DirectX/Models/skull.txt"
#define IsDigit(c) (c >= '0' && c <= '9')

void GetStaticSampler(CD3DX12_STATIC_SAMPLER_DESC* sampler)
{
    sampler[0].Init(
        0, 
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );

    sampler[1].Init(
        1, 
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );

    sampler[2].Init(
        2, 
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );

    sampler[3].Init(
        3, 
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );

    sampler[4].Init(
        4, 
        D3D12_FILTER_ANISOTROPIC, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f, 8
    );

    sampler[5].Init(
        5, 
        D3D12_FILTER_ANISOTROPIC, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f, 8
    );

    sampler[6].Init(
        6, 
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
    );
}


void _BuildMaterials(std::vector<MaterialListItem>& MaterialList, 
                    std::vector<SrvListItem>& SrvList, 
                    std::unordered_map<std::string, Resource::Texture>& Textures, 
                    std::unordered_map<std::string, Light::Material>& Materials,
                    UINT& materialCounter,
                    UINT srvIndexCounter)
{
    for(auto& item: MaterialList)
    {
        Light::Material mat;
        mat.iFramesDirty = 3;
        mat.Name = item.Name;
        mat.matTransform = item.matTransform;
        mat.vec4DiffuseAlbedo = item.vec4DiffuseAlbedo;
        mat.vec3FresnelR0 = item.vec3FresnelR0;
        mat.nRoughness = item.nRoughness;
        mat.nCBMaterialIndex = materialCounter++;

        std::unordered_map<std::string, Resource::Texture>::iterator diffItor, normItor;
        SrvListItem sli;

        if(!item.bDiffIsNull)
        {
            diffItor = Textures.find(item.lpszDiffuseMap);
            if(diffItor != Textures.end())
            {
                mat.nDiffuseSrvHeapIndex= srvIndexCounter++;

                sli.ViewDimension = item.ViewDimension;
                sli.pResource = diffItor->second.pResource.Get();
                sli.ResourceDesc = sli.pResource->GetDesc();
                SrvList.push_back(sli);
            }
            else
                goto a;
        }
        else
        {
a:          mat.nDiffuseSrvHeapIndex = -1;
        }
        
        if(!item.bNormIsNull)
        {
            normItor = Textures.find(item.lpszNormalMap);
            if(normItor != Textures.end())
            {
                mat.nNormalSrvHeapIndex = srvIndexCounter++;

                sli.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                sli.pResource = normItor->second.pResource.Get();
                sli.ResourceDesc = sli.pResource->GetDesc();
                SrvList.push_back(sli);
            }
            else
                goto b;
        }
        else
b:          mat.nNormalSrvHeapIndex = -1;
        
        Materials[mat.Name] = mat;
    }

    MaterialList.clear();
}

void _BuildGeometries(ID3D12Device* device, 
                     ID3D12GraphicsCommandList* cmdList, 
                     std::unordered_map<std::string, Resource::MeshGeometry>& Geos, 
                     std::unordered_map<std::string, GeoListItem>& GeoList)
{
    for(auto& item: GeoList)
    {
        Resource::MeshGeometry geo;

        geo.Name = item.first;

        geo.pGPUVertexBuffer = CreateDefaultBuffer(device, cmdList, item.second.pVertices, item.second.nVertexByteSize, &geo.pUploaderVertexBuffer);
        geo.pGPUIndexBuffer = CreateDefaultBuffer(device, cmdList, item.second.pIndices, item.second.nIndexByteSize, &geo.pUploaderIndexBuffer);

        if(item.second.bAutoRelease)
        {
            free(item.second.pVertices);
            free(item.second.pIndices);
        }

        geo.nVertexBufferByteSize = item.second.nVertexByteSize;
        geo.nVertexByteStride = item.second.nVertexByteStride;
        geo.nIndexBufferByteSize = item.second.nIndexByteSize;
        geo.emIndexFormat = item.second.emIndexFormat;
        geo.DrawArgs = item.second.Submeshes;

        Geos[geo.Name] = geo;
    }

    GeoList.clear();
}

void _LoadTextures(ID3D12Device* device, 
                  ID3D12GraphicsCommandList* cmdList, 
                  std::vector<TextureListItem>& TextureList, 
                  std::unordered_map<std::string, Resource::Texture>& Textures)
{
    Resource::Texture tex;

    for(auto& item: TextureList)
    {
        tex.Name = item.Name;
        tex.FileName = item.FileName;
        tex.pResource = LoadDDSFromFile(device, cmdList, item.FileName.c_str(), &tex.pUploader);
        
        Textures[tex.Name] = tex;
    }

    TextureList.clear();
}

void _BuildSRV(ID3D12Device* device, 
              ID3D12DescriptorHeap* pSRVHeap, 
              std::vector<SrvListItem>& SrvList, 
              UINT descriptorSize)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = SrvList.size();
    desc.NodeMask = 0;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSRVHeap));

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pSRVHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    for(auto& item: SrvList)
    {
        if(!item.bSkip)
        {
            srv.ViewDimension = item.ViewDimension;
            srv.Format = item.ResourceDesc.Format == 0? DXGI_FORMAT_R8G8B8A8_UNORM: item.ResourceDesc.Format;
            
            switch(srv.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                srv.Texture2D.MipLevels = item.ResourceDesc.MipLevels;
                srv.Texture2D.MostDetailedMip = 0;
                srv.Texture2D.PlaneSlice = 0;
                srv.Texture2D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                srv.TextureCube.MipLevels = item.ResourceDesc.MipLevels;
                srv.TextureCube.MostDetailedMip = 0;
                srv.TextureCube.ResourceMinLODClamp = 0;
                break;
            }

            device->CreateShaderResourceView(item.pResource, &srv, handle);
        }
        handle.Offset(1, descriptorSize);
    }
}

void LoadSkullModel(std::vector<SkullModelVertex>& vertices, std::vector<SkullModelIndex>& indices)
{
    HANDLE hFile;
    UINT iFileLength;
    char* pFileBuffer;
    char* pBegin, *pBlock;
    UINT nVertexCounter = 0, nTriangleCounter = 0;

//****** Read File
    if((hFile = CreateFileA(SKULL_MODE_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return;
    
    iFileLength = GetFileSize(hFile, NULL);

    pFileBuffer = (char*)malloc(iFileLength + 2);
    ReadFile(hFile, pFileBuffer, iFileLength, NULL, NULL);
    CloseHandle(hFile);
    pFileBuffer[iFileLength] = '\0';
    pFileBuffer[iFileLength + 1] = '\0';

//****** Load Data
    pBlock = pFileBuffer;

    while(!IsDigit(*pBlock))++pBlock;
    pBegin = pBlock;
    while(*pBlock != '\r')++pBlock;
    *pBlock++ = '\0';

    nVertexCounter = atoi(pBegin);
    while(!IsDigit(*pBlock))++pBlock;
    pBegin = pBlock;
    while(*pBlock != '\r')++pBlock;
    *pBlock++ = '\0';
    
    nTriangleCounter = atoi(pBegin);
    
    vertices.resize(nVertexCounter);
    indices.resize(nTriangleCounter);

    while(*pBlock++ != '{');

    for(UINT i = 0; i < nVertexCounter; ++i)
    {
        for(UINT j = 0; j < 6; ++j)
        {
            while(!IsDigit(*pBlock) && *pBlock != '-')++pBlock;
            pBegin = pBlock;
            while(*pBlock != ' ' && *pBlock != '\r')++pBlock;
            *pBlock++ = '\0';

            vertices[i].v[j] = atof(pBegin);
        }
    }

    while(*pBlock++ != '\n');
    assert(*pBlock == '}');

    while(*pBlock++ != '{');
    for(UINT i = 0; i < nTriangleCounter; ++i)
    {
        for(UINT j = 0; j < 3; ++j)
        {
            while(!IsDigit(*pBlock))++pBlock;
            pBegin = pBlock;
            while(*pBlock != ' ' && *pBlock != '\r')++pBlock;
            *pBlock++ = '\0';
            indices[i].i[j] = atof(pBegin);
        }
    }

    free(pFileBuffer);
}   
