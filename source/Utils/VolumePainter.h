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
		struct BrushInfo
		{
			Vector3 position;
			float radius;

			Vector4 colorOpacity;
		};

		struct VolumeInfo
		{
			Vector3 minCorner;
			float _padding0;

			Vector3 sizeInv;
			float _padding1;

			Vector3 size;
			float _padding2;
		};

	public:

		VolumePainter();
		~VolumePainter();

		bool Init(uint32_t width, uint32_t height, uint32_t depth);

		luaSRV GetSRV()	{ return luaSRV(volumeTextureSRV); }

		void ImportTexture(string textureName);
		void DrawBrush(Vector3& position, float radius, Vector4& colorOpacity);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<VolumePainter>("VolumePainter")
				.addConstructor<void(*)(void)>()
				.addFunction("Init", &VolumePainter::Init)
				.addFunction("GetSRV", &VolumePainter::GetSRV)
				.addFunction("ImportTexture", &VolumePainter::ImportTexture)
				.addFunction("DrawBrush", &VolumePainter::DrawBrush)
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

		ID3D11Buffer* brushInfoBuffer;
		ID3D11Buffer* volumeInfoBuffer;
	};
}