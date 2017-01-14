#pragma once
#include "stdafx.h"
#include "Common.h"

#define SKELETAL_MESH_MAX_COUNT 16384
#define SKELETAL_MESH_INIT_COUNT 1024
#define SKELETAL_MESH_NULL SKELETAL_MESH_MAX_COUNT

#define MAT_MESH PATH_SYS_MESHES "mat_sphere" EXT_STATIC

namespace EngineCore
{
	struct SkeletalMeshData
	{
		uint16_t matCount;

		uint32_t* vertexCount;
		ID3D11Buffer **vertexBuffer;

		uint32_t* indexCount;
		ID3D11Buffer **indexBuffer;

		BoundingBox box;

		SkeletalMeshData()
		{
			vertexBuffer = nullptr;
			indexBuffer = nullptr;
			indexCount = nullptr;
			vertexCount = nullptr;
			matCount = 0;
		}

		inline void Close()
		{
			if(vertexBuffer)
				for(int i = 0; i < matCount; i++)
					_RELEASE(vertexBuffer[i]);
			_DELETE_ARRAY(vertexBuffer);
			_DELETE_ARRAY(vertexCount);

			if(indexBuffer)
				for(int i = 0; i < matCount; i++)
					_RELEASE(indexBuffer[i]);
			_DELETE_ARRAY(indexBuffer);
			_DELETE_ARRAY(indexCount);
			
			matCount = 0;
			box = BoundingBox();
		}
	};

	class SkeletalMeshMgr
	{
	public:
		SkeletalMeshMgr();
		~SkeletalMeshMgr();
		
		inline static SkeletalMeshMgr* Get(){return instance;}

		uint32_t GetSkeletalMesh(string& name, bool reload = false);
		void DeleteSkeletalMesh(uint32_t id);
		void DeleteSkeletalMeshByName(string& name);
		
		void PreloadSkeletalMeshes();

		inline static SkeletalMeshData* GetSkeletalMeshPtr(uint32_t id)
		{
			if(id == SKELETAL_MESH_NULL) return null_mesh;
			return instance->mesh_array[id].mesh;
		}

		static string& GetSkeletalMeshName(uint32_t id)
		{
			if(id == SKELETAL_MESH_NULL) return null_name;
			return instance->mesh_array[id].name;
		}

		void UpdateSkeletalMeshes();

		inline bool IsJustReloaded(uint32_t id) 
		{
			if(id == SKELETAL_MESH_NULL)
				return false;
			bool res = mesh_reloaded[id] > 0;
			if(res)
				mesh_reloaded[id]--;
			return res;
		}

		inline void SaveSTMFile(string& file) {loadNoNativeFile(file, true);}

	private:
		static SkeletalMeshMgr *instance;
		static SkeletalMeshData* null_mesh;
		static string null_name;

		struct SkeletalMeshHandle
		{
			SkeletalMeshData* mesh;
			uint32_t refcount;
			uint32_t filedate;
			string name;

			SkeletalMeshHandle()
			{
				mesh = nullptr;
				refcount = 0;
				filedate = 0;
			}
		};
		
		unordered_map<string, uint32_t> mesh_map;
		
		SArray<SkeletalMeshHandle, SKELETAL_MESH_MAX_COUNT> mesh_array;
		SDeque<uint32_t, SKELETAL_MESH_MAX_COUNT> mesh_free;

		SArray<uint32_t, SKELETAL_MESH_MAX_COUNT> mesh_reloaded;

		uint32_t AddSkeletalMeshToList(string& name, bool reload);
		uint32_t FindSkeletalMeshInList(string& name);

		SkeletalMeshData* loadSTMFile(string& name);
		
		Assimp::Importer m_importer; // TODO: one global, mesh loading locked for one thread

		SkeletalMeshData* loadNoNativeFile(string& filename, bool onlyConvert = false);
		SkeletalMeshData* loadAIScene(string& filename, const aiScene* scene, bool convert, bool noLoad);

		void convertToSTM(string& filename, SkeletalMeshData* mesh, uint32_t** indices, LitVertex** vertices);
	};
}