#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "EarlyVisibilitySystem.h"
#include "RenderMgrs.h"
#include "Util.h"

#define ENVPROBS_INIT_COUNT 128

#define ENVPROBS_MAT PATH_SHADERS "offline/envmap_render"
#define ENVPROBS_MIPS_MAT PATH_SHADERS "offline/envmap_mipgen"

#define ENVPROBS_SPEC_MIN 2
#define ENVPROBS_SPEC_MIPS_OFFSET 0
#define ENVPROBS_RES 128

#define ENVPROBS_SUBFOLDER "/probes/"
#define ENVPROBS_NAME_LENGTH 10

#define ENVPROBS_PRIORITY_ALWAYS 0

#define TEX_HAMMERSLEY PATH_SYS_TEXTURES "hammersley" EXT_TEXTURE

namespace EngineCore
{
	enum EnvParallaxType
	{
		EP_PARALLAX_SPHERE = 0,
		EP_PARALLAX_BOX = 1,
		EP_PARALLAX_NONE = 2
	};

	enum EnvProbPriority
	{
		EP_PRIORITY_ALWAYS = 0,
		EP_PRIORITY_1 = 1,
		EP_PRIORITY_2 = 2,
		EP_PRIORITY_3 = 3,
		EP_PRIORITY_4 = 4,
		EP_PRIORITY_5 = 5,
		EP_PRIORITY_6 = 6,
	};

	struct EnvProbComponent
	{
		ENTITY_IN_COMPONENT
			
		bool dirty;

		// static
		string probName;
		uint32_t probId;

		// update on props change
		EnvParallaxType type;
		float fade;		
		uint32_t mipsCount; // mipsNum - 1
		Vector3 offset;
		float nearClip;
		float farClip;
		int32_t resolution;
		bool isHQ;
		uint32_t priority;

		// update on dirty
		float cachedDistance;
		XMMATRIX cachedInvTransform;
		Vector3 cachedPos;
		
		ALIGNED_ALLOCATION
	};

	class BaseWorld;

	class EnvProbSystem
	{
	public:
		EnvProbSystem(BaseWorld* wrd, uint32_t maxCount);
		~EnvProbSystem()
		{
			for(auto& i: *components.data())
			{
				TEXTURE_DROP(i.probId);
			}
		}
		
		void AddComponent(Entity e);
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline EnvProbComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		void DeleteComponent(Entity e);
		
		void RegToScene();
		
		bool IsDirty(Entity e);
		bool SetDirty(Entity e);
		
		bool Bake(Entity e);

		string GetProbFileName(string& probName) const;

		void UpdateProps(Entity e);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<EnvProbSystem>("EnvProbSystem")
					.addFunction("AddComponent", &EnvProbSystem::AddComponent)
					.addFunction("DeleteComponent", &EnvProbSystem::DeleteComponent)
					.addFunction("HasComponent", &EnvProbSystem::HasComponent)

					.addFunction("UpdateProps", &EnvProbSystem::UpdateProps)

					.addFunction("Bake", &EnvProbSystem::Bake)
				.endClass();
		}

	private:

		ComponentRDArray<EnvProbComponent> components;

		TransformSystem* transformSys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		SArray<Frustum*, FRUSTUM_MAX_COUNT>* frustums;
		BaseWorld* world;
	};
}