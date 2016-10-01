#include "stdafx.h"
#include "GlobalColor.h"
#include "Util.h"
#include "Common.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
	GlobalColor *GlobalColor::m_instance = nullptr;

	GlobalColor::GlobalColor()
	{	
		if (!m_instance)
		{
			m_instance = this;

			if(!LoadColorsFromFile(string(PATH_GUI_COLORS)))
				ERR("Cant load colors information from %s !", PATH_GUI_COLORS);
		}
		else
			ERR("Only one instance of GlobalColor is allowed!");
	}

	GlobalColor::~GlobalColor()
	{
		m_instance = nullptr;
		colors.clear();
	}

	bool GlobalColor::LoadColorsFromFile(string& filename)
	{
		FileIO file(filename);
		auto root = file.Root();
		if(!root)
			return false;

		auto colors = file.Node(L"colors", root);
		if(!colors)
			return false;

		for(auto& it: *colors)
			AddColor(WstringToString(it.first), WCharToXMFloat4((wchar_t*)it.second.value.c_str()));

		return true;
	}

	void GlobalColor::ClearColors()
	{
		colors.clear();
	}

	void GlobalColor::AddColor(string& id, XMFLOAT4 l_color)
	{
		colors.insert(make_pair(id, l_color));
	}

	XMFLOAT4* GlobalColor::GetColorPtr(string& id)
	{
		auto it = colors.find(id);
		if(it == colors.end())
		{
			ERR("Undefined color id %s", id.c_str());
			return (XMFLOAT4*)&null_color;
		}

		return &it->second;
	}
}