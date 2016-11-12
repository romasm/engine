#include "stdafx.h"
#include "TexMgr.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

#include "ScenePipeline.h"
#include "ECS\EnvProbSystem.h"

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
		
		null_texture = LoadTexture(string(PATH_TEXTURE_NULL));
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
}

uint32_t TexMgr::GetTexture(string& name, bool reload)
{
	uint32_t res = TEX_NULL;
	if(name.length() == 0)
		return res;

	res = FindTextureInList(name);
	if(res != TEX_NULL)
		return res;

	res = AddTextureToList(name, reload);
	if(res != TEX_NULL)
		return res;

	ERR("Cant load texture %s", name.c_str());

	return res;
}

uint32_t TexMgr::AddTextureToList(string& name, bool reload)
{
	if(tex_free.size() == 0)
	{
		ERR("Texture resources amount overflow!");
		return TEX_NULL;
	}

	uint32_t idx = tex_free.front();
	auto& handle = tex_array[idx];

	//handle.tex = LoadTexture(name);
	//if(handle.tex == nullptr)
	//	return TEX_NULL;
	if(!FileIO::IsExist(name))
		return TEX_NULL;
	handle.tex = null_texture;
	
	handle.name = name;
	handle.refcount = 1;

	//handle.filedate = FileIO::GetDateModifRaw(name);
	if(reload)
		handle.filedate = NEED_RELOADING;
	else
		handle.filedate = NEED_LOADING_ONCE;

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

void TexMgr::UpdateTextures()
{
	for(auto& it: tex_map)
	{
		auto& tex = tex_array[it.second];

		if(tex.filedate == NOT_RELOAD)
			continue;

		if(tex.filedate == NEED_LOADING_ONCE)
			tex.filedate = NOT_RELOAD;
		else
		{
			uint32_t last_date = FileIO::GetDateModifRaw(tex.name);
			if(last_date == tex.filedate || tex.filedate == NOT_RELOAD)
				continue;
			tex.filedate = last_date;
		}

		auto newTex = LoadTexture(tex.name);
		if(!newTex)
			continue;

		auto oldTex = tex.tex;
		tex.tex = newTex;
		if(oldTex != null_texture)
			_RELEASE(oldTex);
	}
}

ID3D11ShaderResourceView* TexMgr::LoadTexture(string& name)
{
	wstring tempName = StringToWstring(name);
	ID3D11ShaderResourceView* tex = nullptr;

	if(name.find(".dds") != string::npos || name.find(".DDS") != string::npos)
	{
		HRESULT hr = CreateDDSTextureFromFileEx( DEVICE, tempName.c_str(), 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false, nullptr, &tex, nullptr);
		if(FAILED(hr))
		{
			ERR("Cant load DDS texture %s !", name.c_str());
			return nullptr;
		}
	}
	else if(name.find(".tga") != string::npos || name.find(".TGA") != string::npos)
	{
		TexMetadata metaData;
		ScratchImage image;
		HRESULT hr = LoadFromTGAFile(tempName.c_str(), &metaData, image);
		if(FAILED(hr))
		{
			ERR("Cant load TGA texture %s !", name.c_str());
			return nullptr;
		}
		
		ScratchImage imageMips;
		GenerateMipMaps(*image.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, imageMips);
		CreateShaderResourceView(DEVICE, imageMips.GetImages(), imageMips.GetImageCount(), imageMips.GetMetadata(), &tex);
	}
	else
	{
		TexMetadata metaData;
		ScratchImage image;
		HRESULT hr = LoadFromWICFile(tempName.c_str(), WIC_FLAGS_IGNORE_SRGB, &metaData, image);
		if(FAILED(hr))
		{
			ERR("Cant load WIC texture %s !", name.c_str());
			return nullptr;
		}
		
		ScratchImage imageMips;
		GenerateMipMaps(*image.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, imageMips);
		CreateShaderResourceView(DEVICE, imageMips.GetImages(), imageMips.GetImageCount(), imageMips.GetMetadata(), &tex);
	}	

	LOG("Texture loaded %s", name.c_str());

	return tex;
}