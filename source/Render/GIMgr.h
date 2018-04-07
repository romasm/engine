#pragma once

#include "Common.h"
#include "LightBuffers.h"

namespace EngineCore
{
	struct GISampleData
	{
		Vector3 minCorner;
		float worldSizeRcp;
	};

	class GIMgr
	{
	public:
		GIMgr(class BaseWorld* wrd);
		~GIMgr();
				
		bool ReloadGIData();
		void DropGIData();

		ID3D11ShaderResourceView* GetGIVolumeSRV();
		ID3D11Buffer* GetGISampleData() { return sampleDataGPU; }

	private:
		bool InitBuffers();

		BaseWorld* world;

		ID3D11Texture3D* giVolume;
		ID3D11ShaderResourceView* giVolumeSRV;
		ID3D11UnorderedAccessView* giVolumeUAV;
		
		uint32_t sgVolume;

		GISampleData sampleData;
		ID3D11Buffer* sampleDataGPU;
	};

}