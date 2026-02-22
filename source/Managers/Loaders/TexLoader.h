#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore::RHI { struct GfxSRV; }

namespace EngineCore
{
	namespace TexLoader
	{
		RHI::GfxSRV* LoadTexture(const string& resName);

		bool SaveTexture(const string& filename, RHI::GfxSRV* srv);
		bool ConvertTextureToEngineFormat(const string& sourceFile, const string& resFile, bool getMips, uint32_t genMipsFilter);

		bool IsNative(string filename);
		bool IsSupported(string filename);
		WICCodecs WICCodec(const string& filename);
	};
}
