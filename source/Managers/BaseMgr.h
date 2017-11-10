#pragma once
#include "stdafx.h"
#include "Common.h"
#include "ResourceProcessor.h"

#define RESOURCE_MAX_COUNT 16384

namespace EngineCore
{
	template<typename DataType, uint32_t MaxCount = RESOURCE_MAX_COUNT>
	class BaseMgr
	{
	public:
		BaseMgr();
		void Close();
		
		inline static BaseMgr* Get(){return instance;}
		
		static string& GetName(uint32_t id)
		{
			if(id == nullres) return instance->null_name;
			return instance->resource_array[id].name;
		}

		uint32_t GetResource(string& name, bool reload = false, onLoadCallback callback = nullptr);
		void DeleteResource(uint32_t id);
		void DeleteResourceByName(string& name);
		
		inline static DataType* GetResourcePtr(uint32_t id)
		{
			if(id == nullres) return instance->null_resource;
			return instance->resource_array[id].resource;
		}

		inline static bool IsNull(DataType* resource) {return resource == instance->null_resource;}
		
		void DefferedDeallocate();

		void OnPostLoadMainThread(uint32_t id, onLoadCallback func, LoadingStatus status);
		void CallCallback(uint32_t id, onLoadCallback func, LoadingStatus status);
		void OnLoad(uint32_t id, DataType* data, ImportInfo& info, uint32_t& date);

		void CheckForReload();

		static const uint32_t nullres = MaxCount;

		virtual void ResourceDeallocate(DataType*& resource)
		{
			_DELETE(resource);
		};

		virtual uint32_t AddResourceToList(string& name, bool reload, onLoadCallback callback);
		uint32_t FindResourceInList(string& name);

		static BaseMgr *instance;

		DataType* null_resource;
		string null_name;
		ResourceType resType;
		char* resExt;

		struct ResourceHandle
		{
			DataType* resource;
			uint32_t refcount;
			string name;

#ifdef _EDITOR
			ReloadingType reloadStatus;
			uint32_t filedate;
			ImportInfo impInfo;
#endif

			ResourceHandle()
			{
				resource = nullptr;
				refcount = 0;
#ifdef _EDITOR
				reloadStatus = ReloadingType::RELOAD_NONE;
				filedate = 0;
				ZeroMemory(&impInfo, sizeof(impInfo));
#endif
			}
		};

		unordered_map<string, uint32_t> resource_map;
		
		SArray<ResourceHandle, MaxCount> resource_array;
		SDeque<uint32_t, MaxCount> free_ids;

		DArray<DataType*> deallocationQueue;
	};


	template<typename DataType, uint32_t MaxCount>
	BaseMgr<DataType, MaxCount>* BaseMgr<DataType, MaxCount>::instance = nullptr;
	
	template<typename DataType, uint32_t MaxCount>
	BaseMgr<DataType, MaxCount>::BaseMgr()
	{
		if(!instance)
		{
			instance = this;

			resource_array.resize(MaxCount);
			free_ids.resize(MaxCount);
			for(uint32_t i = 0; i < MaxCount; i++)
				free_ids[i] = i;

			uint32_t smallSize = max<uint32_t>( MaxCount / 16, 16 );
			resource_map.reserve( smallSize );
			deallocationQueue.reserve( smallSize );

			null_resource = nullptr;
			null_name = "";

			resType = ResourceType(0);
		}
		else
			ERR("Only one instance of Manager is allowed!");
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::Close()
	{
		DefferedDeallocate();

		for(uint32_t i=0; i<MaxCount; i++)
		{
			if(resource_array[i].resource != null_resource)
				ResourceDeallocate(resource_array[i].resource);
			resource_array[i].name.erase();
		}
		ResourceDeallocate(null_resource);
		null_name.clear();

		instance = nullptr;
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::DefferedDeallocate()
	{
		for(auto& data: deallocationQueue)
		{
			if(data != null_resource)
				ResourceDeallocate(data);
		}
		deallocationQueue.resize(0);
	}

	template<typename DataType, uint32_t MaxCount>
	uint32_t BaseMgr<DataType, MaxCount>::GetResource(string& name, bool reload, onLoadCallback callback)
	{
		uint32_t res = nullres;
		if( name.length() == 0 )
		{
			CallCallback(res, callback, LoadingStatus::FAILED);
			return res;
		}

		if( resExt != nullptr && name.find(resExt) == string::npos )
		{
			ERR("Wrong resource name %s", name.c_str());
			CallCallback(res, callback, LoadingStatus::FAILED);
			return res;
		}

		res = FindResourceInList(name);
		if(res != nullres)
		{
			const LoadingStatus status = (resource_array[res].resource == null_resource) ? LoadingStatus::NEW : LoadingStatus::LOADED;

			if( status == LoadingStatus::NEW )
				ResourceProcessor::Get()->QueueLoad(res, resType, callback, true);
			else
				CallCallback(res, callback, status);

			return res;
		}

		res = AddResourceToList(name, reload, callback);
		if(res != nullres)
			return res;

		ERR("Cant load resource %s", name.c_str());
		CallCallback(res, callback, LoadingStatus::FAILED);
		return res;
	}

	template<typename DataType, uint32_t MaxCount>
	uint32_t BaseMgr<DataType, MaxCount>::AddResourceToList(string& name, bool reload, onLoadCallback callback)
	{
		if(free_ids.size() == 0)
		{
			ERR("Resources amount overflow!");
			return nullres;
		}

		uint32_t idx = free_ids.front();
		auto& handle = resource_array[idx];

		handle.name = name;
		handle.resource = null_resource;
		handle.refcount = 1;

		if(!FileIO::IsExist(name))
		{
#ifdef _EDITOR
			WRN("File %s doesn\'t exist, creation expected in future.", name.data());
			if(reload)
				handle.reloadStatus = ReloadingType::RELOAD_ALWAYS;
			else
				handle.reloadStatus = ReloadingType::RELOAD_ONCE;
			handle.filedate = 1;

			ResourceProcessor::Get()->QueueLoad(idx, resType, callback);
#else
			ERR("File %s doesn\'t exist", name.data());
#endif
		}
		else
		{
#ifdef _EDITOR
			if(reload)
				handle.reloadStatus = ReloadingType::RELOAD_ALWAYS;
			else
				handle.reloadStatus = ReloadingType::RELOAD_NONE;
			handle.filedate = 0;
#endif
			ResourceProcessor::Get()->QueueLoad(idx, resType, callback);
		}

		resource_map.insert(make_pair(name, idx));
		free_ids.pop_front();

		return idx;
	}

	template<typename DataType, uint32_t MaxCount>
	uint32_t BaseMgr<DataType, MaxCount>::FindResourceInList(string& name)
	{
		auto it = resource_map.find(name);
		if(it == resource_map.end())
			return nullres;

		auto& handle = resource_array[it->second];
		handle.refcount++;
		return it->second;
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::DeleteResource(uint32_t id)
	{
		if(id == nullres)
			return;
		
		auto& handle = resource_array[id];

		if(handle.refcount == 1)
		{
			if(handle.resource != null_resource)
			{
				// TODO: thread safe move deallocation in background
				// deallocationQueue.push_back(handle.resource); // not thread safe
				ResourceDeallocate(handle.resource);
				LOG("Resource droped %s", handle.name.c_str());
			}
			else
			{
				handle.resource = nullptr;
			}

			handle.refcount = 0;

#ifdef _EDITOR
			handle.filedate = 0;
			handle.reloadStatus = ReloadingType::RELOAD_NONE;
			ZeroMemory(&handle.impInfo, sizeof(handle.impInfo));
#endif

			free_ids.push_back(id);

			resource_map.erase(handle.name);

			handle.name.clear();
		}
		else if(handle.refcount == 0)
		{
			WRN("Resource %s has already deleted!", handle.name.c_str());
		}
		else
			handle.refcount--;
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::DeleteResourceByName(string& name)
	{
		if(name.length() == 0)
			return;

		auto it = resource_map.find(name);
		if(it == resource_map.end())
			return;

		DeleteResource(it->second);
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::OnPostLoadMainThread(uint32_t id, onLoadCallback func, LoadingStatus status)
	{
		CallCallback(id, func, status);
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::CallCallback(uint32_t id, onLoadCallback func, LoadingStatus status)
	{
		if(func)
			func(id, status == LOADED);
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::OnLoad(uint32_t id, DataType* data, ImportInfo& info, uint32_t& date)
	{
		auto& handle = resource_array[id];

		auto oldResource = handle.resource;
		handle.resource = data;
		if(oldResource != null_resource)
			deallocationQueue.push_back(oldResource);

#ifdef _EDITOR
		handle.impInfo = info;
		if( handle.filedate == 0 || handle.filedate == 1 )
			handle.filedate = date;

		if( handle.reloadStatus == ReloadingType::RELOAD_ONCE )
			handle.reloadStatus = ReloadingType::RELOAD_NONE;
#endif
	}

	template<typename DataType, uint32_t MaxCount>
	void BaseMgr<DataType, MaxCount>::CheckForReload()
	{
#ifdef _EDITOR
		auto it = resource_map.begin();
		while(it != resource_map.end())
		{
			auto& handle = resource_array[it->second];

			if( handle.reloadStatus == ReloadingType::RELOAD_NONE || handle.filedate == 0 )
			{
				it++;
				continue;
			}
			
			if( handle.reloadStatus != ReloadingType::RELOAD_ONCE )
			{
				uint32_t last_date;
				if( handle.impInfo.filePath.empty() )
					last_date = FileIO::GetDateModifRaw((string&)it->first);
				else
					last_date = FileIO::GetDateModifRaw(handle.impInfo.filePath);

				if( last_date == handle.filedate || last_date == 0 )
				{	
					it++;
					continue;
				}
				handle.filedate = last_date;
			}

			if( handle.impInfo.filePath.empty() || handle.impInfo.filePath == it->first )
			{
				ResourceProcessor::Get()->QueueLoad(it->second, resType, nullptr);
			}
			else
			{
				auto rt = resType;
				uint32_t resId = it->second;

				ResourceProcessor::Get()->QueueImport(handle.impInfo,
					[rt, resId](const ImportInfo& info, bool status) -> void
				{
					if(status)
						ResourceProcessor::Get()->QueueLoad(resId, rt, nullptr);
				}, false);
			}

			it++;
		}
#endif
	}
}