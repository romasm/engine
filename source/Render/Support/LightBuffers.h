#pragma once

#include "Common.h"

#define LIGHT_SPOT_FRAME_MAX 1024
#define LIGHT_POINT_FRAME_MAX 1024

#define LIGHT_DIR_FRAME_MAX 8
#define LIGHT_DIR_NUM_CASCADES 4

#define CASTER_SPOT_FRAME_MAX 256
#define CASTER_POINT_FRAME_MAX 128

namespace EngineCore
{	

#define TOTAL_LIGHT_COUNT (LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX + CASTER_POINT_FRAME_MAX + LIGHT_DIR_FRAME_MAX)

#define SPOT_L_ID(i) (i + 0)
#define SPOT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX)
#define POINT_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX)
#define POINT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX)
#define DIR_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX + CASTER_POINT_FRAME_MAX)

	typedef int32_t LightsIDs[TOTAL_LIGHT_COUNT];

	static_assert(TOTAL_LIGHT_COUNT * sizeof(int32_t) <= 64 * 1024, "LightsIDs > 4096");

	struct LightsCount
	{
		int32_t spot_count;
		int32_t point_count;
		int32_t caster_spot_count;
		int32_t caster_point_count;

		int32_t dir_count;
		int32_t envProbsCountHQ;
		int32_t envProbsCountSQ;
		int32_t envProbsCountLQ;
	};

	struct SpotLightBuffer
	{
		Vector4 Type;
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
		Vector4 DirUpAreaX;
		Vector4 DirSideAreaY;
		Vector4 VirtposAreaZ;
	};

	struct SpotCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
		Vector4 DirUpAreaX;
		Vector4 DirSideAreaY;
		Vector4 VirtposAreaZ;
		Vector4 ShadowmapAdress;
		Vector4 ShadowmapParamsType;
		Vector4 farNear;
		XMMATRIX matViewProj;
	};

	struct PointLightBuffer
	{
		Vector4 Type;
		Vector4 PosRange;
		Vector4 Color;
		Vector4 AreaInfo;
		Vector4 DirAreaA;
	};

	struct PointCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorShParams;
		Vector4 AreaInfo;
		Vector4 DirAreaA;
		Vector4 ShadowmapParams0;
		Vector4 ShadowmapParams1Type;
		Vector4 ShadowmapAdress0;
		Vector4 ShadowmapAdress1;
		Vector4 ShadowmapAdress2;
		Vector4 ShadowmapAdress3;
		Vector4 ShadowmapAdress4;
		Vector4 ShadowmapAdress5;
		Vector4 farNear;
		XMMATRIX matProj;
		XMMATRIX matView;
	};

	struct DirLightBuffer
	{
		Vector4 ColorAreaX;
		Vector4 DirAreaY;
		Vector4 Pos0;
		Vector4 Pos1;
		Vector4 Pos2;
		Vector4 Pos3;
		Vector4 ShadowmapAdress0;
		Vector4 ShadowmapAdress1;
		Vector4 ShadowmapAdress2;
		Vector4 ShadowmapAdress3;
		XMMATRIX matViewProj0;
		XMMATRIX matViewProj1;
		XMMATRIX matViewProj2;
		XMMATRIX matViewProj3;
	};

	// env probes
	
	enum EnvParallaxType
	{
		EP_PARALLAX_NONE = 0,
		EP_PARALLAX_SPHERE = 1,
		EP_PARALLAX_BOX = 2
	};

	enum EnvProbQuality
	{
		EP_HIGH = 0,
		EP_STANDART,
		EP_LOW,
		EP_QUAL_COUNT
	};

	struct EnvProbData 
	{
		uint32_t probId;
		EnvProbQuality quality;
		Vector3 position;
		uint32_t mips;
		float distance;
		float fade;
		EnvParallaxType type;
		Vector3 offset;
		Vector3 shape;
		XMMATRIX invTransform;
		float priorityDist;
		uint32_t priority;

		EnvProbData() {}

		EnvProbData(uint32_t pId, EnvProbQuality q, const Vector3& pos, uint32_t m, float dist, float f, 
			EnvParallaxType t, const Vector3& o, const Vector3& sh, const XMMATRIX& invT, uint32_t p) :
		probId(pId), quality(q), position(pos), mips(m), distance(dist), fade(f), type(t), 
			offset(o), shape(sh), invTransform(invT), priority(p)
		{
			priorityDist = 0;
		}
	};

	struct EnvProbRenderData
	{
		Vector4 positionDistance;
		Vector4 offsetFade;
		Vector4 mipsTypeAdressPriority;
		Vector4 shape;
		XMMATRIX invTransform;

		EnvProbRenderData(){}

		EnvProbRenderData(const Vector3& pos, float dist, const Vector3& offset, float fade, uint32_t mips,
			EnvParallaxType type, int32_t adress, uint32_t p, const Vector3& sh, const XMMATRIX& invT) :
				positionDistance(pos.x, pos.y, pos.z, dist), offsetFade(offset.x, offset.y, offset.z, fade), 
				mipsTypeAdressPriority(float(mips), float(type), float(adress), float(p)), shape(sh.x, sh.y, sh.z, 0), invTransform(invT)
		{}
	};
}