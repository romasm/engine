#include "stdafx.h"
#include "BaseMgr.h"
#include "macros.h"
#include "Log.h"

using namespace EngineCore;

template<DataType>
BaseMgr* BaseMgr::instance = nullptr;

template<DataType>
BaseMgr::BaseMgr()
{
	if(!instance)
	{
		instance = this;

		resource_array.resize(RESOURCE_MAX_COUNT);
		free_ids.resize(RESOURCE_MAX_COUNT);
		for(uint32_t i = 0; i < RESOURCE_MAX_COUNT; i++)
			free_ids[i] = i;
		resource_map.reserve(RESOURCE_INIT_COUNT);

		null_resource = nullptr;
		null_name = "";

		resType = ResourceType(0);
	}
	else
		ERR("Only one instance of Manager is allowed!");
}

template<DataType>
BaseMgr::~BaseMgr()
{
	for(uint32_t i=0; i<RESOURCE_MAX_COUNT; i++)
	{
		_RELEASE(resource_array[i].resource);
		resource_array[i].name.erase();
	}
	_RELEASE(null_resource);
	null_name.clear();

	instance = nullptr;
}

template<DataType>
uint32_t BaseMgr::GetResource(string& name, bool reload, onLoadCallback callback)
{
	uint32_t res = RESOURCE_NULL;
	if(name.length() == 0)
		return res;

	res = FindResourceInList(name);
	if(res != RESOURCE_NULL)
	{
		const LoadingStatus status = (resource_array[res].resource == null_resource) ? LoadingStatus::NEW : LoadingStatus::LOADED;
		CallCallback(res, callback, status);
		return res;
	}

	res = AddResourceToList(name, reload, callback);
	if(res != RESOURCE_NULL)
		return res;

	ERR("Cant load resource %s", name.c_str());

	return res;
}

template<DataType>
uint32_t BaseMgr::AddResourceToList(string& name, bool reload, onLoadCallback callback)
{
	if(free_ids.size() == 0)
	{
		ERR("Resources amount overflow!");
		return RESOURCE_NULL;
	}

	uint32_t idx = free_ids.front();
	auto& handle = resource_array[idx];

	handle.name = name;
	handle.tex = null_resource;
	handle.refcount = 1;
	
	if(!FileIO::IsExist(name))
	{
		WRN("File %s doesn\'t exist, creation expected in future.", name.data());
		if(reload)
			handle.filedate = ReloadingType::RELOAD_ALWAYS;
		else
			handle.filedate = ReloadingType::RELOAD_ONCE;
	}
	else
	{
		if(reload)
			handle.filedate = FileIO::GetDateModifRaw(name);
		else
			handle.filedate = ReloadingType::RELOAD_NONE;
		ResourceProcessor::Get()->QueueLoad(idx, resType, callback);
	}
	
	resource_map.insert(make_pair(name, idx));
	free_ids.pop_front();
	
	return idx;
}

template<DataType>
uint32_t BaseMgr::FindResourceInList(string& name)
{
	auto it = resource_map.find(name);
	if(it == resource_map.end())
		return RESOURCE_NULL;

	auto& handle = resource_array[it->second];
	handle.refcount++;
	return it->second;
}

template<DataType>
void BaseMgr::DeleteTexture(uint32_t id)
{
	if(id == RESOURCE_NULL)
		return;
	
	auto& handle = resource_array[id];

	if(handle.refcount == 1)
	{
		if(handle.tex != null_resource)
		{
			_RELEASE(handle.tex);
			LOG("Texture droped %s", handle.name.c_str());
		}

		handle.refcount = 0;
		handle.filedate = 0;

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

template<DataType>
void BaseMgr::DeleteTextureByName(string& name)
{
	if(name.length() == 0)
		return;
	
	auto it = resource_map.find(name);
	if(it == resource_map.end())
		return;

	auto& handle = resource_array[it->second];

	if(handle.refcount == 1)
	{
		if(handle.tex != null_resource)
		{
			_RELEASE(handle.resource);
			LOG("Resource droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		free_ids.push_back(it->second);

		resource_map.erase(name);

		handle.name.clear();
	}
	else
		handle.refcount--;
}

template<DataType>
void BaseMgr::OnPostLoadMainThread(uint32_t id, onLoadCallback func, LoadingStatus status)
{
	if(func)
		func(id, status == LOADED);
}

template<DataType>
void BaseMgr::OnLoad(uint32_t id, ID3D11ShaderResourceView* data)
{
	auto& handle = resource_array[id];

	auto oldResource = handle.resource;
	handle.resource = data;
	if(oldResource != null_resource)
		_RELEASE(oldResource);
}

template<DataType>
void BaseMgr::UpdateTextures()
{
	auto it = resource_map.begin();
	while(it != resource_map.end())
	{
		auto& handle = resource_array[it->second];

		if( handle.filedate == ReloadingType::RELOAD_NONE )
		{
			it++;
			continue;
		}

		if( handle.filedate == ReloadingType::RELOAD_ONCE )
			handle.filedate = ReloadingType::RELOAD_NONE;
		else
		{
			uint32_t last_date = FileIO::GetDateModifRaw(handle.name);
			if( last_date == handle.filedate || last_date == 0 || handle.filedate == ReloadingType::RELOAD_NONE )
			{	
				it++;
				continue;
			}
			handle.filedate = last_date;
		}
		
		ResourceProcessor::Get()->QueueLoad(it->second, resType, nullptr);
		it++;
	}
}