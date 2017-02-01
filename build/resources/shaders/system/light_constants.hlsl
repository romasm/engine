
#define ROUGHNESS_REMAP 1

#define NORMAL_CLAMP 0.01
#define NORMAL_CLAMP_DOUBLE NORMAL_CLAMP * 2
#define NORMAL_CLAMP_MUL 0.5 / NORMAL_CLAMP

#define LIGHT_SPOT_FRAME_MAX 128
#define LIGHT_SPOT_DISK_FRAME_MAX 128
#define LIGHT_SPOT_RECT_FRAME_MAX 128
#define LIGHT_POINT_FRAME_MAX 128
#define LIGHT_POINT_SPHERE_FRAME_MAX 128
#define LIGHT_POINT_TUBE_FRAME_MAX 128
#define LIGHT_DIR_FRAME_MAX 8
#define LIGHT_DIR_NUM_CASCADES 4

#define CASTER_SPOT_FRAME_MAX 64
#define CASTER_SPOT_DISK_FRAME_MAX 64
#define CASTER_SPOT_RECT_FRAME_MAX 64
#define CASTER_POINT_FRAME_MAX 32
#define CASTER_POINT_SPHERE_FRAME_MAX 32
#define CASTER_POINT_TUBE_FRAME_MAX 32

#define SHADOW_NEARCLIP 0.01

#define SHADOWS_BUFFER_RES 4096
#define SHADOWS_MAXRES 2048
#define SHADOWS_MINRES 64

#define SHADOWS_RES_BIAS_SCALE SHADOWS_MAXRES / 2

struct LightComponents
{
    float3 diffuse;
	float3 specular;
	float3 scattering;
};

struct ShadowHelpers
{
    float DoUL;
	float3 L;
};

struct LightsIDs
{
	uint SpotLightsIDs[LIGHT_SPOT_FRAME_MAX];
	uint DiskLightsIDs[LIGHT_SPOT_DISK_FRAME_MAX];
	uint RectLightsIDs[LIGHT_SPOT_RECT_FRAME_MAX];
	
	uint SpotCastersIDs[CASTER_SPOT_FRAME_MAX];
	uint DiskCastersIDs[CASTER_SPOT_DISK_FRAME_MAX];
	uint RectCastersIDs[CASTER_SPOT_RECT_FRAME_MAX];

	uint PointLightsIDs[LIGHT_POINT_FRAME_MAX];
	uint SphereLightsIDs[LIGHT_POINT_SPHERE_FRAME_MAX];
	uint TubeLightsIDs[LIGHT_POINT_TUBE_FRAME_MAX];

	uint PointCastersIDs[CASTER_POINT_FRAME_MAX];
	uint SphereCastersIDs[CASTER_POINT_SPHERE_FRAME_MAX];
	uint TubeCastersIDs[CASTER_POINT_TUBE_FRAME_MAX];

	uint DirLightsIDs[LIGHT_DIR_FRAME_MAX];
};

// for deffered
struct SpotLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
};

struct DiskLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 AreaInfoEmpty;
	float4 VirtposEmpty;
};

struct RectLightBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 DirUpAreaX;
	float4 DirSideAreaY;
	float4 VirtposAreaZ;
};

struct SpotCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 ShadowmapAdress;
	float4 ShadowmapParams;
	matrix matViewProj;
};

struct DiskCasterBuffer
{
	float4 PosRange;
	float4 ColorConeX;
	float4 DirConeY;
	float4 AreaInfoEmpty;
	float4 VirtposEmpty;
	float4 ShadowmapAdress;
	float4 ShadowmapParams;
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
	matrix matViewProj;
};

struct PointLightBuffer
{
	float4 PosRange;
	float4 Color;
};

struct SphereLightBuffer
{
	float4 PosRange;
	float4 ColorEmpty;
	float4 AreaInfoEmpty;
};

struct TubeLightBuffer
{
	float4 PosRange;
	float4 ColorEmpty;
	float4 AreaInfo;
	float4 DirAreaA;
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
	matrix matProj;
};

struct SphereCasterBuffer
{
	float4 PosRange;
	float4 ColorShParams;
	float4 AreaInfoShParams;
	float4 ShadowmapParams;
	float4 ShadowmapAdress0;
	float4 ShadowmapAdress1;
	float4 ShadowmapAdress2;
	float4 ShadowmapAdress3;
	float4 ShadowmapAdress4;
	float4 ShadowmapAdress5;
	matrix matProj;
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
	matrix matProj;
	matrix matView;
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