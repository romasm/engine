#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "EarlyVisibilitySystem.h"
#include "CameraSystem.h"
#include "TypeMgr.h"

#define DIR_LIGHT_SCREENSIZE 999999.0f

#define CASCADE_DIST_0 7.0f
#define CASCADE_DIST_1 30.0f
#define CASCADE_DIST_2 200.0f
#define CASCADE_DIST_3 2000.0f

#define DEPTH_CASCADE_0 2500.0f
#define DEPTH_CASCADE_1 4000.0f
#define DEPTH_CASCADE_2 7500.0f
#define DEPTH_CASCADE_3 10000.0f

#define DEPTH_CASCADE_0_HALF DEPTH_CASCADE_0 * 0.5f
#define DEPTH_CASCADE_1_HALF DEPTH_CASCADE_1 * 0.5f
#define DEPTH_CASCADE_2_HALF DEPTH_CASCADE_2 * 0.5f
#define DEPTH_CASCADE_3_HALF DEPTH_CASCADE_3 * 0.5f

namespace EngineCore
{
	struct CascadeShadow
	{
		ShadowRenderMgr* render_mgr[LIGHT_DIR_NUM_CASCADES];

		// update on transform
		BoundingOrientedBox worldFrustum[LIGHT_DIR_NUM_CASCADES];
		XMMATRIX view_proj[LIGHT_DIR_NUM_CASCADES];
		XMMATRIX view[LIGHT_DIR_NUM_CASCADES];
		XMFLOAT3 pos[LIGHT_DIR_NUM_CASCADES];
		
		ID3D11Buffer* vp_buf[LIGHT_DIR_NUM_CASCADES];

		Entity camera;

		CascadeShadow& operator=(const CascadeShadow& right) 
		{
			for(uint8_t i = 0; i < LIGHT_DIR_NUM_CASCADES; i++)
			{
				render_mgr[i] = right.render_mgr[i];
				worldFrustum[i] = right.worldFrustum[i];
				view_proj[i] = right.view_proj[i];
				view[i] = right.view[i];
				pos[i] = right.pos[i];
				vp_buf[i] = right.vp_buf[i];
			}
			camera.set(right.camera.index(), right.camera.generation());
			return *this;
		}

		ALIGNED_ALLOCATION
	};

	struct CascadeProj
	{
		XMMATRIX proj[LIGHT_DIR_NUM_CASCADES];

		float posOffset[LIGHT_DIR_NUM_CASCADES];
		float size[LIGHT_DIR_NUM_CASCADES];
		
		Entity camera;

		ALIGNED_ALLOCATION
	};

	struct GlobalLightComponent
	{
		friend class GlobalLightSystem;

		ENTITY_IN_COMPONENT

	public:
		bool dirty;
		bool active;
		
		XMFLOAT3 color;
		float brightness;
		float area;
		
	private:
		XMFLOAT3 dir;
		XMFLOAT3 dir_up;

		XMFLOAT4 hdr_color;

		// dir disk: x - sin ang, y - cos ang
		XMFLOAT2 area_data; 

		DArray<CascadeShadow> cascadePerCamera;				

	public:
		GlobalLightComponent()
		{
			dirty = true;
			parent.setnull();
			area = 0;
			active = true;
			color = XMFLOAT3(1.0f,1.0f,1.0f);
			brightness = 1.0f;
		}
	};

	class BaseWorld;

	class GlobalLightSystem
	{
		friend BaseWorld;
	public:
		GlobalLightSystem(BaseWorld* w, uint32_t maxCount);
		~GlobalLightSystem()
		{
			for(auto& i: *components.data())
				for(auto& j: i.cascadePerCamera)
					destroyCascades(j);
		}

		GlobalLightComponent* AddComponent(Entity e)
		{
			GlobalLightComponent* res = components.add(e.index());
			res->dirty = true;
			res->parent = e;

			updateLightComp(*res);
			initShadows(*res);

			return res;
		}
		void AddComponent(Entity e, GlobalLightComponent D)
		{
			D.dirty = true;
			D.parent = e;
			components.add(e.index(), D);

			auto comp = GetComponent(e);
			updateLightComp(*comp);
			initShadows(*comp);
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp) return;

			GlobalLightComponent nComp;
			nComp.active = comp->active;
			nComp.area = comp->area;
			nComp.area_data = comp->area_data;
			nComp.brightness = comp->brightness;
			nComp.color = comp->color;
			nComp.dir = comp->dir;
			nComp.dir_up = comp->dir_up;
			nComp.hdr_color = comp->hdr_color;

			AddComponent(dest, nComp);
		}
		void DeleteComponent(Entity e)
		{
			auto comp = GetComponent(e);
			if(!comp) return;
			for(auto& i: comp->cascadePerCamera)
				destroyCascades(i);
			components.remove(e.index());
		}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		GlobalLightComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);
		
		void Update();
		void RegToScene();
		void RegShadowMaps();
		void RenderShadows();
		void ClearShadowsQueue();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		bool SetActive(Entity e, bool active);

		void AddCascadesForCamera(CameraComponent* camera);
		void SwitchCascadesForCamera(CameraComponent* camOld, CameraComponent* camNew);
		void DeleteCascadesForCamera(CameraComponent* camera);
		void UpdateCascadesForCamera(CameraComponent* camera);
		
		inline void _AddComponent(Entity e) {AddComponent(e);}

		bool SetColor(Entity e, XMFLOAT3 color);
		XMFLOAT3 GetColor(Entity e);
		
		bool SetBrightness(Entity e, float brightness);
		float GetBrightness(Entity e);

		bool SetArea(Entity e, float area);
		float GetArea(Entity e);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<GlobalLightSystem>("GlobalLightSystem")
					.addFunction("IsActive", &GlobalLightSystem::IsActive)
					.addFunction("SetActive", &GlobalLightSystem::SetActive)

					.addFunction("SetColor", &GlobalLightSystem::SetColor)
					.addFunction("GetColor", &GlobalLightSystem::GetColor)
					.addFunction("SetBrightness", &GlobalLightSystem::SetBrightness)
					.addFunction("GetBrightness", &GlobalLightSystem::GetBrightness)
					.addFunction("SetArea", &GlobalLightSystem::SetArea)
					.addFunction("GetArea", &GlobalLightSystem::GetArea)

					.addFunction("AddComponent", &GlobalLightSystem::_AddComponent)
					.addFunction("DeleteComponent", &GlobalLightSystem::DeleteComponent)
					.addFunction("HasComponent", &GlobalLightSystem::HasComponent)
				.endClass();
		}

	private:
		void initShadows(GlobalLightComponent& comp);
		void buildCascades(GlobalLightComponent& comp, uint16_t camId, CameraComponent* cam);
		void buildProj(uint16_t camId, CameraComponent* cam);
		void destroyCascades(CascadeShadow& cascade);
		inline void cleanCascades(CascadeShadow& cascade);

		inline void updateLightComp(GlobalLightComponent& comp);
		
		void matrixGenerate(GlobalLightComponent& comp, CascadeShadow& cascade, CascadeProj& projCascade);

		ComponentRDArray<GlobalLightComponent> components;

		RArray<uint16_t> cascadeNumForCamera;
		uint16_t camerasCount;

		SArray<CascadeProj, CAMERAS_MAX_COUNT> projPerCamera;

		TransformSystem* transformSys;
		CameraSystem* cameraSystem;
		FrustumMgr* frustumMgr;
		BaseWorld* world;

		float depth_cascade[LIGHT_DIR_NUM_CASCADES];
		float depth_cascade_half[LIGHT_DIR_NUM_CASCADES];
		float casc_dist[LIGHT_DIR_NUM_CASCADES];
	};
}