#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "EarlyVisibilitySystem.h"
#include "RenderMgrs.h"
#include "Util.h"
#include "LightBuffers.h"

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
		uint32_t mipsCount;
		Vector3 offset;
		float nearClip;
		float farClip;
		EnvProbQuality quality;
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

		static inline int32_t GetResolution(EnvProbQuality qual) 
		{
			return epResolutions[qual];
		}
		static inline int32_t GetMipsCount(EnvProbQuality qual) 
		{
			return GetLog2(epResolutions[qual]) - GetLog2(ENVPROBS_SPEC_MIN) + 1;
		}
		static inline DXGI_FORMAT GetFormat(EnvProbQuality qual) 
		{
			return epFormats[qual];
		}

	private:

		ComponentRDArray<EnvProbComponent> components;

		TransformSystem* transformSys;
		EarlyVisibilitySystem* earlyVisibilitySys;
		SArray<Frustum*, FRUSTUM_MAX_COUNT>* frustums;
		BaseWorld* world;

		static int32_t epResolutions[EP_QUAL_COUNT];
		static DXGI_FORMAT epFormats[EP_QUAL_COUNT];
	};
}