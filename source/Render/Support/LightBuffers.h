#pragma once

#include "Common.h"

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

#define SPOT_VOXEL_FRAME_MAX CASTER_SPOT_FRAME_MAX + CASTER_SPOT_DISK_FRAME_MAX + CASTER_SPOT_RECT_FRAME_MAX
#define POINT_VOXEL_FRAME_MAX CASTER_POINT_FRAME_MAX + CASTER_POINT_SPHERE_FRAME_MAX + CASTER_POINT_TUBE_FRAME_MAX

namespace EngineCore
{
	// for deffered
	struct SpotLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
	};

	struct DiskLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 AreaInfoEmpty;
		XMFLOAT4 VirtposEmpty;
	};

	struct RectLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 DirUpAreaX;
		XMFLOAT4 DirSideAreaY;
		XMFLOAT4 VirtposAreaZ;
	};

	struct SpotCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 ShadowmapAdress;
		XMFLOAT4 ShadowmapParams;
		XMMATRIX matViewProj;
	};

	struct DiskCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 AreaInfoEmpty;
		XMFLOAT4 VirtposEmpty;
		XMFLOAT4 ShadowmapAdress;
		XMFLOAT4 ShadowmapParams;
		XMMATRIX matViewProj;
	};

	struct RectCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 DirUpAreaX;
		XMFLOAT4 DirSideAreaY;
		XMFLOAT4 VirtposAreaZ;
		XMFLOAT4 ShadowmapAdress;
		XMFLOAT4 ShadowmapParams;
		XMMATRIX matViewProj;
	};

	struct PointLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 Color;
	};

	struct SphereLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorEmpty;
		XMFLOAT4 AreaInfoEmpty;
	};

	struct TubeLightBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorEmpty;
		XMFLOAT4 AreaInfo;
		XMFLOAT4 DirAreaA;
	};

	struct PointCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorShParams;
		XMFLOAT4 ShadowmapParams0;
		XMFLOAT4 ShadowmapParams1;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMFLOAT4 ShadowmapAdress4;
		XMFLOAT4 ShadowmapAdress5;
		XMMATRIX matProj;
	};

	struct SphereCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorShParams;
		XMFLOAT4 AreaInfoShParams;
		XMFLOAT4 ShadowmapParams;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMFLOAT4 ShadowmapAdress4;
		XMFLOAT4 ShadowmapAdress5;
		XMMATRIX matProj;
	};

	struct TubeCasterBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorShParams;
		XMFLOAT4 AreaInfo;
		XMFLOAT4 DirAreaA;
		XMFLOAT4 ShadowmapParams0;
		XMFLOAT4 ShadowmapParams1;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMFLOAT4 ShadowmapAdress4;
		XMFLOAT4 ShadowmapAdress5;
		XMMATRIX matProj;
		XMMATRIX matView;
	};

	struct DirLightBuffer
	{
		XMFLOAT4 ColorAreaX;
		XMFLOAT4 DirAreaY;
		XMFLOAT4 Pos0;
		XMFLOAT4 Pos1;
		XMFLOAT4 Pos2;
		XMFLOAT4 Pos3;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMMATRIX matViewProj0;
		XMMATRIX matViewProj1;
		XMMATRIX matViewProj2;
		XMMATRIX matViewProj3;
	};

	// voxels
	struct SpotVoxelBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorConeX;
		XMFLOAT4 DirConeY;
		XMFLOAT4 Virtpos;
		XMFLOAT4 ShadowmapAdress;
		XMFLOAT4 ShadowmapHPixProjNearclip;
		XMMATRIX matViewProj;
	};

	struct PointVoxelBuffer
	{
		XMFLOAT4 PosRange;
		XMFLOAT4 ColorShadowmapProj;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMFLOAT4 ShadowmapAdress4;
		XMFLOAT4 ShadowmapAdress5;
		XMFLOAT4 ShadowmapHPix0;
		XMFLOAT4 ShadowmapHPix1;
		XMMATRIX matProj;
		XMMATRIX matView;
	};

	struct DirVoxelBuffer
	{
		XMFLOAT4 Color;
		XMFLOAT4 Dir;
		XMFLOAT4 PosHPix0;
		XMFLOAT4 PosHPix1;
		XMFLOAT4 PosHPix2;
		XMFLOAT4 PosHPix3;
		XMFLOAT4 ShadowmapAdress0;
		XMFLOAT4 ShadowmapAdress1;
		XMFLOAT4 ShadowmapAdress2;
		XMFLOAT4 ShadowmapAdress3;
		XMMATRIX ViewProj0;
		XMMATRIX ViewProj1;
		XMMATRIX ViewProj2;
		XMMATRIX ViewProj3;
	};
}