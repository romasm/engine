#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
	namespace TexLoader
	{		
		ID3D11ShaderResourceView* LoadFromMemory(string& resName, uint8_t* data, uint32_t size);
		ID3D11ShaderResourceView* LoadFromFile(string& filename);
		
		bool SaveTexture(string& filename, ID3D11ShaderResourceView* srv);

		bool IsSupported(string filename);
		WICCodecs WICCodec(string& filename);
	};
}