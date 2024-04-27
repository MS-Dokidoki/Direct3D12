#pragma once
#ifndef _D3DHELPER_GEOMETRYGENERATOR_H
#define _D3DHELPER_GEOMETRYGENERATOR_H
#include "D3DBase.h"

namespace D3DHelper
{
    namespace Geometry
    {
        /// @brief 几何体顶点数据结构体
        struct Vertex
        {
            Vertex() {}
            Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) : vec3Position(px, py, pz), vec3Normal(nx, ny, nz), vec3TangentU(tx, ty, tz), vec2TexCoords(u, v) {}
            Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangentU, DirectX::XMFLOAT2 texCoords) : vec3Position(position), vec3Normal(normal), vec3TangentU(tangentU), vec2TexCoords(texCoords) {}
    
            DirectX::XMFLOAT3 vec3Position;  // 顶点坐标
            DirectX::XMFLOAT3 vec3Normal;    // 顶点法线
            DirectX::XMFLOAT3 vec3TangentU;  // 切线向量
            DirectX::XMFLOAT2 vec2TexCoords; // 纹理坐标

        };

        /// @brief 几何体网格数据结构体
        struct Mesh
        {
            std::vector<Vertex> vertices;
            std::vector<UINT16> indices;
        };
		
		struct Mesh32
		{
			std::vector<Vertex> vertices;
			std::vector<UINT32> indices;
		};
		
        /// @brief 几何体网格生成静态类
        class GeometryGenerator
        {
        public:
            /// @brief 生成以局部坐标系原点为中心的柱体
            /// @param bottomRadius 底面半径
            /// @param topRadius    顶面半径
            /// @param height       高度
            /// @param nSliceCount  切片数量
            /// @param nStackCount  堆叠数量
            /// @return
            static Mesh CreateCylinder(float bottomRadius, float topRadius, float height, UINT nSliceCount, UINT nStackCount);

            /// @brief 生成以局部坐标系原点为中心的球体
            /// @param radius       球体半径
            /// @param nSliceCount  切片数量
            /// @param nStackCount  堆叠数量
            /// @return
            static Mesh CreateSphere(float radius, UINT nSliceCount, UINT nStackCount);

            /// @brief 生成以局部坐标系原点为中心的 x-z 栅格平面
            /// @param fHorizonalLength 横向长度( x 轴方向)
            /// @param fVerticalLength 纵向长度( z 轴方向)
            /// @param nHoriGridCount 横向栅格数量
            /// @param nVertGridCount 纵向栅格数量
            /// @return
            static Mesh CreateGrid(float fHorizonalLength, float fVerticalLength, UINT nHoriGridCount, UINT nVertGridCount);
        
            static Mesh CreateCube(float fWidth, float fHeight, float fDepth);

            static Mesh CreateQuad(float x, float y, float w, float h, float depth);
        };
    };
};

#endif