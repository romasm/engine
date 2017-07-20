#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "TexLoader.h"
#include "Pathes.h"

#define TEXTURE(name) TexMgr::Get()->GetTexture(name)
#define RELOADABLE_TEXTURE(name, need_reload) TexMgr::Get()->GetTexture(name, need_reload)
#define TEXTURE_DROP(id) {TexMgr::Get()->DeleteTexture((uint32_t)id); id = TEX_NULL;}
#define TEXTURE_NAME_DROP(name) TexMgr::Get()->DeleteTextureByName(name);

#define TEXTURE_GETPTR(id) TexMgr::GetTexturePtr(id)

namespace EngineCore
{
	class TexMgr : public BaseMgr<ID3D11ShaderResourceView>
	{
	public:
		TexMgr() : BaseMgr<ID3D11ShaderResourceView>()
		{
			null_resource = TexLoader::LoadFromFile(string(PATH_TEXTURE_NULL));
			resType = ResourceType::TEXTURE;
		}
	};
}