#pragma once

#include "macros.h"
#include "stdafx.h"
#include "DataTypes.h"
#include "Render.h"
#include "Shader.h"
#include "Buffer.h"
#include "Image.h"
#include "Font.h"
#include "Text.h"
#include "Hud.h"
#include "Util.h"
#include "Frustum.h"
#include "Timer.h"
#include "RenderTarget.h"
#include "ScreenPlane.h"
#include "TexMgr.h"
#include "ShaderMgr.h"
#include "StMeshMgr.h"
#include "Material.h"
#include "EngineSettings.h"
#include "Common.h"
#include "World.h"

#define DFG_res 256
#define DFG_mat PATH_SHADERS "offline/scep_diff_dfg"
#define DFG_tex PATH_TEXTURES "tech/system/pbs_env_lut" EXT_TEXTURE

#define NOISE2D_res 512
#define NOISE2D_mat PATH_SHADERS "offline/noise2d"
#define NOISE2D_tex PATH_TEXTURES "tech/system/noise2d" EXT_TEXTURE

namespace EngineCore
{
	static bool GenetareTexture(uint32_t res, string shadername, DXGI_FORMAT fmt, string file)
	{
		RenderTarget rt;
		if(!rt.Init(res,res))
		{
			ERR("Cant create texture %s with shader %s", file.c_str(), shadername.c_str());
			rt.Close();
			return false;
		}
		if(!rt.AddRT(fmt))
		{
			ERR("Cant create texture %s with shader %s", file.c_str(), shadername.c_str());
			rt.Close();
			return false;
		}

		ScreenPlane plane(shadername);

		rt.ClearRenderTargets(0,0,0,0);
		rt.SetRenderTarget();
		plane.Draw();

		ID3D11Resource* resource = nullptr;
		rt.GetShaderResourceView(0)->GetResource(&resource);
		ScratchImage texture;
	
		bool result = true;
		HRESULT hr = CaptureTexture(DEVICE, CONTEXT, resource, texture);
		if ( SUCCEEDED(hr) )
		{
			hr = SaveToDDSFile( texture.GetImages(), texture.GetImageCount(), texture.GetMetadata(), 
				DDS_FLAGS_NONE, StringToWstring(file).c_str() );
			if ( FAILED(hr) )
			{
				ERR("Cant create texture %s with shader %s", file.c_str(), shadername.c_str());
				result = false;
			}
		}
		else
		{
			ERR("Cant create texture %s with shader %s", file.c_str(), shadername.c_str());
			result = false;
		}

		rt.Close();

		return result;
	}
}