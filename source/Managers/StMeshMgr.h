#pragma once
#include "stdafx.h"
#include "Common.h"

#define STMESH_MAX_COUNT 16384
#define STMESH_INIT_COUNT 1024
#define STMESH_NULL STMESH_MAX_COUNT

#define MAT_MESH PATH_SYS_MESHES "mat_sphere" EXT_STATIC

namespace EngineCore
{
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

		inline static MeshData* GetStMeshPtr(uint32_t id)
		{
			if(id == STMESH_NULL) return null_mesh;
			return instance->mesh_array[id].mesh;
		}

		static string& GetName(uint32_t id)
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

	private:
		static StMeshMgr *instance;
		static MeshData* null_mesh;
		static string null_name;

		struct StMeshHandle
		{
			MeshData* mesh;
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
	};
}