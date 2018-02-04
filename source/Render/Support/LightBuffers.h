#pragma once

#include "Common.h"

#define LIGHT_SPOT_FRAME_MAX 64
#define LIGHT_SPOT_DISK_FRAME_MAX 64
#define LIGHT_SPOT_RECT_FRAME_MAX 64
#define LIGHT_POINT_FRAME_MAX 64
#define LIGHT_POINT_SPHERE_FRAME_MAX 64
#define LIGHT_POINT_TUBE_FRAME_MAX 64
#define LIGHT_DIR_FRAME_MAX 8
#define LIGHT_DIR_NUM_CASCADES 4

#define CASTER_SPOT_FRAME_MAX 64
#define CASTER_SPOT_DISK_FRAME_MAX 64
#define CASTER_SPOT_RECT_FRAME_MAX 64
#define CASTER_POINT_FRAME_MAX 32
#define CASTER_POINT_SPHERE_FRAME_MAX 32
#define CASTER_POINT_TUBE_FRAME_MAX 32

#define SPOT_VOXEL_FRAME_MAX CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX
#define POINT_VOXEL_FRAME_MAX CASTER_POINT_FRAME_MAX + CASTER_POINT_SPHERE_FRAME_MAX + CASTER_POINT_TUBE_FRAME_MAX

namespace EngineCore
{	

#define TOTAL_LIGHT_COUNT (LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX + LIGHT_POINT_TUBE_FRAME_MAX + \
							CASTER_POINT_FRAME_MAX + CASTER_POINT_SPHERE_FRAME_MAX + CASTER_POINT_TUBE_FRAME_MAX + \
							LIGHT_DIR_FRAME_MAX)

#define SPOT_L_ID(i) (i + 0)
#define DISK_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX)
#define RECT_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX)

#define SPOT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX)
#define DISK_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX)
#define RECT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX)

#define POINT_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX)
#define SPHERE_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX)
#define TUBE_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX)

#define POINT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX + LIGHT_POINT_TUBE_FRAME_MAX)
#define SPHERE_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX + LIGHT_POINT_TUBE_FRAME_MAX + \
							CASTER_POINT_FRAME_MAX)
#define TUBE_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX + LIGHT_POINT_TUBE_FRAME_MAX + \
							CASTER_POINT_FRAME_MAX + CASTER_POINT_SPHERE_FRAME_MAX)

#define DIR_ID(i) (i + LIGHT_SPOT_FRAME_MAX + LIGHT_SPOT_DISK_FRAME_MAX + LIGHT_SPOT_RECT_FRAME_MAX + \
							CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX + \
							LIGHT_POINT_FRAME_MAX + LIGHT_POINT_SPHERE_FRAME_MAX + LIGHT_POINT_TUBE_FRAME_MAX + \
							CASTER_POINT_FRAME_MAX + CASTER_POINT_SPHERE_FRAME_MAX + CASTER_POINT_TUBE_FRAME_MAX)

	/*struct LightsIDs
	{		
		int32_t SpotLightsIDs[LIGHT_SPOT_FRAME_MAX];
		int32_t DiskLightsIDs[LIGHT_SPOT_DISK_FRAME_MAX];
		int32_t RectLightsIDs[LIGHT_SPOT_RECT_FRAME_MAX];
	
		int32_t SpotCastersIDs[CASTER_SPOT_FRAME_MAX];
		int32_t DiskCastersIDs[CASTER_SPOT_DISK_FRAME_MAX];
		int32_t RectCastersIDs[CASTER_SPOT_RECT_FRAME_MAX];

		int32_t PointLightsIDs[LIGHT_POINT_FRAME_MAX];
		int32_t SphereLightsIDs[LIGHT_POINT_SPHERE_FRAME_MAX];
		int32_t TubeLightsIDs[LIGHT_POINT_TUBE_FRAME_MAX];

		int32_t PointCastersIDs[CASTER_POINT_FRAME_MAX];
		int32_t SphereCastersIDs[CASTER_POINT_SPHERE_FRAME_MAX];
		int32_t TubeCastersIDs[CASTER_POINT_TUBE_FRAME_MAX];

		int32_t DirLightsIDs[LIGHT_DIR_FRAME_MAX];
	}*/

	typedef int32_t LightsIDs[TOTAL_LIGHT_COUNT];

	static_assert(TOTAL_LIGHT_COUNT * sizeof(int32_t) <= 4096, "LightsIDs > 4096");

	struct LightsCount
	{
		int32_t spot_count;
		int32_t disk_count;
		int32_t rect_count;
		int32_t point_count;

		int32_t sphere_count;
		int32_t tube_count;
		int32_t dir_count;
		int32_t caster_spot_count;

		int32_t caster_disk_count;
		int32_t caster_rect_count;
		int32_t caster_point_count;
		int32_t caster_sphere_count;

		int32_t caster_tube_count;
		int32_t _padding0;
		int32_t _padding1;
		int32_t _padding2;
	};
		
	// for deffered
	struct SpotLightBuffer
	{
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
	};

	struct DiskLightBuffer
	{
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
		Vector4 AreaInfoEmpty;
		Vector4 VirtposEmpty;
	};

	struct RectLightBuffer
	{
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
		Vector4 ShadowmapAdress;
		Vector4 ShadowmapParams;
		Vector4 farNear;
		XMMATRIX matViewProj;
	};

	struct DiskCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
		Vector4 AreaInfoEmpty;
		Vector4 VirtposEmpty;
		Vector4 ShadowmapAdress;
		Vector4 ShadowmapParams;
		Vector4 farNear;
		XMMATRIX matViewProj;
	};

	struct RectCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorConeX;
		Vector4 DirConeY;
		Vector4 DirUpAreaX;
		Vector4 DirSideAreaY;
		Vector4 VirtposAreaZ;
		Vector4 ShadowmapAdress;
		Vector4 ShadowmapParams;
		Vector4 farNear;
		XMMATRIX matViewProj;
	};

	struct PointLightBuffer
	{
		Vector4 PosRange;
		Vector4 Color;
	};

	struct SphereLightBuffer
	{
		Vector4 PosRange;
		Vector4 Color;
		Vector4 AreaInfo;
	};

	struct TubeLightBuffer
	{
		Vector4 PosRange;
		Vector4 Color;
		Vector4 AreaInfo;
		Vector4 DirAreaA;
	};

	struct PointCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorShParams;
		Vector4 ShadowmapParams0;
		Vector4 ShadowmapParams1;
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

	struct SphereCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorShParams;
		Vector4 AreaInfo;
		Vector4 ShadowmapParams0;
		Vector4 ShadowmapParams1;
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

	struct TubeCasterBuffer
	{
		Vector4 PosRange;
		Vector4 ColorShParams;
		Vector4 AreaInfo;
		Vector4 DirAreaA;
		Vector4 ShadowmapParams0;
		Vector4 ShadowmapParams1;
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
		EP_PARALLAX_SPHERE = 0,
		EP_PARALLAX_BOX = 1,
		EP_PARALLAX_NONE = 2
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
		Vector3 bBox;
		XMMATRIX invTransform;
		float priority;

		EnvProbData(uint32_t pId, EnvProbQuality q, const Vector3& pos, uint32_t m, float dist, float f, 
			EnvParallaxType t, const Vector3& o, const Vector3& bb, const XMMATRIX& invT, float p) :
		probId(pId), quality(q), position(pos), mips(m), distance(dist), fade(f), type(t), 
			offset(o), bBox(bb), invTransform(invT), priority(p)
		{}
	};

	struct EnvProbBuffer
	{
		Vector4 PositionDistance;
		Vector4 OffsetFade;
		Vector4 MipsTypeAdress;
		Vector4 BBox;
		XMMATRIX BBoxInvTransform;
	};
}