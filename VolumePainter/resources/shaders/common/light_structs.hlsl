
#define LIGHT_TYPE_POINT	0
#define LIGHT_TYPE_SPHERE	1
#define LIGHT_TYPE_TUBE		2
#define LIGHT_TYPE_SPOT		3
#define LIGHT_TYPE_DISK		4
#define LIGHT_TYPE_RECT		5

#define LIGHT_SPOT_FRAME_MAX 1024
#define LIGHT_POINT_FRAME_MAX 1024

#define LIGHT_DIR_FRAME_MAX 8
#define LIGHT_DIR_NUM_CASCADES 4

#define CASTER_SPOT_FRAME_MAX 256
#define CASTER_POINT_FRAME_MAX 128

#define TOTAL_LIGHT_COUNT (LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX + CASTER_POINT_FRAME_MAX + LIGHT_DIR_FRAME_MAX)

#define SPOT_L_ID(i) (i + 0)
#define SPOT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX)
#define POINT_L_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX)
#define POINT_C_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX)
#define DIR_ID(i) (i + LIGHT_SPOT_FRAME_MAX + CASTER_SPOT_FRAME_MAX + LIGHT_POINT_FRAME_MAX + CASTER_POINT_FRAME_MAX)

#define SHADOW_NEARCLIP 0.01

#define SHADOWS_BUFFER_RES 4096
#define SHADOWS_MAXRES 2048
#define SHADOWS_MINRES 64

#define SHADOWS_RES_BIAS_SCALE SHADOWS_MAXRES / 2

// from GlobalLightSystem.h
#define DIRLIGHT_Z_CASCADE_0 2500.0f
#define DIRLIGHT_Z_CASCADE_1 4000.0f
#define DIRLIGHT_Z_CASCADE_2 7500.0f
#define DIRLIGHT_Z_CASCADE_3 10000.0f

struct LightComponents
{
    float3 diffuse;
	float3 specular;
	float3 scattering;

	void Append(in LightComponents other)
	{
		diffuse += other.diffuse;
		specular += other.specular;
		scattering += other.scattering;
	}

	void AppendShadowed(in LightComponents other, in float lightAmount)
	{
		diffuse += other.diffuse * lightAmount;
		specular += other.specular * lightAmount;
		scattering += other.scattering;
	}
};

struct LightPrepared
{
	float3 unnormL;
	float3 L;
    float DoUL;
};

typedef int LightsIDs[TOTAL_LIGHT_COUNT];

struct LightsCount
{
	int spot_count;
	int point_count;
	int caster_spot_count;
	int caster_point_count;

	int dir_count;
	int envProbsCountHQ;
	int envProbsCountSQ;
	int envProbsCountLQ;
};

// for deffered
struct SpotCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 DirUpAreaX;
	float4 DirSideAreaY;
	float4 VirtposAreaZ;
	float4 ShadowmapAdress;
	float4 ShadowmapParamsType;
	float4 farNear;
	matrix matViewProj;
};

struct SpotLightBuffer
{
	float4 Type;
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 DirUpAreaX;
	float4 DirSideAreaY;
	float4 VirtposAreaZ;

	void Construct(in SpotCasterBuffer longData)
	{
		Type.x = longData.ShadowmapParamsType.w;
		PosRange = longData.PosRange;
		ColorConeX = longData.ColorConeX;
		DirConeY = longData.DirConeY;
		DirUpAreaX = longData.DirUpAreaX;
		DirSideAreaY = longData.DirSideAreaY;
		VirtposAreaZ = longData.VirtposAreaZ;
	}
};

struct PointCasterBuffer
{
	float4 PosRange;
	float4 ColorShParams;
	float4 AreaInfo;
	float4 DirAreaA;
	float4 ShadowmapParams0;
	float4 ShadowmapParams1Type;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	float4 ShadowmapAdress4;
	float4 ShadowmapAdress5;
	float4 farNear;
	matrix matProj;
	matrix matView;
};

struct PointLightBuffer
{
	float4 Type;
	float4 PosRange;
	float4 Color;
	float4 AreaInfo;
	float4 DirAreaA;

	void Construct(in PointCasterBuffer longData)
	{
		Type.x = longData.ShadowmapParams1Type.z;
		PosRange = longData.PosRange;
		Color = longData.ColorShParams;
		AreaInfo = longData.AreaInfo;
		DirAreaA = longData.DirAreaA;
	}
};

struct DirLightBuffer
{
	float4 ColorAreaX;
	float4 DirAreaY;
	float4 Pos0;
	float4 Pos1;
	float4 Pos2;
	float4 Pos3;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	matrix matViewProj0;
	matrix matViewProj1;
	matrix matViewProj2;
	matrix matViewProj3;
};

// for voxel inject
struct SpotVoxelBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 Virtpos;
	float4 ShadowmapAdress;
	float4 ShadowmapHPixProjNearclip;
	matrix matViewProj;
};

struct PointVoxelBuffer
{
	float4 PosRange;
	float4 ColorShadowmapProj;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	float4 ShadowmapAdress4;
	float4 ShadowmapAdress5;
	float4 ShadowmapHPix0;
	float4 ShadowmapHPix1;
	matrix matProj;
	matrix matView;
};

struct DirVoxelBuffer
{
	float4 Color;
	float4 Dir;
	float4 PosHPix0;
	float4 PosHPix1;
	float4 PosHPix2;
	float4 PosHPix3;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	matrix ViewProj0;
	matrix ViewProj1;
	matrix ViewProj2;
	matrix ViewProj3;
};

// env probes

#define ENVPROBS_FRAME_COUNT_HQ 8
#define ENVPROBS_FRAME_COUNT_SQ 32
#define ENVPROBS_FRAME_COUNT_LQ 64

#define ENVPROBS_PARALLAX_NONE 0
#define ENVPROBS_PARALLAX_SPHERE 1
#define ENVPROBS_PARALLAX_BOX 2

struct EnvProbRenderData
{
	float4 positionDistance;
	float4 offsetFade;
	float4 mipsTypeAdressPriority;
	float4 shape;
	matrix invTransform;
};