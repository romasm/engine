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
	struct TextureMeta
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;

		uint32_t mipsCount;
		uint32_t arraySize;

		DXGI_FORMAT format;
		RHI::TextureDimension dimension;

		TextureMeta() : width(0), height(0), depth(0), mipsCount(0), arraySize(0),
			format(DXGI_FORMAT_UNKNOWN), dimension(RHI::TextureDimension::Tex2D) {}

		inline bool IsInvalid()
		{
			return (width == 0 && height == 0);
		}
	};

	class TexMgr : public BaseMgr<RHI::GfxSRV>
	{
	public:
		TexMgr() : BaseMgr<RHI::GfxSRV>()
		{
			null_resource = TexLoader::LoadTexture(string(PATH_TEXTURE_NULL));
			resType = ResourceType::TEXTURE;
			resExt = EXT_TEXTURE;
		}
		inline static TexMgr* Get(){return (TexMgr*)BaseMgr<RHI::GfxSRV>::Get();}

		virtual void ResourceDeallocate(RHI::GfxSRV*& resource)
		{
			_DELETE(resource);
		};

		static TextureMeta GetMeta(uint32_t id)
		{
			TextureMeta result;

			auto* gfxSrv = GetResourcePtr(id);
			if(!gfxSrv || !gfxSrv->sourceTexture)
				return result;

			auto* tex = gfxSrv->sourceTexture;
			result.width     = tex->width;
			result.height    = tex->height;
			result.depth     = tex->depth;
			result.mipsCount = tex->mipLevels;
			result.format    = tex->format;
			result.dimension = tex->dimension;

			// arraySize: for cube maps depth stores faces/cubes, for arrays it's the slice count
			switch(tex->dimension)
			{
			case RHI::TextureDimension::CubeMap:      result.arraySize = 6; break;
			case RHI::TextureDimension::CubeMapArray:  result.arraySize = tex->depth * 6; break;
			case RHI::TextureDimension::Tex2DArray:    result.arraySize = tex->depth; break;
			default:                                   result.arraySize = 1; break;
			}

			return result;
		}
	};
}
