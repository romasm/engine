#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "TypeMgr.h"
#include "../Frustum.h"
#include "../RenderMgrs.h"

#define CAMERAS_INIT_COUNT 16
#define CAMERAS_MAX_COUNT 128

namespace EngineCore
{
	struct CameraComponent
	{
		ENTITY_IN_COMPONENT
			
		bool dirty;
		bool active;

		// static
		SceneRenderMgr* render_mgr;

		// update on props change
		BoundingFrustum localFrustum;
				
		float fov;
		float aspect_ratio;
		float near_clip;
		float far_clip;
		
		int32_t frust_id;
		int32_t volume_id;

		XMMATRIX projMatrix;

		// update on transform
		BoundingFrustum worldFrustum;

		XMVECTOR camPos;
		XMVECTOR camLookDir;
		XMVECTOR camUp;

		XMMATRIX viewMatrix;
		XMMATRIX prevViewProj;

		XMMATRIX view_proj;
		
		ALIGNED_ALLOCATION
	};

	class World;
	class GlobalLightSystem;

	class CameraSystem // TODO: share rndmgr between cameras (move rendermgr to scenepipeline???)
	{
		friend World;
		friend GlobalLightSystem;
	public:
		CameraSystem(World* wrd);
		void SetGlobalLightSys(GlobalLightSystem* gls);
		~CameraSystem();

		CameraComponent* AddComponent(Entity e)
		{
			if(components.dataSize() >= CAMERAS_MAX_COUNT)
				return nullptr;
			CameraComponent* res = components.add(e.index());
			res->far_clip = 10000.0f;
			res->near_clip = 0.1f;
			res->fov = 1.57f;
			res->aspect_ratio = 1.0f;
			res->active = false;

			res->parent = e;
			res->dirty = true;
			initCamera(res);
			return res;
		}
		void AddComponent(Entity e, CameraComponent D)
		{
			if(components.dataSize() >= CAMERAS_MAX_COUNT)
				return;
			D.parent = e;
			D.dirty = true;
			D.active = false;
			D.aspect_ratio = 1.0f;
			components.add(e.index(), D);
			initCamera(&components.getDataById(e.index()));
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp) return;
			AddComponent(dest, *comp);
		}
		void DeleteComponent(Entity e);
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline CameraComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == ENTITY_COUNT) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		
		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		void RegToDraw();
		void RegSingle(Entity e);

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		bool Activate(Entity e, ScenePipeline* scene);
		bool Deactivate(Entity e, ScenePipeline* scene);

		XMVECTOR GetVectorFromScreen(Entity e, XMVECTOR screen_point, float screen_w, float screen_h);

		XMFLOAT3 GetPos(Entity e)
		{
			XMFLOAT3 res;
			XMStoreFloat3(&res, GetPosV(e));
			return res;
		}
		XMFLOAT3 GetUp(Entity e)
		{
			XMFLOAT3 res;
			XMStoreFloat3(&res, GetUpV(e));
			return res;
		}
		XMFLOAT3 GetLookDir(Entity e)
		{
			XMFLOAT3 res;
			XMStoreFloat3(&res, GetLookDirV(e));
			return res;
		}
		XMFLOAT3 GetLookTangent(Entity e)
		{
			XMFLOAT3 res;
			XMStoreFloat3(&res, GetLookTangentV(e));
			return res;
		}

		XMVECTOR GetLookDirV(Entity e);
		XMVECTOR GetLookTangentV(Entity e);
		XMVECTOR GetUpV(Entity e);
		XMVECTOR GetPosV(Entity e);

		bool SetFov(Entity e, float fov);
		bool SetAspect(Entity e, float aspect);
		bool SetFar(Entity e, float farplane);
		bool SetNear(Entity e, float nearplane);

		bool SetProps(Entity e, CameraComponent D);

		float GetFov(Entity e);
		float GetAspect(Entity e);
		float GetFar(Entity e);
		float GetNear(Entity e);

		int32_t GetFrustumId(Entity e);
		int32_t GetVolumeId(Entity e);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		XMFLOAT3 _GetVectorFromScreen(Entity e, int x, int y, int screen_w, int screen_h)
		{
			XMFLOAT3 res;
			XMVECTOR t = GetVectorFromScreen(e, XMVectorSet(float(x), float(y), 0, 0), float(screen_w), float(screen_h));
			XMStoreFloat3(&res, t);
			return res;
		}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<CameraSystem>("CameraSystem")
					.addFunction("IsActive", &CameraSystem::IsActive)
					.addFunction("Activate", &CameraSystem::Activate)
					//.addFunction("Deactivate", &CameraSystem::Deactivate)
					.addFunction("GetVectorFromScreen", &CameraSystem::_GetVectorFromScreen)

					.addFunction("GetPos", &CameraSystem::GetPos)
					.addFunction("GetUp", &CameraSystem::GetUp)
					.addFunction("GetLookDir", &CameraSystem::GetLookDir)
					.addFunction("GetLookTangent", &CameraSystem::GetLookTangent)

					.addFunction("SetFov", &CameraSystem::SetFov)
					.addFunction("GetFov", &CameraSystem::GetFov)
					.addFunction("SetAspect", &CameraSystem::SetAspect)
					.addFunction("GetAspect", &CameraSystem::GetAspect)
					.addFunction("SetFar", &CameraSystem::SetFar)
					.addFunction("GetFar", &CameraSystem::GetFar)
					.addFunction("SetNear", &CameraSystem::SetNear)
					.addFunction("GetNear", &CameraSystem::GetNear)

					.addFunction("GetFrustumId", &CameraSystem::GetFrustumId)

					.addFunction("AddComponent", &CameraSystem::_AddComponent)
					.addFunction("DeleteComponent", &CameraSystem::DeleteComponent)
					.addFunction("HasComponent", &CameraSystem::HasComponent)
				.endClass();
		}

	private:
		void regCamera(CameraComponent& comp);
		void initCamera(CameraComponent* comp);

		ComponentSDArray<CameraComponent, ENTITY_COUNT> components;

		TransformSystem* transformSys;
		FrustumMgr* frustum_mgr;

		GlobalLightSystem* globalLightSystem;
	};

	struct CameraLink
	{
		CameraSystem* sys;
		Entity e;

		CameraLink() {sys = nullptr;}
		CameraLink(CameraSystem* system, Entity cam) {sys = system; e = cam;}
		inline CameraComponent* Get() const {return sys->GetComponent(e);}
	};
}