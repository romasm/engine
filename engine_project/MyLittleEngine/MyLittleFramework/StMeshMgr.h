#pragma once
#include "stdafx.h"
#include "Common.h"

#define STMESH_MAX_COUNT 16384
#define STMESH_INIT_COUNT 1024
#define STMESH_NULL STMESH_MAX_COUNT

#define MAT_MESH PATH_SYS_MESHES "mat_sphere" EXT_STATIC

namespace EngineCore
{
	struct StMeshData
	{
		uint16_t matCount;

		uint32_t* vertexCount;
		ID3D11Buffer **vertexBuffer;

		uint32_t* indexCount;
		ID3D11Buffer **indexBuffer;

		BoundingBox box;

		StMeshData()
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

	class StMeshMgr
	{
	public:
		StMeshMgr();
		~StMeshMgr();
		
		inline static StMeshMgr* Get(){return instance;}

		uint32_t GetStMesh(string& name, bool reload = false);
		void DeleteStMesh(uint32_t id);
		void DeleteStMeshByName(string& name);
		
		void PreloadStMeshes();

		inline static StMeshData* GetStMeshPtr(uint32_t id)
		{
			if(id == STMESH_NULL) return null_mesh;
			return instance->mesh_array[id].mesh;
		}

		static string& GetStMeshName(uint32_t id)
		{
			if(id == STMESH_NULL) return null_name;
			return instance->mesh_array[id].name;
		}

		void UpdateStMeshes();

		inline bool IsJustReloaded(uint32_t id) 
		{
			if(id == STMESH_NULL)
				return false;
			bool res = mesh_reloaded[id] > 0;
			if(res)
				mesh_reloaded[id]--;
			return res;
		}

		inline void SaveSTMFile(string& file) {loadNoNativeFile(file, true);}

	private:
		static StMeshMgr *instance;
		static StMeshData* null_mesh;
		static string null_name;

		struct StMeshHandle
		{
			StMeshData* mesh;
			uint32_t refcount;
			uint32_t filedate;
			string name;

			StMeshHandle()
			{
				mesh = nullptr;
				refcount = 0;
				filedate = 0;
			}
		};
		
		unordered_map<string, uint32_t> mesh_map;
		
		SArray<StMeshHandle, STMESH_MAX_COUNT> mesh_array;
		SDeque<uint32_t, STMESH_MAX_COUNT> mesh_free;

		SArray<uint32_t, STMESH_MAX_COUNT> mesh_reloaded;

		uint32_t AddStMeshToList(string& name, bool reload);
		uint32_t FindStMeshInList(string& name);

		StMeshData* loadSTMFile(string& name);
		
		Assimp::Importer m_importer;
		StMeshData* loadNoNativeFile(string& filename, bool onlyConvert = false);
		StMeshData* loadAIScene(string& filename, const aiScene* scene, bool convert, bool noLoad);

		void convertToSTM(string& filename, StMeshData* mesh, uint32_t** indices, LitVertex** vertices);
	};
}