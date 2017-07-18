#include "stdafx.h"
#include "TexMgr.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

#include "ScenePipeline.h"
#include "EnvProbSystem.h"
#include "TexLoader.h"

using namespace EngineCore;

TexMgr* TexMgr::instance = nullptr;
ID3D11ShaderResourceView* TexMgr::null_texture = nullptr;
string TexMgr::null_name = "";

TexMgr::TexMgr()
{
	if(!instance)
	{
		instance = this;

		tex_array.resize(TEX_MAX_COUNT);
		tex_free.resize(TEX_MAX_COUNT);
		for(uint32_t i=0; i<TEX_MAX_COUNT; i++)
			tex_free[i] = i;
		tex_map.reserve(TEX_INIT_COUNT);

		null_texture = TexLoader::LoadFromFile(string(PATH_TEXTURE_NULL));
	}
	else
		ERR("Only one instance of TexMgr is allowed!");
}

TexMgr::~TexMgr()
{
	for(uint32_t i=0; i<TEX_MAX_COUNT; i++)
	{
		_RELEASE(tex_array[i].tex);
		tex_array[i].name.erase();
	}
	_RELEASE(null_texture);
	null_name.clear();

	instance = nullptr;
}

void TexMgr::PreloadTextures()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetTexture(string(TEX_NOISE2D), reload);
	GetTexture(string(TEX_PBSENVLUT), reload);
	GetTexture(string(TEX_SMAA_AREA), reload);
	GetTexture(string(TEX_SMAA_SEARCH), reload);
	GetTexture(string(TEX_HBAO_DITHER), reload);

	GetTexture(string(TEX_HAMMERSLEY), reload);

	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/arrow_down"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/arrow_right"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/maximize"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/minimize"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/move"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/pipet"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/reset"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/restore"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/rotate"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/scale"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/select"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/win_close"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/assign_asset"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/clear_str"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/delete"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/copy_mat"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/new_mat"EXT_TEXTURE), reload);
}

uint32_t TexMgr::GetTexture(string& name, bool reload, onLoadCallback callback)
{
	uint32_t res = TEX_NULL;
	if(name.length() == 0)
		return res;

	res = FindTextureInList(name);
	if(res != TEX_NULL)
	{
		const LoadingStatus status = (tex_array[res].tex == null_texture) ? LoadingStatus::NEW : LoadingStatus::LOADED;
		CallCallback(res, callback, status);
		return res;
	}

	res = AddTextureToList(name, reload, callback);
	if(res != TEX_NULL)
		return res;

	ERR("Cant load texture %s", name.c_str());

	return res;
}

uint32_t TexMgr::AddTextureToList(string& name, bool reload, onLoadCallback callback)
{
	if(tex_free.size() == 0)
	{
		ERR("Texture resources amount overflow!");
		return TEX_NULL;
	}

	uint32_t idx = tex_free.front();
	auto& handle = tex_array[idx];

	handle.name = name;
	handle.tex = null_texture;
	handle.refcount = 1;
	
	if(!FileIO::IsExist(name))
	{
		WRN("Texture file %s doesn\'t exist, creation expected in future.", name.data());
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
		ResourceProcessor::Get()->QueueLoad(idx, ResourceType::TEXTURE, callback);
	}
	
	tex_map.insert(make_pair(name, idx));
	tex_free.pop_front();
	
	return idx;
}

uint32_t TexMgr::FindTextureInList(string& name)
{
	auto it = tex_map.find(name);
	if(it == tex_map.end())
		return TEX_NULL;

	auto& handle = tex_array[it->second];
	handle.refcount++;
	return it->second;
}

void TexMgr::DeleteTexture(uint32_t id)
{
	if(id == TEX_NULL)
		return;
	
	auto& handle = tex_array[id];

	if(handle.refcount == 1)
	{
		if(handle.tex != null_texture)
		{
			_RELEASE(handle.tex);
			LOG("Texture droped %s", handle.name.c_str());
		}

		handle.refcount = 0;
		handle.filedate = 0;

		tex_free.push_back(id);

		tex_map.erase(handle.name);

		handle.name.clear();
	}
	else if(handle.refcount == 0)
	{
		WRN("Texture %s has already deleted!", handle.name.c_str());
	}
	else
		handle.refcount--;
}

void TexMgr::DeleteTextureByName(string& name)
{
	if(name.length() == 0)
		return;
	
	auto it = tex_map.find(name);
	if(it == tex_map.end())
		return;

	auto& handle = tex_array[it->second];

	if(handle.refcount == 1)
	{
		if(handle.tex != null_texture)
		{
			_RELEASE(handle.tex);
			LOG("Texture droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		tex_free.push_back(it->second);

		tex_map.erase(name);

		handle.name.clear();
	}
	else
		handle.refcount--;
}

void TexMgr::OnPostLoadMainThread(uint32_t id, onLoadCallback func, LoadingStatus status)
{
	CallCallback(id, func, status);
}

void TexMgr::CallCallback(uint32_t id, onLoadCallback func, LoadingStatus status) // TODO
{
	if(func)
		func(id, status == LOADED);
}

void TexMgr::OnLoad(uint32_t id, ID3D11ShaderResourceView* data)
{
	auto& handle = tex_array[id];

	auto oldTex = handle.tex;
	handle.tex = data;
	if(oldTex != null_texture)
		_RELEASE(oldTex);
}

void TexMgr::UpdateTextures()
{
	auto it = tex_map.begin();
	while(it != tex_map.end())
	{
		auto& handle = tex_array[it->second];

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
		
		ResourceProcessor::Get()->QueueLoad(it->second, ResourceType::TEXTURE);
		it++;
	}
}