#pragma once
#ifndef _D3DHELPER_GEOMETRYGENERATOR_H
#define _D3DHELPER_GEOMETRYGENERATOR_H
#include "D3DBase.h"

namespace D3DHelper
{
    namespace Geometry
    {
        /// @brief �����嶥�����ݽṹ��
        struct Vertex
        {
            Vertex() {}
            Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v) : vec3Position(px, py, pz), vec3Normal(nx, ny, nz), vec3TangentU(tx, ty, tz), vec2TexCoords(u, v) {}
            Vertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT3 tangentU, DirectX::XMFLOAT2 texCoords) : vec3Position(position), vec3Normal(normal), vec3TangentU(tangentU), vec2TexCoords(texCoords) {}
    
            DirectX::XMFLOAT3 vec3Position;  // ��������
            DirectX::XMFLOAT3 vec3Normal;    // ���㷨��
            DirectX::XMFLOAT3 vec3TangentU;  // ��������
            DirectX::XMFLOAT2 vec2TexCoords; // ��������

        };

        /// @brief �������������ݽṹ��
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
		
        /// @brief �������������ɾ�̬��
        class GeometryGenerator
        {
        public:
            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ�����
            /// @param bottomRadius ����뾶
            /// @param topRadius    ����뾶
            /// @param height       �߶�
            /// @param nSliceCount  ��Ƭ����
            /// @param nStackCount  �ѵ�����
            /// @return
            static Mesh CreateCylinder(float bottomRadius, float topRadius, float height, UINT nSliceCount, UINT nStackCount);

            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ�����
            /// @param radius       ����뾶
            /// @param nSliceCount  ��Ƭ����
            /// @param nStackCount  �ѵ�����
            /// @return
            static Mesh CreateSphere(float radius, UINT nSliceCount, UINT nStackCount);

            /// @brief �����Ծֲ�����ϵԭ��Ϊ���ĵ� x-z դ��ƽ��
            /// @param fHorizonalLength ���򳤶�( x �᷽��)
            /// @param fVerticalLength ���򳤶�( z �᷽��)
            /// @param nHoriGridCount ����դ������
            /// @param nVertGridCount ����դ������
            /// @return
            static Mesh CreateGrid(float fHorizonalLength, float fVerticalLength, UINT nHoriGridCount, UINT nVertGridCount);
        
            static Mesh CreateCube(float fWidth, float fHeight, float fDepth);

            static Mesh CreateQuad(float x, float y, float w, float h, float depth);
        };
    };
};

#endif