#pragma once

#include "ECS_defines.h"
#include "../Common.h"
#include "Entity.h"
#include "../RenderMgrs.h"
#include "EarlyVisibilitySystem.h"

namespace EngineCore
{

#define MAX_SHADOW_CASTERS ENTITY_COUNT / 6

	struct ShadowComponent // TODO
	{
		friend class ShadowSystem;

		ENTITY_IN_MULTICOMPONENT
		
		bool dirty;

	public:
		// static
		ShadowRenderMgr* render_mgr;

		// update on props change
		BoundingFrustum localFrustum;

		// update on transform
		BoundingFrustum worldFrustum;
		XMMATRIX view_proj;
		XMMATRIX proj;
		XMMATRIX view;

		ID3D11Buffer* vp_buf;

		ALIGNED_ALLOCATION
	};

	class World;
	class LightSystem;

	class ShadowSystem
	{
	public:
		ShadowSystem(World* wrd);
		~ShadowSystem()
		{
			for(auto& it: *components.data())
			{
				_CLOSE(it.render_mgr);
				_RELEASE(it.vp_buf);
			}
		}

		void SetLightSys(LightSystem* ls);

		ShadowComponent* AddComponent(Entity e)
		{
			ShadowComponent* res = nullptr;
			if(shadow_casters >= MAX_SHADOW_CASTERS)
				return res;

			res = components.add(e.index());
			res->parent = e;
			initShadowmap(res);

			shadow_casters++;
			return res;
		}
		void DeleteComponents(Entity e)
		{
			auto comp = GetComponent(e);
			if(!comp) return;
			do
			{
				_RELEASE(comp->vp_buf);
				_CLOSE(comp->render_mgr);
			}
			while(comp = GetNextComponent(comp));

			uint removed = components.remove(e.index());
			shadow_casters -= removed;
		}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		ShadowComponent* ShadowSystem::GetComponent(Entity e, uint num = 0)
		{return components.getDataById(e.index(), num);}
		ShadowComponent* ShadowSystem::GetNextComponent(ShadowComponent* comp)
		{
			if(comp->next == ENTITY_COUNT) return nullptr;
			return &components.getDataByArrayIdx(comp->next);
		}

		void Update();
		void RenderShadows();
		void ClearShadows();

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		void UpdateShadowmapData(Entity e);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<ShadowSystem>("ShadowSystem")
					.addFunction("AddComponent", &ShadowSystem::AddComponent)
					.addFunction("DeleteComponents", &ShadowSystem::DeleteComponents)
					.addFunction("HasComponent", &ShadowSystem::HasComponent)
				.endClass();
		}

	private:
		void initShadowmap(ShadowComponent* comp);
		
		size_t shadow_casters;

		MultiComponentSArray<ShadowComponent, ENTITY_COUNT, ENTITY_COUNT> components;

		EarlyVisibilitySystem* earlyVisibilitySys;
		LightSystem* lightSys;

		SArray<Frustum*, FRUSTUM_MAX_COUNT>* frustums;
	};
}