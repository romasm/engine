#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
#define MESH_FILE_VERSION 101
#define SKELETON_FILE_VERSION 101
#define ANIMATION_FILE_VERSION 102

#define ANIMATION_BAKE_MAX_KPS 120 // keys per second
#define ANIMATION_BAKE_MIN_KPS 10

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

	struct AnimationFileHeader
	{
		uint32_t version;
		uint32_t boneCount;
	};

	struct NodeInfo
	{
		aiMatrix4x4 transform;
		string parent;
	};

	struct BoneTransformation
	{
		XMVECTOR translation;
		XMVECTOR rotation;
		XMVECTOR scale;
	};

	struct BoneAnimation
	{
		DArrayAligned<BoneTransformation> keys;
	};

	struct AnimationData
	{
		float duration;
		int32_t keysCount;
		DArray<BoneAnimation> bones;
	};

	namespace MeshLoader
	{		
		static Assimp::Importer meshImporter;
		void Configurate();

		MeshData* LoadMesh(string& resName);
		MeshData* loadEngineMeshFromMemory(string& filename, uint8_t* data, uint32_t size);

		// TODO: load only cashed bones(sokets) for game
		SkeletonData* LoadSkeleton(string& resName);
		SkeletonData* loadEngineSkeletonFromMemory(string& filename, uint8_t* data, uint32_t size);

		AnimationData* LoadAnimation(string& resName);
		AnimationData* loadEngineAnimationFromMemory(string& filename, uint8_t* data, uint32_t size);

		bool ConvertSkeletonToEngineFormat(string& sourceFile, string& resFile);
		bool ConvertMeshToEngineFormat(string& sourceFile, string& resFile, bool isSkinned);
		bool ConverAnimationToEngineFormat(string& sourceFile, string& resFile);

		bool IsNative(string filename);
		bool IsSupported(string filename);

		bool IsNativeSkeleton(string filename);
		bool IsSupportedSkeleton(string filename);

		bool IsNativeAnimation(string filename);
		bool IsSupportedAnimation(string filename);

		bool saveMesh(string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices);
		bool saveSkeleton(string& filename, DArray<BoneData>& boneData, unordered_map<string, int32_t>& boneIds);
		bool saveAnimation(string& filename, DArray<AnimationData>& animations);

		bool convertAIScene(string& filename, const aiScene* scene, MeshVertexFormat format);
		bool convertAnimationAIScene(string& filename, const aiScene* scene);

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
	};

	inline Matrix aiMatrix4x4ToMatrix(aiMatrix4x4& mat)
	{
		aiVector3D scale;
		aiVector3D pos;
		aiQuaternion rot;
		mat.Decompose(scale, rot, pos);

		Matrix res = Matrix::CreateScale(scale.x, scale.y, scale.z);
		Matrix rotM = Matrix::CreateFromQuaternion(Quaternion(rot.x, rot.y, rot.z, rot.w));
		res = res * rotM;
		res.Translation(Vector3(pos.x, pos.y, pos.z));

		return res;
	}

	inline Vector3 aiVector3DToVector3(aiVector3D& v)
	{
		return Vector3(v.x, v.y, v.z);
	}

	inline Quaternion aiQuaternionToQuaternion(aiQuaternion& q)
	{
		return Quaternion(q.x, q.y, q.z, q.w);
	}

	inline BoneTransformation MatrixToBoneTransformation(Matrix& m)
	{
		Vector3 scale, pos;
		Quaternion rot;
		m.Decompose(scale, rot, pos);
		
		BoneTransformation transf;
		transf.scale = scale;
		transf.rotation = rot;
		transf.translation = pos;
		return transf;
	}

	inline Matrix BoneTransformationToMatrix(BoneTransformation& transf)
	{

	}
}