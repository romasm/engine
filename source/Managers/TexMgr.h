#pragma once
#include "stdafx.h"
#include "Common.h"

#define TEX_MAX_COUNT 65536
#define TEX_INIT_COUNT 1024
#define TEX_NULL TEX_MAX_COUNT

#define TEXTURE(name) TexMgr::Get()->GetTexture(name)
#define RELOADABLE_TEXTURE(name, need_reload) TexMgr::Get()->GetTexture(name, need_reload)
#define TEXTURE_DROP(id) {TexMgr::Get()->DeleteTexture((uint32_t)id); id = TEX_NULL;}
#define TEXTURE_NAME_DROP(name) TexMgr::Get()->DeleteTextureByName(name);

#define TEXTURE_GETPTR(id) TexMgr::GetTexturePtr(id)

namespace EngineCore
{
	class TexMgr
	{
	public:
		TexMgr();
		~TexMgr();
		
		inline static TexMgr* Get(){return instance;}
		
		static string& GetTextureName(uint32_t id)
		{
			if(id == TEX_NULL) return null_name;
			return instance->tex_array[id].name;
		}

		uint32_t GetTexture(string& name, bool reload = false);
		void DeleteTexture(uint32_t id);
		void DeleteTextureByName(string& name);

		void PreloadTextures();

		inline static ID3D11ShaderResourceView* GetTexturePtr(uint32_t id)
		{
			if(id == TEX_NULL) return null_texture;
			return instance->tex_array[id].tex;
		}

		void UpdateTextures();

	private:
		static TexMgr *instance;
		static ID3D11ShaderResourceView* null_texture;
		static string null_name;

		struct TexHandle
		{
			ID3D11ShaderResourceView* tex;
			uint32_t refcount;
			uint32_t filedate;
			string name;

			TexHandle()
			{
				tex = nullptr;
				refcount = 0;
				filedate = 0;
			}
		};

		unordered_map<string, uint32_t> tex_map;
		
		SArray<TexHandle, TEX_MAX_COUNT> tex_array;
		SDeque<uint32_t, TEX_MAX_COUNT> tex_free;
		
		uint32_t AddTextureToList(string& name, bool reload);
		uint32_t FindTextureInList(string& name);

		ID3D11ShaderResourceView* LoadTexture(string& name);

	public:
		static bool SaveTexture(string& name, ID3D11ShaderResourceView* srv);
	};
}