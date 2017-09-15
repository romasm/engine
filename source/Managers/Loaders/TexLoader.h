#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
	namespace TexLoader
	{		
		ID3D11ShaderResourceView* LoadTexture(string& resName);
		
		bool SaveTexture(string& filename, ID3D11ShaderResourceView* srv);
		bool ConvertTextureToEngineFormat(string& sourceFile, string& resFile, bool getMips, uint32_t genMipsFilter);

		bool IsSupported(string filename);
		WICCodecs WICCodec(string& filename);
	};
}