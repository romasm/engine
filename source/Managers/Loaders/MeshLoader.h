#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
#define MESH_FILE_VERSION 103
#define SKELETON_FILE_VERSION 101
#define ANIMATION_FILE_VERSION 103

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

	struct MeshBBox
	{
		Vector3 center;
		Vector3 extents;
	};

	struct MeshData
	{
		RArray<GPUMeshBuffer> vertexBuffers;
		RArray<GPUMeshBuffer> indexBuffers;

#ifdef _EDITOR
		RArray<uint8_t*> vertices;
		RArray<uint32_t*> indices;
#endif

		BoundingBox box;
		MeshVertexFormat vertexFormat;
		float maxVertexOffset;

		MeshData() : vertexFormat(MeshVertexFormat::LIT_VERTEX) {}

		~MeshData()
		{
			for(uint32_t i = 0; i < (uint32_t)vertexBuffers.size(); i++)
				_RELEASE(vertexBuffers[i].buffer);
			for(uint32_t i = 0; i < (uint32_t)indexBuffers.size(); i++)
				_RELEASE(indexBuffers[i].buffer);

#ifdef _EDITOR
			for (uint32_t i = 0; i < (uint32_t)vertices.size(); i++)
				_DELETE_ARRAY(vertices[i]);
			for (uint32_t i = 0; i < (uint32_t)indices.size(); i++)
				_DELETE_ARRAY(indices[i]);
#endif

			vertexBuffers.destroy();
			indexBuffers.destroy();
			
			box.Center = Vector3::Zero;
			box.Extents = Vector3::Zero;
			vertexFormat = MeshVertexFormat::LIT_VERTEX;
			maxVertexOffset = 0;
		}
	};
	
	struct MeshFileHeader
	{
		uint32_t version;

		uint32_t materialCount;
		MeshBBox bbox;
		float maxVertexOffset;
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
		
		inline static void Lerp(BoneTransformation& t1, BoneTransformation& t2, float alpha, BoneTransformation& res)
		{
			res.scale = XMVectorLerp(t1.scale, t2.scale, alpha);
			res.rotation = XMQuaternionSlerp(t1.rotation, t2.rotation, alpha);
			res.translation = XMVectorLerp(t1.translation, t2.translation, alpha);
		}

		inline void SetZero()
		{
			translation = XMVectorZero();
			rotation = XMVectorSet(0.0f,0.0f,0.0f,1.0f);
			scale = XMVectorSet(1.0f,1.0f,1.0f,1.0f);
		}
	};

	struct BoneAnimation
	{
		DArrayAligned<BoneTransformation> keys; // transform per keyframe
	};

	struct AnimationData
	{
		float duration;
		int32_t keysCountMinusOne;
		DArray<BoneAnimation> bones; // tracks per bone
		DArray<MeshBBox> bboxes; // box per keyframe
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
			unordered_map<string, int32_t>& boneIds, DArray<BoneData>& boneData, Vector3& posMin, Vector3& posMax, float& vertexOffset);
		
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
		
		inline Vector3 GetVertexPos(uint8_t* vertices, uint32_t index, MeshVertexFormat format)
		{
			switch (format)
			{
			case MeshVertexFormat::LIT_VERTEX:
				return ((LitVertex*)vertices)[index].Pos;
			case MeshVertexFormat::LIT_SKINNED_VERTEX:
				return ((LitSkinnedVertex*)vertices)[index].Pos;
			}
			return Vector3();
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
		Matrix res = Matrix::CreateScale(transf.scale);
		Matrix rotM = Matrix::CreateFromQuaternion(transf.rotation);
		res = res * rotM;
		res.Translation(transf.translation);
		return res;
	}
}