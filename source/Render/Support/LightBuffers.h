#pragma once

#include "Common.h"

#define LIGHT_PROB_FRAME_MAX 2048

#define LIGHT_SPOT_FRAME_MAX 1024
#define LIGHT_POINT_FRAME_MAX 1024

#define LIGHT_DIR_FRAME_MAX 8
#define LIGHT_DIR_NUM_CASCADES 4

#define CASTER_SPOT_FRAME_MAX 256
#define CASTER_POINT_FRAME_MAX 128

#define SHADOWS_BUF_RES 4096
#define SHADOWS_BUF_RES_RCP 1.0f / SHADOWS_BUF_RES
#define SHADOWS_BUF_MIPS 8 // min - 16

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
		Vector4 colorAreaX;
		Vector4 dirAreaY;
		Vector4 position[LIGHT_DIR_NUM_CASCADES];
		Vector4 shadowmapAdress[LIGHT_DIR_NUM_CASCADES];
		XMMATRIX matViewProj[LIGHT_DIR_NUM_CASCADES];

		DirLightBuffer(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, ShadowMap* shm[LIGHT_DIR_NUM_CASCADES])
		{
			colorAreaX = Vector4(color.x, color.y, color.z, area.x);
			dirAreaY = Vector4(dir.x, dir.y, dir.z, area.y);

			for (int32_t i = 0; i < LIGHT_DIR_NUM_CASCADES; i++)
			{
				position[i] = Vector4(pos[i].x, pos[i].y, pos[i].z, 0.5f / shm[i]->res);
				matViewProj[i] = view_proj[i];
				shadowmapAdress[i] = Vector4(shm[i]->x * SHADOWS_BUF_RES_RCP, shm[i]->y * SHADOWS_BUF_RES_RCP, shm[i]->res * SHADOWS_BUF_RES_RCP, (float)shm[i]->dsv);
			}
		}
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

#define UINFIED_TYPE_SPOT_LIGHT 0
#define UINFIED_TYPE_SPOT_CASTER 1
#define UINFIED_TYPE_POINT_LIGHT 2
#define UINFIED_TYPE_POINT_CASTER 3
#define UINFIED_TYPE_ENV_PROB_LQ 4
#define UINFIED_TYPE_ENV_PROB_SQ 5
#define UINFIED_TYPE_ENV_PROB_HQ 6

	struct UnifiedDataPackage
	{
		Vector4 vectors[21];

		inline void SetSpotLight(uint8_t sourceType, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos)
		{
			vectors[0] = Vector4((float)UINFIED_TYPE_SPOT_LIGHT, float(sourceType), 0, 0);
			vectors[1] = Vector4(pos.x, pos.y, pos.z, range);
			vectors[2] = Vector4(color.x, color.y, color.z, cone.x);
			vectors[3] = Vector4(dir.x, dir.y, dir.z, cone.y);
			vectors[4] = Vector4(up.x, up.y, up.z, area.x);
			vectors[5] = Vector4(side.x, side.y, side.z, area.y);
			vectors[6] = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);
		}

		inline void SetSpotCaster(uint8_t sourceType, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear,
			CXMMATRIX vp, CXMMATRIX proj, const ShadowMap& shm)
		{
			vectors[0] = Vector4((float)UINFIED_TYPE_SPOT_CASTER, float(sourceType), 0, 0);
			vectors[1] = Vector4(pos.x, pos.y, pos.z, range);
			vectors[2] = Vector4(color.x, color.y, color.z, cone.x);
			vectors[3] = Vector4(dir.x, dir.y, dir.z, cone.y);
			vectors[4] = Vector4(up.x, up.y, up.z, area.x);
			vectors[5] = Vector4(side.x, side.y, side.z, area.y);
			vectors[6] = Vector4(virtpos.x, virtpos.y, virtpos.z, area.z);
			vectors[7] = Vector4(shm.x * SHADOWS_BUF_RES_RCP, shm.y * SHADOWS_BUF_RES_RCP,
				shm.res * SHADOWS_BUF_RES_RCP, (float)shm.dsv);
			vectors[8] = Vector4(0.5f / shm.res, max<float>(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]), farNear.x, 0);
			vectors[9] = farNear;

			vectors[10] = vp.r[0];
			vectors[11] = vp.r[1];
			vectors[12] = vp.r[2];
			vectors[13] = vp.r[3];
		}

		inline void SetPointLight(uint8_t sourceType, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir)
		{
			vectors[0] = Vector4((float)UINFIED_TYPE_POINT_LIGHT, float(sourceType), 0, 0);
			vectors[1] = Vector4(pos.x, pos.y, pos.z, range);
			vectors[2] = Vector4(color.x, color.y, color.z, 0);
			vectors[3] = Vector4(area.x, area.y, area.z, area.y * area.y);
			vectors[4] = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
		}

		inline void SetPointCaster(uint8_t type, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir, 
			Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, const ShadowMap* shm[6])
		{
			vectors[0] = Vector4((float)UINFIED_TYPE_POINT_CASTER, pos.x, pos.y, pos.z);
			vectors[1] = Vector4(color.x, color.y, color.z, max(proj.r[0].m128_f32[0], proj.r[1].m128_f32[1]));
			vectors[2] = Vector4(area.x, area.y, area.z, area.y * area.y);
			vectors[3] = Vector4(dir.x, dir.y, dir.z, area.y + 2 * area.x);
			vectors[4] = Vector4(0.5f / shm[0]->res, 0.5f / shm[1]->res, 0.5f / shm[2]->res, 0.5f / shm[3]->res);
			vectors[5] = Vector4(0.5f / shm[4]->res, 0.5f / shm[5]->res, float(type), range);
			vectors[6] = Vector4(shm[0]->x * SHADOWS_BUF_RES_RCP, shm[0]->y * SHADOWS_BUF_RES_RCP, shm[0]->res * SHADOWS_BUF_RES_RCP, (float)shm[0]->dsv);
			vectors[7] = Vector4(shm[1]->x * SHADOWS_BUF_RES_RCP, shm[1]->y * SHADOWS_BUF_RES_RCP, shm[1]->res * SHADOWS_BUF_RES_RCP, (float)shm[1]->dsv);
			vectors[8] = Vector4(shm[2]->x * SHADOWS_BUF_RES_RCP, shm[2]->y * SHADOWS_BUF_RES_RCP, shm[2]->res * SHADOWS_BUF_RES_RCP, (float)shm[2]->dsv);
			vectors[9] = Vector4(shm[3]->x * SHADOWS_BUF_RES_RCP, shm[3]->y * SHADOWS_BUF_RES_RCP, shm[3]->res * SHADOWS_BUF_RES_RCP, (float)shm[3]->dsv);
			vectors[10] = Vector4(shm[4]->x * SHADOWS_BUF_RES_RCP, shm[4]->y * SHADOWS_BUF_RES_RCP, shm[4]->res * SHADOWS_BUF_RES_RCP, (float)shm[4]->dsv);
			vectors[11] = Vector4(shm[5]->x * SHADOWS_BUF_RES_RCP, shm[5]->y * SHADOWS_BUF_RES_RCP, shm[5]->res * SHADOWS_BUF_RES_RCP, (float)shm[5]->dsv);
			vectors[12] = farNear;

			vectors[13] = proj.r[0];
			vectors[14] = proj.r[1];
			vectors[15] = proj.r[2];
			vectors[16] = proj.r[3];

			vectors[17] = view.r[0];
			vectors[18] = view.r[1];
			vectors[19] = view.r[2];
			vectors[20] = view.r[3];
		}

		inline void SetEnvProb(int32_t type, const Vector3& pos, float dist, const Vector3& offset, float fade, uint32_t mips,
			EnvParallaxType parallax, int32_t adress, uint32_t p, const Vector3& sh, const XMMATRIX& invT)
		{
			vectors[0].x = (float)type;
			vectors[1] = Vector4(pos.x, pos.y, pos.z, dist);
			vectors[2] = Vector4(offset.x, offset.y, offset.z, fade);
			vectors[3] = Vector4(float(mips), float(type), float(adress), float(p));
			vectors[4] = Vector4(sh.x, sh.y, sh.z, 0);

			vectors[5] = invT.r[0];
			vectors[6] = invT.r[1];
			vectors[7] = invT.r[2];
			vectors[8] = invT.r[3];
		}
	};
}