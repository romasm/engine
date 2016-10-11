#include "stdafx.h"
#include "FontMgr.h"
#include "Common.h"
#include "DataTypes.h"
#include "Render.h"

using namespace EngineCore;

FontMgr *FontMgr::instance = nullptr;

FontMgr::FontMgr()
{
	if(!instance)
	{
		instance = this;
		font_map.reserve(FONT_INIT_COUNT);
	}
	else
		ERR("Only one instance of FontMgr is allowed!");
}

FontMgr::~FontMgr()
{
	for(auto& it: font_map)
		_DELETE(it.second.font);
	font_map.clear();

	instance = nullptr;
}

void FontMgr::PreloadFonts()
{
	GetFont(string(PATH_FONTS "opensans_light_18px"));
	GetFont(string(PATH_FONTS "opensans_light_20px"));
	GetFont(string(PATH_FONTS "opensans_light_25px"));
	GetFont(string(PATH_FONTS "opensans_light_40px"));
	GetFont(string(PATH_FONTS "opensans_light_70px"));
	GetFont(string(PATH_FONTS "opensans_normal_12px"));
	GetFont(string(PATH_FONTS "opensans_normal_14px"));
	GetFont(string(PATH_FONTS "opensans_normal_16px"));
	GetFont(string(PATH_FONTS "opensans_normal_18px"));
	GetFont(string(PATH_FONTS "opensans_normal_20px"));
	GetFont(string(PATH_FONTS "opensans_normal_25px"));
	GetFont(string(PATH_FONTS "opensans_normal_40px"));
	GetFont(string(PATH_FONTS "opensans_normal_70px"));
}

Font* FontMgr::GetFont(string& name)
{
	if(name.size() == 0)
		return nullptr;

	Font* font = FindFontInList(name);
	if(font != nullptr)
		return font;

	font = AddFontToList(name);
	if(font != nullptr)
		return font;

	ERR("Cant get font %s!", name.c_str());
	return nullptr;
}

Font* FontMgr::AddFontToList(string& name)
{
	FontHandle handle;
	handle.name = name;
	handle.font = new Font(name);
	LOG("Font loaded %s", name.c_str());

	handle.refcount = 1;
	
	font_map.insert(make_pair(name, handle));
	return handle.font;
}

Font* FontMgr::FindFontInList(string& name)
{
	auto it = font_map.find(name);
	if(it == font_map.end())
		return nullptr;

	it->second.refcount++;
	return it->second.font;
}

void FontMgr::DeleteFont(string& name)
{
	auto it = font_map.find(name);
	if(it == font_map.end())
		return;
	
	if(it->second.refcount <= 1)
	{
		_DELETE(it->second.font);
		LOG("Font droped %s", it->second.name.c_str());
		it->second.name.clear();
		font_map.erase(it);
	}
	else
		it->second.refcount--;
}