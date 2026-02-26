#pragma once

#include "stdafx.h"
#include "DataTypes.h"
#include "MaterialData.h"

#define PROC_STRING(prefix, string) if(str == #string) return prefix##string;

namespace StringToData
{
	inline uint8_t GetRTWriteMask( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return ColorWriteAll;

		if(str == "ALL")   return ColorWriteAll;
		if(str == "RED")   return ColorWriteRed;
		if(str == "GREEN") return ColorWriteGreen;
		if(str == "BLUE")  return ColorWriteBlue;
		if(str == "ALPHA") return ColorWriteAlpha;

		return ColorWriteAll;
	};

	inline EngineCore::RHI::TextureAddressMode GetAddressType( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return AddressWrap;

		if(str == "CLAMP")       return AddressClamp;
		if(str == "WRAP")        return AddressWrap;
		if(str == "BORDER")      return AddressBorder;
		if(str == "MIRROR")      return AddressMirror;
		if(str == "MIRROR_ONCE") return AddressClamp; // no MIRROR_ONCE in RHI, fallback

		return AddressWrap;
	};

	inline EngineCore::RHI::ComparisonFunc GetCompareFunc( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return ComparisonAlways;

		if(str == "ALWAYS")        return ComparisonAlways;
		if(str == "NEVER")         return ComparisonNever;
		if(str == "LESS")          return ComparisonLess;
		if(str == "EQUAL")         return ComparisonEqual;
		if(str == "LESS_EQUAL")    return ComparisonLessEqual;
		if(str == "GREATER")       return ComparisonGreater;
		if(str == "NOT_EQUAL")     return ComparisonNotEqual;
		if(str == "GREATER_EQUAL") return ComparisonGreaterEqual;

		return ComparisonAlways;
	};

	inline EngineCore::RHI::Filter GetFilter( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return FilterMinMagMipPoint;

		if(str == "MIN_MAG_MIP_POINT")                      return FilterMinMagMipPoint;
		if(str == "MIN_MAG_POINT_MIP_LINEAR")               return FilterMinMagPointMipLinear;
		if(str == "MIN_POINT_MAG_LINEAR_MIP_POINT")         return FilterMinPointMagLinearMipPoint;
		if(str == "MIN_POINT_MAG_MIP_LINEAR")               return FilterMinPointMagMipLinear;
		if(str == "MIN_LINEAR_MAG_MIP_POINT")               return FilterMinLinearMagMipPoint;
		if(str == "MIN_LINEAR_MAG_POINT_MIP_LINEAR")        return FilterMinLinearMagPointMipLinear;
		if(str == "MIN_MAG_LINEAR_MIP_POINT")               return FilterMinMagLinearMipPoint;
		if(str == "MIN_MAG_MIP_LINEAR")                     return FilterMinMagMipLinear;
		if(str == "ANISOTROPIC")                             return FilterAnisotropic;
		if(str == "COMPARISON_MIN_MAG_MIP_POINT")           return FilterMinMagMipPoint;
		if(str == "COMPARISON_MIN_MAG_POINT_MIP_LINEAR")    return FilterMinMagPointMipLinear;
		if(str == "COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT") return FilterMinPointMagLinearMipPoint;
		if(str == "COMPARISON_MIN_POINT_MAG_MIP_LINEAR")    return FilterMinPointMagMipLinear;
		if(str == "COMPARISON_MIN_LINEAR_MAG_MIP_POINT")    return FilterMinLinearMagMipPoint;
		if(str == "COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR") return FilterMinLinearMagPointMipLinear;
		if(str == "COMPARISON_MIN_MAG_LINEAR_MIP_POINT")    return FilterMinMagLinearMipPoint;
		if(str == "COMPARISON_MIN_MAG_MIP_LINEAR")          return FilterComparisonMinMagMipLinear;
		if(str == "COMPARISON_ANISOTROPIC")                  return FilterComparisonAnisotropic;

		return FilterMinMagMipPoint;
	};

	inline EngineCore::RHI::StencilOp GetStencilOp( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return StencilKeep;

		if(str == "KEEP")     return StencilKeep;
		if(str == "ZERO")     return StencilZero;
		if(str == "REPLACE")  return StencilReplace;
		if(str == "INCR_SAT") return StencilIncrSat;
		if(str == "DECR_SAT") return StencilDecrSat;
		if(str == "INVERT")   return StencilInvert;
		if(str == "INCR")     return StencilIncr;
		if(str == "DECR")     return StencilDecr;

		return StencilKeep;
	};

	inline EngineCore::RHI::BlendOp GetBlendOp( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return BlendOpAdd;

		if(str == "ADD")          return BlendOpAdd;
		if(str == "SUBTRACT")     return BlendOpSubtract;
		if(str == "REV_SUBTRACT") return BlendOpRevSubtract;
		if(str == "MIN")          return BlendOpMin;
		if(str == "MAX")          return BlendOpMax;

		return BlendOpAdd;
	};

	inline EngineCore::RHI::Blend GetBlend( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return BlendZero;

		if(str == "ZERO")             return BlendZero;
		if(str == "ONE")              return BlendOne;
		if(str == "SRC_COLOR")        return BlendSrcColor;
		if(str == "INV_SRC_COLOR")    return BlendInvSrcColor;
		if(str == "SRC_ALPHA")        return BlendSrcAlpha;
		if(str == "INV_SRC_ALPHA")    return BlendInvSrcAlpha;
		if(str == "DEST_ALPHA")       return BlendDestAlpha;
		if(str == "INV_DEST_ALPHA")   return BlendInvDestAlpha;
		if(str == "DEST_COLOR")       return BlendDestColor;
		if(str == "INV_DEST_COLOR")   return BlendInvDestColor;
		if(str == "SRC_ALPHA_SAT")    return BlendSrcAlphaSat;
		if(str == "BLEND_FACTOR")     return BlendBlendFactor;
		if(str == "INV_BLEND_FACTOR") return BlendInvBlendFactor;

		return BlendZero;
	};

	inline EngineCore::RHI::FillMode GetFill( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return FillSolid;

		if(str == "SOLID")     return FillSolid;
		if(str == "WIREFRAME") return FillWireframe;

		return FillSolid;
	};

	inline EngineCore::RHI::CullMode GetCull( const string& str )
	{
		using namespace EngineCore::RHI;
		if(str.empty())
			return CullBack;

		if(str == "NONE")  return CullNone;
		if(str == "FRONT") return CullFront;
		if(str == "BACK")  return CullBack;

		return CullBack;
	};

	inline EngineCore::RENDER_QUEUES GetQueueID( const string& str )
	{
		if(str.empty())
			return EngineCore::RENDER_QUEUES::SC_OPAQUE;

		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_OPAQUE)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_2D)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_2D_FONT)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_ALPHATEST)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_FORWARD)
		PROC_STRING(EngineCore::RENDER_QUEUES::, SC_TRANSPARENT)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_3D)
		PROC_STRING(EngineCore::RENDER_QUEUES::, GUI_3D_OVERLAY)

		return EngineCore::RENDER_QUEUES::SC_OPAQUE;
	};

	inline EngineCore::TECHNIQUES GetTechID( const string& str )
	{
		if(str.empty())
			return EngineCore::TECHNIQUES::TECHNIQUE_DEFAULT;

		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_DEFAULT)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SKIN_DEFAULT)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SHADOW)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SKIN_SHADOW)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_VOXEL)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SKIN_VOXEL)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_PREPASS)
		PROC_STRING(EngineCore::TECHNIQUES::, TECHNIQUE_SKIN_PREPASS)

		return EngineCore::TECHNIQUES::TECHNIQUE_DEFAULT;
	};
}
