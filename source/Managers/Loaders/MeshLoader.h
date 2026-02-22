#pragma once
#include "stdafx.h"
#include "Common.h"
#include "Render\RHI\RHITypes.h"

namespace EngineCore
{
#define MESH_FILE_VERSION 104
#define SKELETON_FILE_VERSION 101
#define ANIMATION_FILE_VERSION 103

#define ANIMATION_BAKE_MAX_KPS 120 // keys per second
#define ANIMATION_BAKE_MIN_KPS 10

	// Upper bounds matching D3D12 mesh shader hardware limits.
	// Each meshlet fits in 64 unique vertices and 84 triangles (D3D12 max is 256/512).
	static constexpr uint32_t MESHLET_MAX_VERTICES   = 64;
	static constexpr uint32_t MESHLET_MAX_PRIMITIVES = 84;

	// Per-meshlet metadata written into the .msh.imp file (version 104+).
	struct Meshlet
	{
		uint32_t vertexOffset;    // first entry in uniqueVertexIndices for this meshlet
		uint32_t vertexCount;     // number of unique vertices used by this meshlet
		uint32_t primitiveOffset; // first entry in packedPrimitiveIndices for this meshlet
		uint32_t primitiveCount;  // number of triangles in this meshlet
	};

	// Per-submesh meshlet data stored alongside vertex/index buffers.
	struct MeshletSubset
	{
		DArray<Meshlet>  meshlets;
		// remapped vertex indices into the submesh vertex buffer
		DArray<uint32_t> uniqueVertexIndices;
		// triangle list, packed as (v2<<20)|(v1<<10)|v0 with 10-bit local vertex indices
		DArray<uint32_t> packedPrimitiveIndices;

		void destroy()
		{
			meshlets.destroy();
			uniqueVertexIndices.destroy();
			packedPrimitiveIndices.destroy();
		}
	};

	enum MeshVertexFormat
	{
		LIT_VERTEX = 0,
		LIT_SKINNED_VERTEX,
	};

	struct GPUMeshBuffer
	{
		// It's count of elements, NOT byte size
		uint32_t count;
		RHI::GfxBuffer* buffer;

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

		// Per-submesh meshlet data for the mesh shader pipeline (version 104+).
		// Populated when loading version-104 .msh.imp files and during conversion.
		RArray<MeshletSubset> meshletSubsets;

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
				_DELETE(vertexBuffers[i].buffer);
			for(uint32_t i = 0; i < (uint32_t)indexBuffers.size(); i++)
				_DELETE(indexBuffers[i].buffer);

			for(uint32_t i = 0; i < (uint32_t)meshletSubsets.size(); i++)
				meshletSubsets[i].destroy();
			meshletSubsets.destroy();

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

		MeshData* LoadMesh(const string& resName);
		MeshData* loadEngineMeshFromMemory(const string& filename, uint8_t* data, uint32_t size);

		// TODO: load only cashed bones(sokets) for game
		SkeletonData* LoadSkeleton(const string& resName);
		SkeletonData* loadEngineSkeletonFromMemory(const string& filename, uint8_t* data, uint32_t size);

		AnimationData* LoadAnimation(const string& resName);
		AnimationData* loadEngineAnimationFromMemory(const string& filename, uint8_t* data, uint32_t size);

		bool ConvertSkeletonToEngineFormat(const string& sourceFile, const string& resFile);
		bool ConvertMeshToEngineFormat(const string& sourceFile, const string& resFile, bool isSkinned);
		bool ConverAnimationToEngineFormat(const string& sourceFile, const string& resFile);

		bool IsNative(string filename);
		bool IsSupported(string filename);

		bool IsNativeSkeleton(string filename);
		bool IsSupportedSkeleton(string filename);

		bool IsNativeAnimation(string filename);
		bool IsSupportedAnimation(string filename);

		bool saveMesh(const string& filename, MeshData* mesh, uint32_t** indices, uint8_t** vertices);
		bool saveSkeleton(const string& filename, DArray<BoneData>& boneData, unordered_map<string, int32_t>& boneIds);
		bool saveAnimation(const string& filename, DArray<AnimationData>& animations);

		// Partition a triangle mesh into meshlets with at most MESHLET_MAX_VERTICES
		// unique vertices and MESHLET_MAX_PRIMITIVES triangles each.
		void GenerateMeshlets(const uint32_t* indices, uint32_t indexCount,
		                      uint32_t vertexCount, MeshletSubset& outSubset);

		bool convertAIScene(const string& filename, const aiScene* scene, MeshVertexFormat format);
		bool convertAnimationAIScene(const string& filename, const aiScene* scene);

		bool loadMeshSkeleton(const string& filename, const aiScene* scene, unordered_map<string, int32_t>& boneIds,
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