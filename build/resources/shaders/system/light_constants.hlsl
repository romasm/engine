
#define ROUGHNESS_REMAP 1

#define NORMAL_CLAMP 0.01
#define NORMAL_CLAMP_DOUBLE NORMAL_CLAMP * 2
#define NORMAL_CLAMP_MUL 0.5 / NORMAL_CLAMP

#define LIGHT_SHADOW_COUNT 64
#define LIGHT_NO_SHADOW_COUNT 128

#define ENVPROBS_COUNT 32

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_POINT_1 1
#define LIGHT_TYPE_POINT_2 2
#define LIGHT_TYPE_POINT_3 3
#define LIGHT_TYPE_POINT_4 4
#define LIGHT_TYPE_POINT_5 5
#define LIGHT_TYPE_SPOT 10
#define LIGHT_TYPE_DIR 20
#define LIGHT_TYPE_DIR_1 21
#define LIGHT_TYPE_DIR_2 22
#define LIGHT_TYPE_DIR_3 23
#define LIGHT_TYPE_DIR_4 24
#define LIGHT_TYPE_DIR_5 25
#define LIGHT_TYPE_DIR_6 26
#define LIGHT_TYPE_DIR_7 27

#define E_SHADOW_FILTER_NONE 0 
#define E_SHADOW_FILTER_PENUMBRA 1
#define E_SHADOW_FILTER_SOFTPCF 2

struct d_light
{
    float4 Pos[LIGHT_NO_SHADOW_COUNT];
	float4 Color[LIGHT_NO_SHADOW_COUNT];
	float4 Params[LIGHT_NO_SHADOW_COUNT];// brightness, quadratic, constant, linear
	float4 RangeConeType[LIGHT_NO_SHADOW_COUNT]; // 0 - point, 1 - spot, 2 - direct
	float4 Dir[LIGHT_NO_SHADOW_COUNT]; // .a - empty
};

struct d_light_wshadows
{
    float4 Pos[LIGHT_SHADOW_COUNT];// .a - lbr
	float4 Color[LIGHT_SHADOW_COUNT];
	float4 Params[LIGHT_SHADOW_COUNT];// quadratic, constant, linear, .a - bias
	float4 RangeConeType[LIGHT_SHADOW_COUNT];// .a - type:
	// 0, 1, 2, 3, 4, 5 - point
	// 10 - spot 
	// 20, 21, 22, 23, 24, 25, 26, 27 - direct
	float4 Dir[LIGHT_SHADOW_COUNT]; // .a - FilterSamples
	float4 Filter[LIGHT_SHADOW_COUNT]; // FilerType, SourceSize, 1/ShadowRes, NearClip
	matrix lightViewMatrix[LIGHT_SHADOW_COUNT];
    matrix lightProjectionMatrix[LIGHT_SHADOW_COUNT];
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