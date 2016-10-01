#pragma once

#include "DataTypes.h"
#include "Render.h"
#include "ScenePipeline.h"
#include "LocalTimer.h"

#include "ECS/Entity.h"
#include "ECS/VisibilitySystem.h"
#include "ECS/EarlyVisibilitySystem.h"
#include "ECS/TransformSystem.h"
#include "ECS/CameraSystem.h"
#include "ECS/StaticMeshSystem.h"
#include "ECS/ControllerSystem.h"
#include "ECS/FreeCamController.h"
#include "ECS/EnvProbSystem.h"
#include "ECS/LightSystem.h"
#include "ECS/LineGeometrySystem.h"
#include "ECS/ShadowSystem.h"
#include "ECS/GlobalLightSystem.h"
#include "ECS/TypeMgr.h"
#include "ECS/ScriptSystem.h"
#include "Frustum.h"
#include "TransformControls.h"

#define DEFAULT_ENV L"default"
#define ENV_MESH PATH_SYS_MESHES "sky_shpere" EXT_STATIC

#define TRANSFORM_BYTE 't'
#define EARLYVIS_BYTE 'e'
#define VIS_BYTE 'v'
#define STATIC_BYTE 's'
#define LIGHT_BYTE 'l'
#define GLIGHT_BYTE 'g'
#define CAMERA_BYTE 'c'
#define SCRIPT_BYTE 'u'

namespace EngineCore
{
	class TypeMgr;

	class World
	{
		friend TypeMgr;
	public:
		World();

		bool Init(string filename);
		bool Init();

		void Snapshot(ScenePipeline* scene);
		void Frame();
		void Close();

		void SetDirty(Entity e);
		
		bool SaveWorld(string filename);
		
		string GetWorldName() const {return world_name;}
		UINT GetID() const {return ID;}
		void SetID(UINT l_ID) {ID = l_ID;}

		void AddScene(ScenePipeline* scene)
		{
			if(m_scenes.find(scene) == m_scenes.end())
				m_scenes.push_back(scene);
		}

		ScenePipeline* CreateScene(Entity cam, int w, int h)
		{
			ScenePipeline* scene = new ScenePipeline();
			if(!scene->Init(w, h))
			{_CLOSE(scene); return nullptr;}
			m_cameraSystem->Activate(cam, scene);
			m_scenes.push_back(scene);
			return scene;
		}

		bool DeleteScene(ScenePipeline* scene)
		{
			auto it = m_scenes.find(scene);
			if(it != m_scenes.end())
			{
				_CLOSE(scene);
				m_scenes.erase(it);
				return true;
			}
			return false;
		}

		inline void PostStMeshesReload()
		{
			if(m_staticMeshSystem)
				m_staticMeshSystem->PostReload();
		}

		Entity GetMainCamera() const {return mainCamera;}
		
		bool IsActive() const {return b_active;}
		void SetActive(bool active) {b_active = active;}

		inline float GetDT() const {return m_dt;} 

		inline FrustumMgr* GetFrustumMgr() const {return m_frustumMgr;}
		inline EntityMgr* GetEntityMgr() const {return m_entityMgr;}
		inline TransformSystem* GetTransformSystem() const {return m_transformSystem;}
		inline VisibilitySystem* GetVisibilitySystem() const {return m_visibilitySystem;}
		inline EarlyVisibilitySystem* GetEarlyVisibilitySystem() const {return m_earlyVisibilitySystem;}
		inline StaticMeshSystem* GetStaticMeshSystem() const {return m_staticMeshSystem;}
		inline CameraSystem* GetCameraSystem() const {return m_cameraSystem;}
		inline EnvProbSystem* GetEnvProbSystem() const {return m_envProbSystem;}
		inline LightSystem* GetLightSystem() const {return m_lightSystem;}
		inline LineGeometrySystem* GetLineGeometrySystem() const {return m_lineGeometrySystem;}
		inline ControllerSystem* GetControllerSystem() const {return m_controllerSystem;}
		inline ShadowSystem* GetShadowSystem() const {return m_shadowSystem;}
		inline GlobalLightSystem* GetGlobalLightSystem() const {return m_globalLightSystem;}
		inline ScriptSystem* GetScriptSystem() const {return m_scriptSystem;}

		inline TransformControls* GetTransformControls() const {return m_transformControls;}

		inline TypeMgr* GetTypeMgr() const {return m_typeMgr;}
		inline NameMgr* GetNameMgr() const {return m_nameMgr;}

		inline Entity CreateEntity() {return m_entityMgr->CreateEntity();}
		Entity CreateNamedEntity(string name) 
		{
			Entity res = m_entityMgr->CreateEntity();
			m_nameMgr->SetName(res, name);
			return res;
		}
		string GetEntityName(Entity e)
		{ 
			if(e.isnull())
				return "";
			return m_nameMgr->GetName(e);
		}
		Entity GetEntityByName(string name) { return m_nameMgr->GetEntityByName(name); }

		inline Entity RestoreEntity() {return m_entityMgr->RestoreEntity();}

		inline bool IsEntityAlive(Entity e) {return m_entityMgr->IsAlive(e);}
		void DestroyEntity(Entity e);
		Entity CopyEntity(Entity e);

		inline void RenameEntity(Entity e, string name) {m_nameMgr->SetName(e, name);}

		inline bool SetEntityType(Entity e, string type) {return m_typeMgr->SetType(e, type);}
		inline string GetEntityType(Entity e) {return m_typeMgr->GetType(e);}
		inline Entity GetFirstEntityByType(string type) {return m_typeMgr->GetFirstByType(type);}
		inline Entity GetNextEntityByType() {return m_typeMgr->GetNextByType();}

		inline LuaRef WrapEntityForLua(Entity e) // must be used somewhere
		{
			LuaRef res = m_scriptSystem->GetLuaClassInstance(e);
			if(res.isNil())
			{
				auto type = m_typeMgr->GetType(e);
				if(!type.empty())
					return m_typeMgr->LuaConstructor(type, e);
			}
			return res;
		}

		void RebakeSky()
		{
			m_envProbSystem->Bake(skyEP);
		}

	#ifdef _DEV
		void UpdateLuaFuncs()
		{
			m_typeMgr->UpdateLuaFuncs();
			m_scriptSystem->UpdateLuaFuncs();
		}
	#endif

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<World>("World")
					.addFunction("Save", &World::SaveWorld)
					.addFunction("GetID", &World::GetID)
					.addFunction("GetWorldName", &World::GetWorldName)

					.addFunction("AddScene", &World::AddScene)
					.addFunction("CreateScene", &World::CreateScene)
					.addFunction("DeleteScene", &World::DeleteScene)

					.addFunction("GetMainCamera", &World::GetMainCamera)

					.addFunction("CreateEntity", &World::CreateEntity)
					.addFunction("CreateNamedEntity", &World::CreateNamedEntity)
					.addFunction("GetEntityName", &World::GetEntityName)
					.addFunction("IsEntityAlive", &World::IsEntityAlive)
					.addFunction("DestroyEntity", &World::DestroyEntity)
					.addFunction("RestoreEntity", &World::RestoreEntity)
					.addFunction("CopyEntity", &World::CopyEntity)
					.addFunction("RenameEntity", &World::RenameEntity)
					.addFunction("GetEntityByName", &World::GetEntityByName)

					.addFunction("SetEntityType", &World::SetEntityType)
					.addFunction("GetEntityType", &World::GetEntityType)
					.addFunction("GetFirstEntityByType", &World::GetFirstEntityByType)
					.addFunction("GetNextEntityByType", &World::GetNextEntityByType)

					.addProperty("transform", &World::GetTransformSystem)
					.addProperty("visibility", &World::GetVisibilitySystem)
					.addProperty("earlyVisibility", &World::GetEarlyVisibilitySystem)
					.addProperty("staticMesh", &World::GetStaticMeshSystem)
					.addProperty("camera", &World::GetCameraSystem)
					.addProperty("light", &World::GetLightSystem)
					.addProperty("globalLight", &World::GetGlobalLightSystem)
					.addProperty("lineGeometry", &World::GetLineGeometrySystem)
					.addProperty("controller", &World::GetControllerSystem)
					.addProperty("script", &World::GetScriptSystem)

					.addProperty("transformControls", &World::GetTransformControls)

					.addFunction("RebakeSky", &World::RebakeSky)

					.addProperty("active", &World::IsActive, &World::SetActive)
				.endClass();
		}

		ALIGNED_ALLOCATION

	private:
		struct WorldHeader
		{
			wchar_t env_name[256];
			XMVECTOR env_rot;
			
			XMVECTOR free_cam_pos;
			XMVECTOR free_cam_rot;
		};

		bool loadWorld(string filename, WorldHeader& header);

		void initMainEntities(WorldHeader header);

		bool b_active;

		LocalTimer m_world_timer;
		float m_dt;

		string world_name;
		UINT ID;

		Entity mainCamera;
		Entity skyEP;

		wstring envName;

		DArray<ScenePipeline*> m_scenes;

		FrustumMgr* m_frustumMgr;

		EntityMgr* m_entityMgr;
		TransformSystem* m_transformSystem;
		VisibilitySystem* m_visibilitySystem;
		EarlyVisibilitySystem* m_earlyVisibilitySystem;
		StaticMeshSystem* m_staticMeshSystem;
		CameraSystem* m_cameraSystem;
		ControllerSystem* m_controllerSystem;
		EnvProbSystem* m_envProbSystem;
		LightSystem* m_lightSystem;
		LineGeometrySystem* m_lineGeometrySystem;
		ShadowSystem* m_shadowSystem;
		GlobalLightSystem* m_globalLightSystem;
		ScriptSystem* m_scriptSystem;

		TransformControls* m_transformControls;

		TypeMgr* m_typeMgr;
		NameMgr* m_nameMgr;
	};
}