#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
	struct TexData
	{
#ifdef _EDITOR
		uint32_t sourceDate;
		ImportInfo importInfo;
#endif

		ID3D11ShaderResourceView* data;

		TexData() : data(nullptr) {}
		~TexData()
		{
			_RELEASE(data);
		}
	};

	namespace TexLoader
	{		
		TexData* LoadTexture(string& resName);
		
		bool SaveTexture(string& filename, ID3D11ShaderResourceView* srv);

		bool IsSupported(string filename);
		WICCodecs WICCodec(string& filename);
	};
}