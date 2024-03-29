#include "stdafx.h"
#include "ShaderMgr.h"
#include "Common.h"

using namespace EngineCore;

ShaderMgr::ShaderMgr() : BaseMgr<BaseShader, SHADERS_MAX_COUNT>()
{
	resType = ResourceType::SHADER;
	resExt = nullptr;
}

uint32_t ShaderMgr::AddResourceToList(string& name, bool simple, onLoadCallback callback, onLoadCallback callbackUpdate)
{
	if(free_ids.size() == 0)
	{
		ERR("Shader resources amount overflow!");
		return nullres;
	}

	uint32_t idx = free_ids.front();
	auto& handle = resource_array[idx];

	if(simple)
		handle.resource = (BaseShader*) new SimpleShader(name);
	else
		handle.resource = (BaseShader*) new Shader(name);

	if(handle.resource->IsError())
	{
		ResourceDeallocate(handle.resource);
		return SHADER_NULL;
	}
	else
		LOG("Shader loaded %s", name.c_str());

	handle.name = name;
	handle.refcount = 1;
	
#ifdef _EDITOR
	if (callbackUpdate)
		handle.callbackUpdate = callbackUpdate;
#endif // _EDITOR

	resource_map.insert(make_pair(name, idx));
	free_ids.pop_front();

	return idx;
}

void ShaderMgr::CheckForReload()
{
#ifdef _DEV
	ImportInfo info;
	uint32_t sourceDate = 0;

	auto it = resource_map.begin();
	while(it != resource_map.end())
	{
		auto& handle = resource_array[it->second];

		string srcFilename = it->first + EXT_SHADER_SOURCE;

		uint32_t last_date = FileIO::GetDateModifRaw(srcFilename);
		if(last_date == handle.resource->GetSrcDate())
		{
			it++;
			continue;
		}
				
		BaseShader* newShader;
		if(handle.resource->IsSimple())
			newShader = (BaseShader*) new SimpleShader((string&)it->first);
		else
			newShader = (BaseShader*) new Shader((string&)it->first);

		handle.resource->SetSrcDate(last_date);

		if(newShader->IsError())
		{
			ResourceDeallocate(newShader);
			it++;
			continue;
		}

		OnLoad(it->second, newShader, info, sourceDate);
		it++;
	}

	DefferedDeallocate();
#endif
}