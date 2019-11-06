#pragma once

#include "Common.h"
#include "Compute.h"

#define COMPUTE_IMPORT_TEXTURE PATH_SHADERS "volume/import_texture#Copy#"
#define COMPUTE_DRAW_BRUSH PATH_SHADERS "volume/draw_brush#Draw#"

#define COPMUTE_TREADS_X 8
#define COPMUTE_TREADS_Y 4
#define COPMUTE_TREADS_Z 2

namespace EngineCore
{
	class VolumePainter
	{
	public:

		VolumePainter();
		~VolumePainter();

		bool Init(uint32_t width, uint32_t height, uint32_t depth);

		luaSRV GetSRV()
		{
			luaSRV res;
			res.srv = volumeTextureSRV;
			return res;
		}

		bool ImportTexture(string textureName);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<VolumePainter>("VolumePainter")
				.addFunction("Init", &VolumePainter::Init)
				.addFunction("GetSRV", &VolumePainter::GetSRV)
				.endClass();
		}

	private:

		uint32_t volumeResolutionX;
		uint32_t volumeResolutionY;
		uint32_t volumeResolutionZ;

		ID3D11Texture3D* volumeTexture;
		ID3D11UnorderedAccessView* volumeTextureUAV;
		ID3D11ShaderResourceView* volumeTextureSRV;

		Compute* computeImportTexture;
		Compute* computeDrawBrush;
	};
}