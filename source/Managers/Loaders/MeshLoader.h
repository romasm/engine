#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
#define MESH_FILE_VERSION 101

	enum MeshVertexFormat 
	{
		LIT_VERTEX = 0,
		LIT_SKINNED_VERTEX,
	};

	struct GPUMeshBuffer
	{
		// It's count of elements, NOT byte size
		uint32_t count;
		ID3D11Buffer* buffer;

		GPUMeshBuffer() : count(0), buffer(nullptr) {}
	};

	struct MeshData
	{
		RArray<GPUMeshBuffer> vertexBuffers;
		RArray<GPUMeshBuffer> indexBuffers;

		BoundingBox box;
		MeshVertexFormat vertexFormat;

		MeshData() : vertexFormat(MeshVertexFormat::LIT_VERTEX) {}

		~MeshData()
		{
			for(uint32_t i = 0; i < (uint32_t)vertexBuffers.size(); i++)
				_RELEASE(vertexBuffers[i].buffer);
			for(uint32_t i = 0; i < (uint32_t)indexBuffers.size(); i++)
				_RELEASE(indexBuffers[i].buffer);

			vertexBuffers.destroy();
			indexBuffers.destroy();
			
			box = BoundingBox();
			vertexFormat = MeshVertexFormat::LIT_VERTEX;
		}
	};
	
	struct MeshFileHeader
	{
		uint32_t version;

		uint32_t materialCount;
		Vector3 bboxCenter;
		Vector3 bboxExtents;

		MeshVertexFormat vertexFormat;
	};

	namespace MeshLoader
	{		
		static Assimp::Importer meshImporter;
		
		MeshData* LoadMesh(string& resName);
		MeshData* loadEngineMeshFromMemory(string& filename, uint8_t* data, uint32_t size);
				
		bool ConvertMeshToEngineFormat(string& sourceFile, string& resFile, bool isSkinned);
		bool IsSupported(string filename);

		bool saveMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices);
		bool convertAIScene(string& filename, const aiScene* scene, MeshVertexFormat format);
		void loadVerticesLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, Vector3& posMin, Vector3& posMax);
		void loadVerticesSkinnedLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, int32_t& boneOffset, Vector3& posMin, Vector3& posMax);
		
		inline uint32_t GetVertexSize(MeshVertexFormat format)
		{
			switch (format)
			{
			case MeshVertexFormat::LIT_VERTEX:
				return sizeof(LitVertex);
			case MeshVertexFormat::LIT_SKINNED_VERTEX:
				return 0;
			}
			return 0;
		}
	};
}