#pragma once

#include "Common.h"
#include "Render.h"
#include "RenderTarget.h"
#include "ScreenPlane.h"

namespace EngineCore
{
#define GAUSSIAN_KERNEL_3_h PATH_SHADERS "system/gblur/kernel3h"
#define GAUSSIAN_KERNEL_3_v PATH_SHADERS "system/gblur/kernel3v"
#define GAUSSIAN_KERNEL_5_h PATH_SHADERS "system/gblur/kernel5h"
#define GAUSSIAN_KERNEL_5_v PATH_SHADERS "system/gblur/kernel5v"
#define GAUSSIAN_KERNEL_7_h PATH_SHADERS "system/gblur/kernel7h"
#define GAUSSIAN_KERNEL_7_v PATH_SHADERS "system/gblur/kernel7v"
#define GAUSSIAN_KERNEL_9_h PATH_SHADERS "system/gblur/kernel9h"
#define GAUSSIAN_KERNEL_9_v PATH_SHADERS "system/gblur/kernel9v"
#define GAUSSIAN_KERNEL_15_h PATH_SHADERS "system/gblur/kernel15h"
#define GAUSSIAN_KERNEL_15_v PATH_SHADERS "system/gblur/kernel15v"
#define GAUSSIAN_KERNEL_21_h PATH_SHADERS "system/gblur/kernel21h"
#define GAUSSIAN_KERNEL_21_v PATH_SHADERS "system/gblur/kernel21v"
#define GAUSSIAN_KERNEL_31_h PATH_SHADERS "system/gblur/kernel31h"
#define GAUSSIAN_KERNEL_31_v PATH_SHADERS "system/gblur/kernel31v"

#define GAUSSIAN_KERNEL_3_h_dd PATH_SHADERS "system/gblur/kernel3h_dd"
#define GAUSSIAN_KERNEL_3_v_dd PATH_SHADERS "system/gblur/kernel3v_dd"
#define GAUSSIAN_KERNEL_5_h_dd PATH_SHADERS "system/gblur/kernel5h_dd"
#define GAUSSIAN_KERNEL_5_v_dd PATH_SHADERS "system/gblur/kernel5v_dd"
#define GAUSSIAN_KERNEL_7_h_dd PATH_SHADERS "system/gblur/kernel7h_dd"
#define GAUSSIAN_KERNEL_7_v_dd PATH_SHADERS "system/gblur/kernel7v_dd"
#define GAUSSIAN_KERNEL_9_h_dd PATH_SHADERS "system/gblur/kernel9h_dd"
#define GAUSSIAN_KERNEL_9_v_dd PATH_SHADERS "system/gblur/kernel9v_dd"
#define GAUSSIAN_KERNEL_15_h_dd PATH_SHADERS "system/gblur/kernel15h_dd"
#define GAUSSIAN_KERNEL_15_v_dd PATH_SHADERS "system/gblur/kernel15v_dd"
#define GAUSSIAN_KERNEL_21_h_dd PATH_SHADERS "system/gblur/kernel21h_dd"
#define GAUSSIAN_KERNEL_21_v_dd PATH_SHADERS "system/gblur/kernel21v_dd"
#define GAUSSIAN_KERNEL_31_h_dd PATH_SHADERS "system/gblur/kernel31h_dd"
#define GAUSSIAN_KERNEL_31_v_dd PATH_SHADERS "system/gblur/kernel31v_dd"

	enum eBlurWhat
	{
		GB_ONLYMIP0 = 0,
		GB_ALLMIPS,
		GB_ALLMIPSBUT0
	};

	enum eKernel
	{
		GB_KERNEL3 = 0,
		GB_KERNEL5,
		GB_KERNEL7,
		GB_KERNEL9,
		GB_KERNEL15,
		GB_KERNEL21,
		GB_KERNEL31,
	};

	class ScenePipeline;

	class GaussianBlur
	{
	public:
		GaussianBlur();
		~GaussianBlur();
		
		void Close();
		bool Init(int width, int height, DXGI_FORMAT format, eKernel kernel, eBlurWhat mipsBlur, bool depthDependent = false);

		void blur(RenderTarget* inout, uint8_t id = 0, ID3D11ShaderResourceView* depth = nullptr);
		void blur(RenderTarget* in, RenderTarget* out, int in_id = 0, int out_id = 0, ID3D11ShaderResourceView* depth = nullptr);

	protected:
		Render* m_render;
		RenderTarget* horzPass;
		ScreenPlane* sp_horz;
		ScreenPlane* sp_vert;
		ScenePipeline* g_scene;
		eBlurWhat blurMips;
		bool isDepthDependent;
	};
}