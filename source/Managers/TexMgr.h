#pragma once
#include "stdafx.h"
#include "Common.h"
#include "BaseMgr.h"
#include "TexLoader.h"
#include "Pathes.h"

#define TEXTURE(name) TexMgr::Get()->GetResource(name)
#define RELOADABLE_TEXTURE(name, need_reload) TexMgr::Get()->GetResource(name, need_reload)
#define TEXTURE_DROP(id) {TexMgr::Get()->DeleteResource((uint32_t)id); id = TexMgr::nullres;}
#define TEXTURE_NAME_DROP(name) TexMgr::Get()->DeleteResourceByName(name);

#define TEXTURE_GETPTR(id) TexMgr::GetResourcePtr(id)

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
		inline static TexMgr* Get(){return (TexMgr*)BaseMgr<ID3D11ShaderResourceView>::Get();}
	};
}