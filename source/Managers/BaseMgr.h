#pragma once
#include "stdafx.h"
#include "Common.h"
#include "ResourceProcessor.h"

#define RESOURCE_MAX_COUNT 65536
#define RESOURCE_INIT_COUNT 1024
#define RESOURCE_NULL RESOURCE_MAX_COUNT

namespace EngineCore
{
	template<DataType>
	class BaseMgr
	{
	public:
		BaseMgr();
		~BaseMgr();
		
		inline static BaseMgr* Get(){return instance;}
		
		static string& GetName(uint32_t id)
		{
			if(id == RESOURCE_NULL) return null_name;
			return instance->resource_array[id].name;
		}

		uint32_t GetResource(string& name, bool reload = false, onLoadCallback callback = nullptr);
		void DeleteResource(uint32_t id);
		void DeleteResourceByName(string& name);
		
		inline static DataType* GetResourcePtr(uint32_t id)
		{
			if(id == RESOURCE_NULL) return null_resource;
			return instance->resource_array[id].tex;
		}
		
		void OnPostLoadMainThread(uint32_t id, onLoadCallback func, LoadingStatus status);
		void OnLoad(uint32_t id, DataType* data);

		void CheckForReload();

	private:
		uint32_t AddResourceToList(string& name, bool reload, onLoadCallback callback);
		uint32_t FindResourceInList(string& name);

		static BaseMgr *instance;

		DataType* null_resource;
		string null_name;
		ResourceType resType;

		struct ResourceHandle
		{
			DataType* resource;
			uint32_t refcount;
			uint32_t filedate;
			string name;

			ResourceHandle()
			{
				resource = nullptr;
				refcount = 0;
				filedate = 0;
			}
		};

		unordered_map<string, uint32_t> resource_map;
		
		SArray<ResourceHandle, RESOURCE_MAX_COUNT> resource_array;
		SDeque<uint32_t, RESOURCE_MAX_COUNT> free_ids;
	};
}