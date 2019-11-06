#include "stdafx.h"
#include "TexLoader.h"
#include "macros.h"
#include "Log.h"
#include "Render.h"

using namespace EngineCore;

ID3D11ShaderResourceView* TexLoader::LoadTexture(string& resName)
{
	ID3D11ShaderResourceView* newTex = nullptr;

	if( resName.find(".dds") == string::npos && resName.find(".DDS") == string::npos )
		return nullptr;

	HRESULT hr = -1;
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

	return newTex;
}

bool TexLoader::ConvertTextureToEngineFormat(string& sourceFile, string& resFile, bool getMips, uint32_t genMipsFilter)
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