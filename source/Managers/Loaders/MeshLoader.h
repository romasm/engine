#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
#define MESH_FILE_VERSION 101
#define SKELETON_FILE_VERSION 101

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

	struct BoneData
	{
		int32_t parent;
		Matrix localTransform;

		BoneData() : parent(-1) {}
	};

	// TODO: load only cashed bones(sokets) for game
	struct SkeletonData
	{
		RArray<BoneData> bData;
		unordered_map<string, int32_t> bIDs;
	};

	struct SkeletonFileHeader
	{
		uint32_t version;
		uint32_t boneCount;
	};

	struct NodeInfo
	{
		aiMatrix4x4 transform;
		string parent;
	};

	namespace MeshLoader
	{		
		static Assimp::Importer meshImporter;
		
		MeshData* LoadMesh(string& resName);
		MeshData* loadEngineMeshFromMemory(string& filename, uint8_t* data, uint32_t size);

		// TODO: load only cashed bones(sokets) for game
		SkeletonData* LoadSkeleton(string& resName);
		SkeletonData* loadEngineSkeletonFromMemory(string& filename, uint8_t* data, uint32_t size);

		bool ConvertSkeletonToEngineFormat(string& sourceFile, string& resFile);
		bool ConvertMeshToEngineFormat(string& sourceFile, string& resFile, bool isSkinned);

		bool IsNative(string filename);
		bool IsSupported(string filename);

		bool IsNativeSkeleton(string filename);
		bool IsSupportedSkeleton(string filename);

		bool saveMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices);
		bool saveSkeleton(string& filename, DArray<BoneData>& boneData, unordered_map<string, int32_t>& boneIds);

		bool convertAIScene(string& filename, const aiScene* scene, MeshVertexFormat format);
		bool convertAISceneSkeleton(string& filename, const aiScene* scene);

		bool loadMeshSkeleton(string& filename, const aiScene* scene, unordered_map<string, int32_t>& boneIds, 
			DArray<BoneData>& boneData, DArray<int32_t>& boneInvRemap, bool boneInvWorldTransforms);

		void loadVerticesLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, Vector3& posMin, Vector3& posMax);
		void loadVerticesSkinnedLit(uint8_t* data, uint32_t count, uint32_t vertexSize, aiMesh* mesh, 
			unordered_map<string, int32_t>& boneIds, DArray<BoneData>& boneData, Vector3& posMin, Vector3& posMax);
		
		inline uint32_t GetVertexSize(MeshVertexFormat format)
		{
			switch (format)
			{
			case MeshVertexFormat::LIT_VERTEX:
				return sizeof(LitVertex);
			case MeshVertexFormat::LIT_SKINNED_VERTEX:
				return sizeof(LitSkinnedVertex);
			}
			return 0;
		}

		inline bool IsSkinned(MeshVertexFormat format)
		{
			return format >= LIT_SKINNED_VERTEX;
		}

		inline Matrix aiMatrix4x4ToMatrix(aiMatrix4x4& mat)
		{
			aiVector3D scale;
			aiVector3D pos;
			aiQuaternion rot;
			mat.Decompose(scale, rot, pos);

			Matrix res;
			res.CreateScale(scale.x, scale.y, scale.z);
			Matrix rotM;
			rotM.CreateFromQuaternion(Quaternion(rot.x, rot.y, rot.z, rot.w));
			res = res * rotM;
			res.Translation(Vector3(pos.x, pos.y, pos.z));

			return res;
		}
	};
}