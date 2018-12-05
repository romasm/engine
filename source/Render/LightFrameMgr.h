#pragma once

#include "Common.h"
#include "Render.h"
#include "Material.h"
#include "RenderTarget.h"
#include "RenderState.h"
#include "LightBuffers.h"
#include "Compute.h"
#include "ShadowsRenderer.h"
#include "Entity.h"
#include "MeshLoader.h"
#include "ECS/EnvProbSystem.h"

#define ENVPROB_FRAME_MAX 32

namespace EngineCore
{
	class EnvProbMgr;

	class LightFrameMgr
	{
	public:
		LightFrameMgr();

		~LightFrameMgr()
		{ClearPerFrame();}

		void RegSpotLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector2& cone, Vector3& pos, Vector3& dir,
			Vector3& up, Vector3& side, Vector3& virtpos);
		void RegPointLight(uint8_t type, Vector4& color, float range, Vector3& area, Vector3& pos, Vector3& dir);

		void RegDirLight(Vector4& color, Vector2& area, Vector3& dir, XMMATRIX* view_proj, Vector3* pos, uint64_t id);

		// TODO: nonAreaColor depricated
		void RegSpotCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector2& cone, Vector3& pos,
			Vector3& dir, Vector3& up, Vector3& side, Vector3& virtpos, Vector4& farNear, CXMMATRIX vp, CXMMATRIX proj, uint64_t id);
		void RegPointCaster(uint8_t type, Vector4& color, Vector4& nonAreaColor, float range, Vector3& area, Vector3& pos, Vector3& dir,
			Vector4& farNear, CXMMATRIX proj, CXMMATRIX view, uint64_t id);

		void RegEnvProb(const EnvProbData& data);

		void ClearPerFrame();

	private:

		ShadowsRenderer* shadowsRenderer;
		EnvProbMgr* envProbMgr;

		RArray<UnifiedDataPackage> globalLightBuffer;
		RArray<DirLightBuffer> lightDirectionalBuffer;
	};
}