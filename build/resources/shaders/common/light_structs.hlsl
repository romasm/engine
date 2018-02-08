
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
	int disk_count;
	int rect_count;
	int point_count;

	int sphere_count;
	int tube_count;
	int dir_count;
	int caster_spot_count;

	int caster_disk_count;
	int caster_rect_count;
	int caster_point_count;
	int caster_sphere_count;

	int caster_tube_count;
	int envProbsCountHQ;
	int envProbsCountSQ;
	int envProbsCountLQ;
};

// for deffered

struct DiskCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 AreaInfoEmpty;
	float4 VirtposEmpty;
	float4 ShadowmapAdress;
	float4 ShadowmapParams;
	float4 farNear;
	matrix matViewProj;
};

struct RectCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 DirUpAreaX;
	float4 DirSideAreaY;
	float4 VirtposAreaZ;
	float4 ShadowmapAdress;
	float4 ShadowmapParams;
	float4 farNear;
	matrix matViewProj;
};

struct SpotCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 ShadowmapAdress;
	float4 ShadowmapParams;
	float4 farNear;
	matrix matViewProj;

	void ConstructDisk(in DiskCasterBuffer shapeData)
	{
		PosRange = shapeData.PosRange;
		ColorConeX = shapeData.ColorConeX;
		DirConeY = shapeData.DirConeY;
		ShadowmapAdress = shapeData.ShadowmapAdress;
		ShadowmapParams = shapeData.ShadowmapParams;
		farNear = shapeData.farNear;
		matViewProj = shapeData.matViewProj;
	}

	void ConstructRect(in RectCasterBuffer shapeData)
	{
		PosRange = shapeData.PosRange;
		ColorConeX = shapeData.ColorConeX;
		DirConeY = shapeData.DirConeY;
		ShadowmapAdress = shapeData.ShadowmapAdress;
		ShadowmapParams = shapeData.ShadowmapParams;
		farNear = shapeData.farNear;
		matViewProj = shapeData.matViewProj;
	}
};

struct SpotLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;

	void Construct(in SpotCasterBuffer longData)
	{
		PosRange = longData.PosRange;
		ColorConeX = longData.ColorConeX;
		DirConeY = longData.DirConeY;
	}
};

struct DiskLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 AreaInfoEmpty;
	float4 VirtposEmpty;

	void Construct(in DiskCasterBuffer longData)
	{
		PosRange = longData.PosRange;
		ColorConeX = longData.ColorConeX;
		DirConeY = longData.DirConeY;
		AreaInfoEmpty = longData.AreaInfoEmpty;
		VirtposEmpty = longData.VirtposEmpty;
	}
};

struct RectLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 DirUpAreaX;
	float4 DirSideAreaY;
	float4 VirtposAreaZ;

	void Construct(in RectCasterBuffer longData)
	{
		PosRange = longData.PosRange;
		ColorConeX = longData.ColorConeX;
		DirConeY = longData.DirConeY;
		DirUpAreaX = longData.DirUpAreaX;
		DirSideAreaY = longData.DirSideAreaY;
		VirtposAreaZ = longData.VirtposAreaZ;
	}
};

struct SphereCasterBuffer
{
	float4 PosRange;
	float4 ColorShParams;
	float4 AreaInfo;
	float4 ShadowmapParams0;
	float4 ShadowmapParams1;
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

struct TubeCasterBuffer
{
	float4 PosRange;
	float4 ColorShParams;
	float4 AreaInfo;
	float4 DirAreaA;
	float4 ShadowmapParams0;
	float4 ShadowmapParams1;
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

struct PointCasterBuffer
{
	float4 PosRange;
	float4 ColorShParams;
	float4 ShadowmapParams0;
	float4 ShadowmapParams1;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	float4 ShadowmapAdress4;
	float4 ShadowmapAdress5;
	float4 farNear;
	matrix matProj;
	matrix matView;

	void ConstructSphere(in SphereCasterBuffer shapeData)
	{
		PosRange = shapeData.PosRange;
		ColorShParams = shapeData.ColorShParams;
		ShadowmapParams0 = shapeData.ShadowmapParams0;
		ShadowmapParams1 = shapeData.ShadowmapParams1;
		ShadowmapAdress0 = shapeData.ShadowmapAdress0;
		ShadowmapAdress1 = shapeData.ShadowmapAdress1;
		ShadowmapAdress2 = shapeData.ShadowmapAdress2;
		ShadowmapAdress3 = shapeData.ShadowmapAdress3;
		ShadowmapAdress4 = shapeData.ShadowmapAdress4;
		ShadowmapAdress5 = shapeData.ShadowmapAdress5;
		farNear = shapeData.farNear;
		matProj = shapeData.matProj;
		matView = shapeData.matView;
	}

	void ConstructTube(in TubeCasterBuffer shapeData)
	{
		PosRange = shapeData.PosRange;
		ColorShParams = shapeData.ColorShParams;
		ShadowmapParams0 = shapeData.ShadowmapParams0;
		ShadowmapParams1 = shapeData.ShadowmapParams1;
		ShadowmapAdress0 = shapeData.ShadowmapAdress0;
		ShadowmapAdress1 = shapeData.ShadowmapAdress1;
		ShadowmapAdress2 = shapeData.ShadowmapAdress2;
		ShadowmapAdress3 = shapeData.ShadowmapAdress3;
		ShadowmapAdress4 = shapeData.ShadowmapAdress4;
		ShadowmapAdress5 = shapeData.ShadowmapAdress5;
		farNear = shapeData.farNear;
		matProj = shapeData.matProj;
		matView = shapeData.matView;
	}
};

struct PointLightBuffer
{
	float4 PosRange;
	float4 Color;

	void Construct(in PointCasterBuffer longData)
	{
		PosRange = longData.PosRange;
		Color = longData.ColorShParams;
	}
};

struct SphereLightBuffer
{
	float4 PosRange;
	float4 Color;
	float4 AreaInfo;

	void Construct(in SphereCasterBuffer longData)
	{
		PosRange = longData.PosRange;
		Color = longData.ColorShParams;
		AreaInfo = longData.AreaInfo;
	}
};

struct TubeLightBuffer
{
	float4 PosRange;
	float4 Color;
	float4 AreaInfo;
	float4 DirAreaA;

	void Construct(in TubeCasterBuffer longData)
	{
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
	float4 bBox;
	matrix invTransform;
};