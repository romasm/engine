#pragma once

#include "Render.h"
#include "Material.h"

#define SP_BUFFER_COUNT 10

namespace EngineCore
{
	class ScreenPlane
	{
	public:
		ScreenPlane(const string& shadername)
		{createPlane(shadername);}
		ScreenPlane(const char* shadername)
		{createPlane(string(shadername));}

		~ScreenPlane();

		void Draw();

		inline SimpleShaderInst* GetShaderInst(){return shaderInst;}
		inline void SetFloat(float f, uint8_t i){shaderInst->SetFloat(f, i);}
		inline void SetVector(Vector4 v, uint8_t i){shaderInst->SetVector(v, i);}

		inline void SetTexture(RHI::GfxSRV *tex, uint8_t id){shaderInst->SetTexture(tex, id);}
		inline void SetTextureByName(const string& name, uint8_t id){shaderInst->SetTextureByName(name, id);}
		inline void SetTextureByNameS(const char* name, uint8_t id){shaderInst->SetTextureByName(string(name), id);}
		inline void ClearTex(){shaderInst->ClearTextures();}

	private:
		void createPlane(const string& shadername);

		SimpleShaderInst* shaderInst;
	};
}
