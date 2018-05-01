#pragma once

#include "ECS_defines.h"
#include "Entity.h"
#include "TransformSystem.h"
#include "RenderMgrs.h"
#include "Util.h"
#include "LightBuffers.h"

#define ENVPROBS_INIT_COUNT 128

#define ENVPROBS_MAT PATH_SHADERS "offline/envmap_render"
#define ENVPROBS_MIPS_MAT PATH_SHADERS "offline/envmap_mipgen"
#define ENVPROBS_COPY_MAT PATH_SHADERS "offline/envmap_mipcopy"

#define ENVPROBS_SPEC_MIPS_OFFSET 0
#define ENVPROBS_RES 128

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
		Vector3 offset;
		float nearClip;
		float farClip;
		EnvProbQuality quality;
		uint32_t priority;

		// update on dirty
		float cachedDistance;
		Vector3 cachedShape;
		XMMATRIX cachedInvTransform;
		Vector3 cachedPos;

		bool needRebake;
		
		ALIGNED_ALLOCATION
	};

	class BaseWorld;
	class EarlyVisibilitySystem;
	struct Frustum;

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
		
		EnvProbComponent* AddComponent(Entity e);
		bool HasComponent(Entity e) const {return components.has(e.index());}
		size_t ComponentsCount() {return components.dataSize();}

		inline EnvProbComponent* GetComponent(Entity e)
		{
			size_t idx = components.getArrayIdx(e.index());
			if(idx == components.capacity()) return nullptr;
			return &components.getDataByArrayIdx(idx);
		}
		void DeleteComponent(Entity e);
		
		void CopyComponent(Entity src, Entity dest);

		void RegToScene();

		uint32_t Serialize(Entity e, uint8_t* data);
		uint32_t Deserialize(Entity e, uint8_t* data);

		bool IsDirty(Entity e);
		bool SetDirty(Entity e);
		
		uint32_t GetType(Entity e);
		bool SetType(Entity e, uint32_t type);
		float GetFade(Entity e);
		bool SetFade(Entity e, float fade);
		Vector3 GetOffset(Entity e);
		bool SetOffset(Entity e, Vector3& offset);
		float GetNearClip(Entity e);
		bool SetNearClip(Entity e, float clip);
		float GetFarClip(Entity e);
		bool SetFarClip(Entity e, float clip);
		uint32_t GetPriority(Entity e);
		bool SetPriority(Entity e, uint32_t priority);
		uint32_t GetQuality(Entity e);
		bool SetQuality(Entity e, uint32_t quality);

		bool Bake(Entity e);

		string GetProbFileName(string& probName) const;

		void UpdateProps(Entity e);
		
		inline void _AddComponent(Entity e) {AddComponent(e);}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<EnvProbSystem>("EnvProbSystem")
					.addFunction("AddComponent", &EnvProbSystem::_AddComponent)
					.addFunction("DeleteComponent", &EnvProbSystem::DeleteComponent)
					.addFunction("HasComponent", &EnvProbSystem::HasComponent)

					.addFunction("GetType", &EnvProbSystem::GetType)
					.addFunction("SetType", &EnvProbSystem::SetType)
					.addFunction("GetFade", &EnvProbSystem::GetFade)
					.addFunction("SetFade", &EnvProbSystem::SetFade)
					.addFunction("GetOffset", &EnvProbSystem::GetOffset)
					.addFunction("SetOffset", &EnvProbSystem::SetOffset)
					.addFunction("GetNearClip", &EnvProbSystem::GetNearClip)
					.addFunction("SetNearClip", &EnvProbSystem::SetNearClip)
					.addFunction("GetFarClip", &EnvProbSystem::GetFarClip)
					.addFunction("SetFarClip", &EnvProbSystem::SetFarClip)
					.addFunction("GetPriority", &EnvProbSystem::GetPriority)
					.addFunction("SetPriority", &EnvProbSystem::SetPriority)
					.addFunction("GetQuality", &EnvProbSystem::GetQuality)
					.addFunction("SetQuality", &EnvProbSystem::SetQuality)
					
					.addFunction("Bake", &EnvProbSystem::Bake)
				.endClass();
		}

		static inline int32_t GetResolution(EnvProbQuality qual) 
		{
			return epResolutions[qual];
		}
		static inline uint32_t GetMipsCount(EnvProbQuality qual) 
		{
			return epMipsCount[qual];
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
		static uint32_t epMipsCount[EP_QUAL_COUNT];
	};
}