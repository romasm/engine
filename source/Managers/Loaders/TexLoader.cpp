#include "stdafx.h"
#include "TexLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

using namespace EngineCore;

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

ID3D11ShaderResourceView* TexLoader::LoadFromFile(string& filename)
{
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(filename, &size);
	if(!data)
		return nullptr;
	
	auto result = LoadFromMemory(filename, data, size);
	_DELETE_ARRAY(data);
	return result;
}

ID3D11ShaderResourceView* TexLoader::LoadFromMemory(string& resName, uint8_t* data, uint32_t size)
{
	ID3D11ShaderResourceView* newTex = nullptr;

	if(resName.find(".dds") != string::npos || resName.find(".DDS") != string::npos)
	{
		HRESULT hr = CreateDDSTextureFromMemoryEx( DEVICE, data, size, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false, nullptr, &newTex, nullptr);
		if(FAILED(hr))
		{
			ERR("Cant load DDS texture %s !", resName.c_str());
			return nullptr;
		}
	}
	else if(resName.find(".tga") != string::npos || resName.find(".TGA") != string::npos)
	{
		TexMetadata metaData;
		ScratchImage image;
		HRESULT hr = LoadFromTGAMemory(data, size, &metaData, image);
		if(FAILED(hr))
		{
			ERR("Cant load TGA texture %s !", resName.c_str());
			return nullptr;
		}

		ScratchImage imageMips;
		GenerateMipMaps(*image.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, imageMips);
		CreateShaderResourceView(DEVICE, imageMips.GetImages(), imageMips.GetImageCount(), imageMips.GetMetadata(), &newTex);
	}
	else
	{
		TexMetadata metaData;
		ScratchImage image;
		HRESULT hr = LoadFromWICMemory(data, size, WIC_FLAGS_IGNORE_SRGB, &metaData, image);
		if(FAILED(hr))
		{
			ERR("Cant load WIC texture %s !", resName.c_str());
			return nullptr;
		}

		ScratchImage imageMips;
		GenerateMipMaps(*image.GetImage(0, 0, 0), TEX_FILTER_DEFAULT, 0, imageMips);
		CreateShaderResourceView(DEVICE, imageMips.GetImages(), imageMips.GetImageCount(), imageMips.GetMetadata(), &newTex);
	}	

	LOG("Texture loaded %s", resName.c_str());
	return newTex;
}

bool TexLoader::SaveTexture(string& filename, ID3D11ShaderResourceView* srv)
{
	ID3D11Resource* resource = nullptr;
	srv->GetResource(&resource);

	ScratchImage texture;
	auto hr = CaptureTexture(Render::Device(), Render::Context(), resource, texture);
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
			return nullptr;
		}

		HRESULT hr = SaveToWICFile( *texture.GetImage(0, 0, 0), WIC_FLAGS_NONE, GetWICCodec(codec), StringToWstring(filename).data() );
		if(FAILED(hr))
		{
			ERR("Cant save WIC texture %s !", filename.c_str());
			return nullptr;
		}
	}	

	LOG_GOOD("Texture saved %s", filename.c_str());
	return true;
}

WICCodecs TexLoader::WICCodec(string& filename)
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