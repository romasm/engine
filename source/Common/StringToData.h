#pragma once

#include "stdafx.h"
#include "DataTypes.h"
#include "MaterialData.h"

#define PROC_STRING(prefix, string) if(str == L#string) return prefix##string;

namespace StringToData
{
	inline UINT8 GetRTWriteMask( const wstring& str )
	{
		if(str.empty())
			return D3D11_COLOR_WRITE_ENABLE_ALL;

		PROC_STRING(D3D11_COLOR_WRITE_ENABLE_, ALL)
		PROC_STRING(D3D11_COLOR_WRITE_ENABLE_, RED)
		PROC_STRING(D3D11_COLOR_WRITE_ENABLE_, GREEN)
		PROC_STRING(D3D11_COLOR_WRITE_ENABLE_, BLUE)
		PROC_STRING(D3D11_COLOR_WRITE_ENABLE_, ALPHA)

		return D3D11_COLOR_WRITE_ENABLE_ALL;
	};

	inline D3D11_TEXTURE_ADDRESS_MODE GetAddressType( const wstring& str )
	{
		if(str.empty())
			return D3D11_TEXTURE_ADDRESS_WRAP;

		PROC_STRING(D3D11_TEXTURE_ADDRESS_, CLAMP)
		PROC_STRING(D3D11_TEXTURE_ADDRESS_, WRAP)
		PROC_STRING(D3D11_TEXTURE_ADDRESS_, BORDER)
		PROC_STRING(D3D11_TEXTURE_ADDRESS_, MIRROR)
		PROC_STRING(D3D11_TEXTURE_ADDRESS_, MIRROR_ONCE)

		return D3D11_TEXTURE_ADDRESS_WRAP;
	};

	inline D3D11_COMPARISON_FUNC GetCompareFunc( const wstring& str )
	{
		if(str.empty())
			return D3D11_COMPARISON_ALWAYS;
		
		PROC_STRING(D3D11_COMPARISON_, ALWAYS)
		PROC_STRING(D3D11_COMPARISON_, NEVER)
		PROC_STRING(D3D11_COMPARISON_, LESS)
		PROC_STRING(D3D11_COMPARISON_, EQUAL)
		PROC_STRING(D3D11_COMPARISON_, LESS_EQUAL)
		PROC_STRING(D3D11_COMPARISON_, GREATER)
		PROC_STRING(D3D11_COMPARISON_, NOT_EQUAL)
		PROC_STRING(D3D11_COMPARISON_, GREATER_EQUAL)

		return D3D11_COMPARISON_ALWAYS;
	};

	inline D3D11_FILTER GetFilter( const wstring& str )
	{
		if(str.empty())
			return D3D11_FILTER_MIN_MAG_MIP_POINT;
		
		PROC_STRING(D3D11_FILTER_, MIN_MAG_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, MIN_MAG_POINT_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, MIN_POINT_MAG_LINEAR_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, MIN_POINT_MAG_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, MIN_LINEAR_MAG_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, MIN_LINEAR_MAG_POINT_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, MIN_MAG_LINEAR_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, MIN_MAG_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, ANISOTROPIC)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_MAG_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_MAG_POINT_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_POINT_MAG_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_LINEAR_MAG_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
		PROC_STRING(D3D11_FILTER_, COMPARISON_MIN_MAG_MIP_LINEAR)
		PROC_STRING(D3D11_FILTER_, COMPARISON_ANISOTROPIC)

		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	};

	inline D3D11_STENCIL_OP GetStencilOp( const wstring& str )
	{
		if(str.empty())
			return D3D11_STENCIL_OP_KEEP;
		
		PROC_STRING(D3D11_STENCIL_OP_, KEEP)
		PROC_STRING(D3D11_STENCIL_OP_, ZERO)
		PROC_STRING(D3D11_STENCIL_OP_, REPLACE)
		PROC_STRING(D3D11_STENCIL_OP_, INCR_SAT)
		PROC_STRING(D3D11_STENCIL_OP_, DECR_SAT)
		PROC_STRING(D3D11_STENCIL_OP_, INVERT)
		PROC_STRING(D3D11_STENCIL_OP_, INCR)
		PROC_STRING(D3D11_STENCIL_OP_, DECR)

		return D3D11_STENCIL_OP_KEEP;
	};

	inline D3D11_BLEND_OP GetBlendOp( const wstring& str )
	{
		if(str.empty())
			return D3D11_BLEND_OP_ADD;
		
		PROC_STRING(D3D11_BLEND_OP_, ADD)
		PROC_STRING(D3D11_BLEND_OP_, SUBTRACT)
		PROC_STRING(D3D11_BLEND_OP_, REV_SUBTRACT)
		PROC_STRING(D3D11_BLEND_OP_, MIN)
		PROC_STRING(D3D11_BLEND_OP_, MAX)

		return D3D11_BLEND_OP_ADD;
	};

	inline D3D11_BLEND GetBlend( const wstring& str )
	{
		if(str.empty())
			return D3D11_BLEND_ZERO;
		
		PROC_STRING(D3D11_BLEND_, ZERO)
		PROC_STRING(D3D11_BLEND_, ONE)
		PROC_STRING(D3D11_BLEND_, SRC_COLOR)
		PROC_STRING(D3D11_BLEND_, INV_SRC_COLOR)
		PROC_STRING(D3D11_BLEND_, SRC_ALPHA)
		PROC_STRING(D3D11_BLEND_, INV_SRC_ALPHA)
		PROC_STRING(D3D11_BLEND_, DEST_ALPHA)
		PROC_STRING(D3D11_BLEND_, INV_DEST_ALPHA)
		PROC_STRING(D3D11_BLEND_, DEST_COLOR)
		PROC_STRING(D3D11_BLEND_, INV_DEST_COLOR)
		PROC_STRING(D3D11_BLEND_, SRC_ALPHA_SAT)
		PROC_STRING(D3D11_BLEND_, BLEND_FACTOR)
		PROC_STRING(D3D11_BLEND_, INV_BLEND_FACTOR)
		PROC_STRING(D3D11_BLEND_, SRC1_COLOR)
		PROC_STRING(D3D11_BLEND_, INV_SRC1_COLOR)
		PROC_STRING(D3D11_BLEND_, SRC1_ALPHA)
		PROC_STRING(D3D11_BLEND_, INV_SRC1_ALPHA)

		return D3D11_BLEND_ZERO;
	};

	inline D3D11_FILL_MODE GetFill( const wstring& str )
	{
		if(str.empty())
			return D3D11_FILL_SOLID;
		
		PROC_STRING(D3D11_FILL_, SOLID)
		PROC_STRING(D3D11_FILL_, WIREFRAME)

		return D3D11_FILL_SOLID;
	};

	inline D3D11_CULL_MODE GetCull( const wstring& str )
	{
		if(str.empty())
			return D3D11_CULL_BACK;
		
		PROC_STRING(D3D11_CULL_, NONE)
		PROC_STRING(D3D11_CULL_, FRONT)
		PROC_STRING(D3D11_CULL_, BACK)

		return D3D11_CULL_BACK;
	};

	inline EngineCore::RENDER_QUEUES GetQueueID( const wstring& str )
	{
		if(str.empty())
			return EngineCore::RENDER_QUEUES::SC_OPAQUE;
		
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_OPAQUE)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_2D)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_2D_FONT)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_ALPHATEST)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_ALPHA)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_TRANSPARENT)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_3D)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_3D_OVERLAY)

		return EngineCore::RENDER_QUEUES::SC_OPAQUE;
	};

	inline EngineCore::TECHNIQUES GetTechID( const wstring& str )
	{
		if(str.empty())
			return EngineCore::TECHNIQUES::TECHNIQUE_DEFAULT;
		
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_DEFAULT)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SHADOW)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_VOXEL)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_PREPASS)

		return EngineCore::TECHNIQUES::TECHNIQUE_DEFAULT;
	};
}