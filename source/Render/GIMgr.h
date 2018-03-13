#pragma once

#include "Common.h"
#include "LightBuffers.h"

namespace EngineCore
{
	class GIMgr
	{
	public:
		GIMgr(bool onlySky);
		~GIMgr();
				
	private:
		bool InitBuffers();

		ID3D11Texture3D* giVolume;
		ID3D11ShaderResourceView* giVolumeSRV;
		ID3D11UnorderedAccessView* giVolumeUAV;
	};

}