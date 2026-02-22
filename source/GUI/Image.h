#pragma once

#include "Render.h"
#include "Material.h"

namespace EngineCore
{
	class Image2D
	{
	public:
		Image2D();
		
		void Init(const string& shadername);

		void SetRect(const MLRECT& rect) // todo
		{
			quad_rect = rect; 
			dirty = true;
		}
		MLRECT& GetRect(){return quad_rect;}

		inline SimpleShaderInst* GetShaderInst(){return shaderInst;}

		void Draw();
		void Close();

		inline void ForceUpdate() {shaderInst->ForceUpdate();}

		ALIGNED_ALLOCATION

	private:
		bool updateVertices();

		SimpleShaderInst* shaderInst;
		RHI::GfxBuffer *vertexBuffer;

		MLRECT quad_rect;

		bool dirty;
	};
}