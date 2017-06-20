#include "stdafx.h"
#include "GlobalColor.h"
#include "Util.h"
#include "Common.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
	GlobalColor *GlobalColor::instance = nullptr;

	GlobalColor::GlobalColor()
	{	
		if (!instance)
		{
			instance = this;
			init = false;

			auto colorsPreset = CONFIG(string, colors);
			if(colorsPreset.empty())
				colorsPreset = PATH_COLORS_CONFIG;
			else
				colorsPreset = PATH_CONFIG "colors_" + colorsPreset + EXT_CONFIG;

			if(!Load(colorsPreset))
				ERR("Cant load colors information from %s !", colorsPreset.data());

			init = true;
		}
		else
			ERR("Only one instance of GlobalColor is allowed!");
	}

	GlobalColor::~GlobalColor()
	{
		instance = nullptr;
		colorsMap.clear();
	}

	bool GlobalColor::Load(string filename)
	{
		FileIO file(filename);
		auto root = file.Root();
		if(!root)
			return false;

		auto colors = file.Node("colors", root);
		if(!colors)
			return false;

		for(auto& it: *colors)
			instance->AddColor(string(it.first), CharToXMFloat4((char*)it.second.value.c_str()));

		instance->configName = filename;

		return true;
	}

	void GlobalColor::AddColor(string& id, XMFLOAT4 l_color)
	{
		if(init)
		{
			auto it = colorsMap.find(id);
			if(it != colorsMap.end())
				it->second = l_color;
		}
		else
			colorsMap.insert(make_pair(id, l_color));
	}

	XMFLOAT4* GlobalColor::GetColorPtr(string& id)
	{
		auto it = colorsMap.find(id);
		if(it == colorsMap.end())
		{
			if(id != "null")
				ERR("Undefined color id %s", id.c_str());
			return (XMFLOAT4*)&null_color;
		}

		return &it->second;
	}

	void GlobalColor::SetColor(string id, XMFLOAT4 color)
	{
		auto it = instance->colorsMap.find(id);
		if(it == instance->colorsMap.end())
		{
			ERR("Undefined color id %s", id.c_str());
			return;
		}
		it->second = color;
	}

	bool GlobalColor::Save(string filename)
	{
		FileIO file(filename);
		auto root = file.Root();
		if(!root)
			return false;
		
		auto colors = file.CreateNode("colors", root);
		if(!colors)
			return false;

		for(auto& it: instance->colorsMap)
			file.WriteFloat4(it.first, it.second, colors);

		file.Save();
		return true;
	}
}