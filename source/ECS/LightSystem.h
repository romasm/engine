#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "EarlyVisibilitySystem.h"
#include "ShadowSystem.h"
#include "TypeMgr.h"

#define LIGHT_TYPE_POINT	0
#define LIGHT_TYPE_SPHERE	1
#define LIGHT_TYPE_TUBE		2
#define LIGHT_TYPE_SPOT		3
#define LIGHT_TYPE_DISK		4
#define LIGHT_TYPE_RECT		5

#define SHADOW_NEARCLIP		0.01f
#define SHADOW_POINT_FOVADD		0.0349f // 2 градуса

namespace EngineCore
{
	struct LightComponent // TODO
	{
		friend class LightSystem;

		ENTITY_IN_COMPONENT

	public:
		bool dirty;
		bool active;

		uint8_t type;

		XMFLOAT3 color;
		float brightness;
		float nonAreaBrightness;
		float range;
		XMFLOAT2 area; 
		XMFLOAT2 cone; // x - in, y - out
		
		bool cast_shadows;
		bool transparent_shadows;

	private:
		XMFLOAT3 pos;
		XMFLOAT3 dir;
		XMFLOAT3 dir_up;
		XMFLOAT3 dir_side;

		XMFLOAT4 hdr_color;
		XMFLOAT4 nonAreaColor;
		float rangeInvSqr;
		XMFLOAT2 cone_data;

		// spot disk: x - radius
		// spot rect: x, y, z - length, width/2, length/2
		// point sphere: x - radius
		// point tube: x - radius, y - length
		XMFLOAT3 area_data; 

		XMFLOAT3 virt_pos;
		float virt_clip;
				
	};

	class BaseWorld;

	class LightSystem
	{
		friend BaseWorld;
	public:
		LightSystem(BaseWorld* w, uint32_t maxCount);

		LightComponent* AddComponent(Entity e)
		{
			LightComponent* res = components.add(e.index());
			res->dirty = true;
			res->parent = e;
			res->cast_shadows = false;
			res->transparent_shadows = false;
			res->area = XMFLOAT2(1.0f, 1.0f);
			res->cone = XMFLOAT2(XM_PIDIV2, 0.1f);
			res->range = 1.0f;
			res->type = 0;
			res->active = true;
			res->color = XMFLOAT3(0,0,0);
			res->brightness = 1.0f;
			res->nonAreaBrightness = 1.0f;

			UpdateLightProps(e);
			initShadows(e, res);

			return res;
		}
		void AddComponent(Entity e, LightComponent D)
		{
			D.dirty = true;
			D.parent = e;
			components.add(e.index(), D);

			UpdateLightProps(e);
			initShadows(e, GetComponent(e));
		}
		void CopyComponent(Entity src, Entity dest)
		{
			auto comp = GetComponent(src);
			if(!comp) return;
			AddComponent(dest, *comp);
		}
		void DeleteComponent(Entity e)
		{
			components.remove(e.index());
		}
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		LightComponent* GetComponent(Entity e)
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

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);

		bool IsActive(Entity e);
		bool SetActive(Entity e, bool active);

		bool SetType(Entity e, uint8_t type);
		uint8_t GetType(Entity e);

		bool SetColor(Entity e, XMFLOAT3 color);
		XMFLOAT3 GetColor(Entity e);
		
		bool SetBrightness(Entity e, float brightness);
		float GetBrightness(Entity e);

		bool SetRange(Entity e, float range);
		float GetRange(Entity e);
		
		bool SetConeIn(Entity e, float conein);
		float GetConeIn(Entity e);
		
		bool SetConeOut(Entity e, float coneout);
		float GetConeOut(Entity e);
		
		bool SetAreaX(Entity e, float areax);
		float GetAreaX(Entity e);
		
		bool SetAreaY(Entity e, float areay);
		float GetAreaY(Entity e);
		
		bool SetCastShadows(Entity e, bool cast);
		bool GetCastShadows(Entity e);
		
		bool SetTransparentShadows(Entity e, bool cast);
		bool GetTransparentShadows(Entity e);

		void UpdateLightProps(Entity e); // TODO
		void UpdateShadows(Entity e);

		XMMATRIX GetProj(Entity e);
		XMMATRIX GetView(Entity e, uchar num, XMVECTOR* pos);

		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<LightSystem>("LightSystem")
					.addFunction("IsActive", &LightSystem::IsActive)
					.addFunction("SetActive", &LightSystem::SetActive)

					.addFunction("SetType", &LightSystem::SetType)
					.addFunction("GetType", &LightSystem::GetType)
					.addFunction("SetColor", &LightSystem::SetColor)
					.addFunction("GetColor", &LightSystem::GetColor)
					.addFunction("SetBrightness", &LightSystem::SetBrightness)
					.addFunction("GetBrightness", &LightSystem::GetBrightness)
					.addFunction("SetRange", &LightSystem::SetRange)
					.addFunction("GetRange", &LightSystem::GetRange)
					.addFunction("SetConeIn", &LightSystem::SetConeIn)
					.addFunction("GetConeIn", &LightSystem::GetConeIn)
					.addFunction("SetConeOut", &LightSystem::SetConeOut)
					.addFunction("GetConeOut", &LightSystem::GetConeOut)
					.addFunction("SetAreaX", &LightSystem::SetAreaX)
					.addFunction("GetAreaX", &LightSystem::GetAreaX)
					.addFunction("SetAreaY", &LightSystem::SetAreaY)
					.addFunction("GetAreaY", &LightSystem::GetAreaY)
					.addFunction("SetCastShadows", &LightSystem::SetCastShadows)
					.addFunction("GetCastShadows", &LightSystem::GetCastShadows)
					.addFunction("SetTransparentShadows", &LightSystem::SetTransparentShadows)
					.addFunction("GetTransparentShadows", &LightSystem::GetTransparentShadows)

					.addFunction("AddComponent", &LightSystem::_AddComponent)
					.addFunction("DeleteComponent", &LightSystem::DeleteComponent)
					.addFunction("HasComponent", &LightSystem::HasComponent)

					.addFunction("UpdateLightProps", &LightSystem::UpdateLightProps)
					.addFunction("UpdateShadows", &LightSystem::UpdateShadows)
				.endClass();
		}

	private:
		void calcNonAreaBrightness(LightComponent& comp);

		inline void updateSpot(LightComponent& comp);
		inline void updatePoint(LightComponent& comp);

		void initShadows(Entity e, LightComponent* comp);

		ComponentRArray<LightComponent> components;

		TransformSystem* transformSys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		ShadowSystem* shadowSystem;
		FrustumMgr* frustumMgr;
		BaseWorld* world;
	};
}