#include "stdafx.h"
#include "MaterialMgr.h"
#include "Common.h"

using namespace EngineCore;

MaterialMgr* MaterialMgr::instance = nullptr;
Material* MaterialMgr::null_material = nullptr;

MaterialMgr::MaterialMgr()
{
	if(!instance)
	{
		instance = this;

		material_map.reserve(MATERIAL_INIT_COUNT);
		null_material = new Material(string(PATH_MATERIAL_NULL));
	}
	else
		ERR("Only one instance of MaterialMgr is allowed!");
}

MaterialMgr::~MaterialMgr()
{
	for(auto& it: material_map)
	{
		_DELETE(it.second.material);
		it.second.name.clear();
	}
	_DELETE(null_material);

	instance = nullptr;
}

Material* MaterialMgr::GetMaterial(string& name)
{
	Material* res = nullptr;
	if(name.length() == 0)
		return null_material;

	res = FindMaterialInList(name);
	if(res != nullptr)
		return res;

	res = AddMaterialToList(name);
	if(res != nullptr)
		return res;

	ERR("Cant get material %s , using NULL material!", name.c_str());
	return null_material;
}

Material* MaterialMgr::AddMaterialToList(string& name)
{
	MaterialHandle handle;

	handle.material = new Material(name);
	if(handle.material->IsError())
	{
		_DELETE(handle.material);
		return nullptr;
	}
	LOG("Material loaded %s", name.c_str());

	handle.name = name;
	handle.refcount = 1;

	material_map.insert(make_pair(name, handle));
	return handle.material;
}

Material* MaterialMgr::FindMaterialInList(string& name)
{
	auto it = material_map.find(name);
	if(it == material_map.end())
		return nullptr;

	it->second.refcount++;
	return it->second.material;
}

void MaterialMgr::DeleteMaterial(string& name)
{
	auto it = material_map.find(name);
	if(it == material_map.end())
		return;
	
	if(it->second.refcount <= 1)
	{
		_DELETE(it->second.material);
		LOG("Material droped %s", name.c_str());
		it->second.name.clear();
		material_map.erase(it);
	}
	else
		it->second.refcount--;
}