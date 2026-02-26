#include "stdafx.h"
#include "TexLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"
#include "DX11Types.h"

using namespace EngineCore;

RHI::GfxSRV* TexLoader::LoadTexture(const string& resName)
{
	if( resName.find(".dds") == string::npos && resName.find(".DDS") == string::npos )
		return nullptr;

	// DX12 path: create DX12 texture + SRV via the RHI device
	if(GFX_DEVICE && GFX_DEVICE->IsDX12())
	{
		uint32_t size = 0;
		uint8_t* data = FileIO::ReadFileData(resName, &size);

		RHI::GfxSRV* result = nullptr;
		if(data)
		{
			result = GFX_DEVICE->LoadDDSFromMemory(data, size);
			_DELETE_ARRAY(data);
		}

		if(!result)
		{
			ERR("Cant load DDS texture %s !", resName.c_str());

#ifdef _EDITOR
#ifdef _DEV
			uint32_t date;
			ImportInfo info;
			ResourceProcessor::LoadImportInfo(resName, info, date);

			if( info.importBytes == 0 )
			{
				auto ext = resName.find(".dds");
				if( ext == string::npos )
					ext = resName.find(".DDS");

				string resourceName = resName.substr(0, ext);
				string tgaTexture = resourceName + ".tga";
				if( !FileIO::IsExist(tgaTexture) )
				{
					tgaTexture = resourceName + ".TGA";
					if( !FileIO::IsExist(tgaTexture) )
						return nullptr;
				}

				info.filePath = tgaTexture;
				info.resourceName = resourceName;
				info.importBytes = IMP_BYTE_TEXTURE;
				info.textureFormat = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			if( ResourceProcessor::ImportResource(info, true) )
			{
				data = FileIO::ReadFileData(resName, &size);
				if(data)
				{
					result = GFX_DEVICE->LoadDDSFromMemory(data, size);
					if(result)
						LOG("Texture loaded %s", resName.c_str());
					_DELETE_ARRAY(data);
				}
			}
#endif
#endif
		}
		else
		{
			LOG("Texture loaded %s", resName.c_str());
		}

		return result;
	}

	// DX11 path (unchanged)
	ID3D11ShaderResourceView* newTex = nullptr;

	HRESULT hr = E_FAIL;
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(resName, &size);
	if(data)
	{
		hr = CreateDDSTextureFromMemoryEx( DEVICE, data, size, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false, nullptr, &newTex, nullptr);
		_DELETE_ARRAY(data);
	}

	if(FAILED(hr))
	{
		ERR("Cant load DDS texture %s !", resName.c_str());

#ifdef _EDITOR
#ifdef _DEV

		uint32_t date;
		ImportInfo info;
		ResourceProcessor::LoadImportInfo(resName, info, date);

		if( info.importBytes == 0 )
		{
			auto ext = resName.find(".dds");
			if( ext == string::npos )
				ext = resName.find(".DDS");

			string resourceName = resName.substr(0, ext);
			string tgaTexture = resourceName + ".tga";
			if( !FileIO::IsExist(tgaTexture) )
			{
				tgaTexture = resourceName + ".TGA";
				if( !FileIO::IsExist(tgaTexture) )
				{
					//LOG("Reimport failed for %s", tgaTexture.c_str());
					return nullptr;
				}
			}

			// standard settings
			info.filePath = tgaTexture;
			info.resourceName = resourceName;
			info.importBytes = IMP_BYTE_TEXTURE;
			info.textureFormat = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		if( ResourceProcessor::ImportResource(info, true) )
		{
			data = FileIO::ReadFileData(resName, &size);
			if(data)
			{
				CreateDDSTextureFromMemoryEx( DEVICE, data, size, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false, nullptr, &newTex, nullptr);
				LOG("Texture loaded %s", resName.c_str());
				_DELETE_ARRAY(data);
			}
		}

#endif
#endif
	}
	else
	{
		LOG("Texture loaded %s", resName.c_str());
	}

	if(!newTex)
		return nullptr;

	auto* result = new RHI::DX11::DX11SRV();
	result->view = newTex;

	// Extract the underlying texture resource and wrap it so that
	// RHI code can access the backing GfxTexture via sourceTexture.
	ID3D11Resource* rawResource = nullptr;
	newTex->GetResource(&rawResource);
	if(rawResource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		newTex->GetDesc(&srvDesc);

		auto* texWrapper = new RHI::DX11::DX11Texture();
		// Determine dimension from SRV view type
		ID3D11Texture2D* tex2D = nullptr;
		ID3D11Texture3D* tex3D = nullptr;
		if(SUCCEEDED(rawResource->QueryInterface(&tex2D)) && tex2D)
		{
			texWrapper->texture2D = tex2D; // AddRef'd by QueryInterface
			D3D11_TEXTURE2D_DESC texDesc;
			tex2D->GetDesc(&texDesc);
			texWrapper->width     = texDesc.Width;
			texWrapper->height    = texDesc.Height;
			texWrapper->depth     = texDesc.ArraySize;
			texWrapper->mipLevels = texDesc.MipLevels;
			texWrapper->format    = texDesc.Format;
			if(texDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
				texWrapper->dimension = (texDesc.ArraySize > 6)
					? RHI::TextureDimension::CubeMapArray
					: RHI::TextureDimension::CubeMap;
			else if(texDesc.ArraySize > 1)
				texWrapper->dimension = RHI::TextureDimension::Tex2DArray;
			else
				texWrapper->dimension = RHI::TextureDimension::Tex2D;
		}
		else if(SUCCEEDED(rawResource->QueryInterface(&tex3D)) && tex3D)
		{
			texWrapper->texture3D = tex3D; // AddRef'd by QueryInterface
			D3D11_TEXTURE3D_DESC texDesc;
			tex3D->GetDesc(&texDesc);
			texWrapper->width     = texDesc.Width;
			texWrapper->height    = texDesc.Height;
			texWrapper->depth     = texDesc.Depth;
			texWrapper->mipLevels = texDesc.MipLevels;
			texWrapper->format    = texDesc.Format;
			texWrapper->dimension = RHI::TextureDimension::Tex3D;
		}
		rawResource->Release(); // GetResource AddRef'd it

		result->sourceTexture     = texWrapper;
		result->ownsSourceTexture = true;
	}

	return result;
}

bool TexLoader::ConvertTextureToEngineFormat(const string& sourceFile, const string& resFile, bool getMips, uint32_t genMipsFilter)
{
	bool status = false;

	if( !IsSupported(sourceFile) )
		return status;
	
	if(sourceFile.find(".dds") != string::npos || sourceFile.find(".DDS") != string::npos)
	{
		if( sourceFile == resFile )
			status = true;
		else
			status = FileIO::Copy(sourceFile, resFile);

		if(status)
			LOG_GOOD("Using *.dds as engine texture %s", resFile.c_str());
		else
			ERR("Texture %s IS NOT copied to engine texture", sourceFile.c_str());

		return status;
	}

	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(sourceFile, &size);
	if(!data)
		return false;

	TexMetadata metaData;
	ScratchImage image;
	ScratchImage imageMips;
	HRESULT hr;
	if(sourceFile.find(".tga") != string::npos || sourceFile.find(".TGA") != string::npos)
	{
		hr = LoadFromTGAMemory(data, size, &metaData, image);
		if(FAILED(hr))
			ERR("Cant load TGA texture %s !", sourceFile.c_str());
	}
	else
	{
		hr = LoadFromWICMemory(data, size, WIC_FLAGS_IGNORE_SRGB, &metaData, image);
		if(FAILED(hr))
			ERR("Cant load WIC texture %s !", sourceFile.c_str());
	}

	if(SUCCEEDED(hr))
	{
		if(getMips)
		{
			hr = GenerateMipMaps(*image.GetImage(0, 0, 0), genMipsFilter, 0, imageMips);
			if(SUCCEEDED(hr))
			{
				hr = SaveToDDSFile( imageMips.GetImages(), imageMips.GetImageCount(), imageMips.GetMetadata(), 
					DDS_FLAGS_NONE, StringToWstring(resFile).data() );
			}
			else
			{
				ERR("Cant generate mip-maps for texture %s !", sourceFile.c_str());
			}
		}
		else
		{
			hr = SaveToDDSFile( image.GetImages(), image.GetImageCount(), image.GetMetadata(), 
				DDS_FLAGS_NONE, StringToWstring(resFile).data() );
		}

		if(SUCCEEDED(hr))
			status = true;
	}

	if(status)
		LOG_GOOD("Texture %s converted to engine format", sourceFile.c_str());
	else
		ERR("Texture %s IS NOT converted to engine format", sourceFile.c_str());

	_DELETE_ARRAY(data);
	return status;
}

bool TexLoader::IsNative(string filename)
{
	if(filename.find(EXT_TEXTURE) != string::npos)
		return true;
	return false;
}

bool TexLoader::IsSupported(string filename)
{
	if(filename.find(".dds") != string::npos || filename.find(".DDS") != string::npos ||
		filename.find(".tga") != string::npos || filename.find(".TGA") != string::npos)
	{
		return true;
	}
	else 
	{
		WICCodecs codec = WICCodec(filename);	
		if(codec == 0)
			return false;
	}
	return true;
}

bool TexLoader::SaveTexture(const string& filename, RHI::GfxSRV* srv)
{
	ScratchImage texture;
	auto hr = GFX_DEVICE->CaptureTexture(srv, texture);
	if ( FAILED(hr) )
		return false;
	
	if(filename.find(".dds") != string::npos || filename.find(".DDS") != string::npos)
	{
		HRESULT hr = SaveToDDSFile( texture.GetImages(), texture.GetImageCount(), texture.GetMetadata(), DDS_FLAGS_NONE, StringToWstring(filename).data() );
		if(FAILED(hr))
		{
			ERR("Cant save DDS texture %s !", filename.c_str());
			return false;
		}
	}
	else if(filename.find(".tga") != string::npos || filename.find(".TGA") != string::npos)
	{
		HRESULT hr = SaveToTGAFile( *texture.GetImage(0, 0, 0), StringToWstring(filename).data() );
		if(FAILED(hr))
		{
			ERR("Cant save TGA texture %s !", filename.c_str());
			return false;
		}
	}
	else
	{
		WICCodecs codec = WICCodec(filename);	
		if(codec == 0)
		{
			ERR("Unsupported texture format for %s !", filename.c_str());
			return false;
		}

		HRESULT hr = SaveToWICFile( *texture.GetImage(0, 0, 0), WIC_FLAGS_NONE, GetWICCodec(codec), StringToWstring(filename).data() );
		if(FAILED(hr))
		{
			ERR("Cant save WIC texture %s !", filename.c_str());
			return false;
		}
	}	

	LOG_GOOD("Texture saved %s", filename.c_str());
	return true;
}

WICCodecs TexLoader::WICCodec(const string& filename)
{
	WICCodecs codec = (WICCodecs)0;
	if( filename.find(".bmp") != string::npos || filename.find(".BMP") != string::npos )
		codec = WIC_CODEC_BMP;
	else if( filename.find(".jpg") != string::npos || filename.find(".JPG") != string::npos )
		codec = WIC_CODEC_JPEG;
	else if( filename.find(".png") != string::npos || filename.find(".PNG") != string::npos )
		codec = WIC_CODEC_PNG;
	else if( filename.find(".tif") != string::npos || filename.find(".TIF") != string::npos )
		codec = WIC_CODEC_TIFF;
	else if( filename.find(".gif") != string::npos || filename.find(".GIF") != string::npos )
		codec = WIC_CODEC_GIF;
	else if( filename.find(".wmp") != string::npos || filename.find(".WMP") != string::npos )
		codec = WIC_CODEC_WMP;
	else if( filename.find(".ico") != string::npos || filename.find(".ICO") != string::npos )
		codec = WIC_CODEC_ICO;	
	return codec;
}