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
		D3D11_SRV_DIMENSION type;

		TextureMeta() : width(0), height(0), depth(0), mipsCount(0), arraySize(0), 
			format(DXGI_FORMAT_UNKNOWN), type(D3D_SRV_DIMENSION_UNKNOWN) {}

		inline bool IsInvalid()
		{
			return (type == D3D_SRV_DIMENSION_UNKNOWN);
		}
	};

	class TexMgr : public BaseMgr<ID3D11ShaderResourceView>
	{
	public:
		TexMgr() : BaseMgr<ID3D11ShaderResourceView>()
		{
			null_resource = TexLoader::LoadTexture(string(PATH_TEXTURE_NULL));
			resType = ResourceType::TEXTURE;
			resExt = EXT_TEXTURE;
		}
		inline static TexMgr* Get(){return (TexMgr*)BaseMgr<ID3D11ShaderResourceView>::Get();}

		virtual void ResourceDeallocate(ID3D11ShaderResourceView*& resource)
		{
			_RELEASE(resource);
		};

		static TextureMeta GetMeta(uint32_t id)
		{
			TextureMeta result;

			auto srv = GetResourcePtr(id);			
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			srv->GetDesc(&desc);
									
			ID3D11Resource* resource = nullptr;
			srv->GetResource(&resource);
			if(!resource)
				return result;

			switch (desc.ViewDimension)
			{
			case D3D11_SRV_DIMENSION_TEXTURE1D:
			case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
				{
					ID3D11Texture1D *tex = 0;
					resource->QueryInterface<ID3D11Texture1D>(&tex);
					if(!tex)
						return result;

					D3D11_TEXTURE1D_DESC texDesc;
					tex->GetDesc(&texDesc);

					result.width = texDesc.Width;
					result.arraySize = texDesc.ArraySize;
					result.mipsCount = texDesc.MipLevels;
				}
				break;
			case D3D11_SRV_DIMENSION_TEXTURE2D:
			case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
			case D3D11_SRV_DIMENSION_TEXTURECUBE:
			case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
				{
					ID3D11Texture2D *tex = 0;
					resource->QueryInterface<ID3D11Texture2D>(&tex);
					if(!tex)
						return result;
					
					D3D11_TEXTURE2D_DESC texDesc;
					tex->GetDesc(&texDesc);

					result.width = texDesc.Width;
					result.height = texDesc.Height;
					result.arraySize = texDesc.ArraySize;
					result.mipsCount = texDesc.MipLevels;
				}
				break;
			case D3D11_SRV_DIMENSION_TEXTURE3D:
				{
					ID3D11Texture3D *tex = 0;
					resource->QueryInterface<ID3D11Texture3D>(&tex);
					if(!tex)
						return result;

					D3D11_TEXTURE3D_DESC texDesc;
					tex->GetDesc(&texDesc);

					result.width = texDesc.Width;
					result.height = texDesc.Height;
					result.depth = texDesc.Depth;
					result.mipsCount = texDesc.MipLevels;
				}
				break;
			default:
				WRN("Resource dimention is unsupported");
				return result;
			}
						
			result.format = desc.Format;
			result.type = desc.ViewDimension;
			return result;
		}
	};
}