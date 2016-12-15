#include "stdafx.h"
#include "World.h"
#include "Common.h"
#include "ECS/TypeMgr.h"
#include "Utils/Profiler.h"

using namespace EngineCore;

BaseWorld::BaseWorld()
{
	b_active = false;
	
	world_name = "";
	ID = 0;

	m_scenes.reserve(16);

	m_dt = 0;
}

void BaseWorld::SetDirty(Entity e)
{
	m_transformSystem->SetDirty(e);
	m_visibilitySystem->SetDirty(e);

	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->SetDirty(e);

	m_staticMeshSystem->SetDirty(e);
	m_cameraSystem->SetDirty(e);
	m_envProbSystem->SetDirty(e);

	if(m_lightSystem)
		m_lightSystem->SetDirty(e);
	if(m_shadowSystem)
		m_shadowSystem->SetDirty(e);
	if(m_globalLightSystem)
		m_globalLightSystem->SetDirty(e);
	if(m_lineGeometrySystem)
		m_lineGeometrySystem->SetDirty(e);
}

bool BaseWorld::Init(string filename)
{
	if(world_name.size() > 0)
		return false;

	WorldHeader header;

	if( !loadWorld(filename, header) )
	{
		ERR("Can\'t load world %ls", filename.data());
		return false;
	}

	initMainEntities(header);

	world_name = filename;

	m_world_timer.Start();
	
	return true;
}

bool BaseWorld::Init()
{
	if(world_name.size() > 0)
		return false;

	world_name = "new_scene.mls";

	WorldHeader header;
	wcscpy_s(header.env_name, DEFAULT_ENV);
	header.free_cam_pos = XMVectorZero();
	header.free_cam_rot = XMVectorZero();
	header.env_rot = XMVectorZero();

	initMainEntities(header);

	m_world_timer.Start();
	
	return true;
}

void BaseWorld::Close()
{
	world_name = "";

	for(auto& it: m_scenes)
		if(it)
		{
			ScenePipeline* scene = it;
			_CLOSE(scene);
		}
	m_scenes.clear();

	_DELETE(m_frustumMgr);
	_DELETE(m_entityMgr);
	_DELETE(m_transformSystem);
	_DELETE(m_visibilitySystem);
	_DELETE(m_earlyVisibilitySystem);
	_DELETE(m_scriptSystem);

	_DELETE(m_staticMeshSystem);
	_DELETE(m_cameraSystem);
	_DELETE(m_controllerSystem);
	_DELETE(m_envProbSystem);
	_DELETE(m_lightSystem);
	_DELETE(m_shadowSystem);
	_DELETE(m_globalLightSystem);
	_DELETE(m_lineGeometrySystem);

	_DELETE(m_typeMgr);
	_DELETE(m_nameMgr);
}

void BaseWorld::DestroyEntity(Entity e)
{
	// immidiate
	m_transformSystem->DeleteComponent(e);
	m_visibilitySystem->DeleteComponent(e);
	
	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->DeleteComponent(e);

	m_scriptSystem->DeleteComponent(e);
	m_staticMeshSystem->DeleteComponent(e);
	m_cameraSystem->DeleteComponent(e);
	m_controllerSystem->DeleteComponent(e);
	m_envProbSystem->DeleteComponent(e);

	if(m_lightSystem)
		m_lightSystem->DeleteComponent(e);
	if(m_shadowSystem)
		m_shadowSystem->DeleteComponents(e);
	if(m_globalLightSystem)
		m_globalLightSystem->DeleteComponent(e);
	if(m_lineGeometrySystem)
		m_lineGeometrySystem->DeleteComponent(e);

	m_typeMgr->ClearType(e);
	m_nameMgr->ClearName(e);

	return m_entityMgr->Destroy(e);
}

Entity BaseWorld::CopyEntity(Entity e)
{
	Entity newEnt;
	newEnt.setnull();

	if(!m_entityMgr->IsAlive(e))
		return newEnt;

	newEnt = CreateEntity();
	if(newEnt.isnull())
		return newEnt;

	m_transformSystem->CopyComponent(e, newEnt);
	m_visibilitySystem->CopyComponent(e, newEnt);

	if(m_earlyVisibilitySystem)
		m_earlyVisibilitySystem->CopyComponent(e, newEnt);

	m_scriptSystem->CopyComponent(e, newEnt);

	m_staticMeshSystem->CopyComponent(e, newEnt);
	m_cameraSystem->CopyComponent(e, newEnt);
	//m_envProbSystem->CopyComponent(e, newEnt);
	
	if(m_lightSystem)
		m_lightSystem->CopyComponent(e, newEnt);
	if(m_globalLightSystem)
		m_globalLightSystem->CopyComponent(e, newEnt);

	m_typeMgr->SetType(newEnt, m_typeMgr->GetType(e));

	return newEnt;
}

#define SAVE_BUFFER_SIZE 16384

bool BaseWorld::loadWorld(string& filename, WorldHeader& header)
{
	uint32_t data_size = 0;
	unique_ptr<uint8_t> data(FileIO::ReadFileData(filename, &data_size));
	uint8_t* t_data = data.get();

	if(!t_data)
	{
		ERR("Cant read world file %s !", filename.c_str());
		return false;
	}

	if( data_size < sizeof(WorldHeader) + sizeof(uint32_t) )
	{
		ERR("World file %s corrupted!", filename.c_str());
		return false;
	}

	// header
	memcpy_s( &header, sizeof(WorldHeader), t_data, sizeof(WorldHeader) );
	t_data += sizeof(WorldHeader);

	m_transformSystem->PreLoad();

	// entities
	uint32_t entCount = *(uint32_t*)t_data;
	if(entCount == 0)
		return true;
	t_data += sizeof(uint32_t);

	while( entCount > 0 && (t_data - data.get()) < data_size )
	{
		uint32_t type_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string type((char*)t_data, type_size);
		t_data += type_size * sizeof(char);

		uint32_t name_size = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string name((char*)t_data, name_size);
		t_data += name_size * sizeof(char);

		Entity ent = CreateNamedEntity(name);
		if(!SetEntityType(ent, type))
			WRN("Cant set type %s to entity %s", type.c_str(), name.c_str());

		uint32_t comp_count = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);
		string components((char*)t_data, comp_count);
		t_data += comp_count * sizeof(char);

		for(uint16_t i = 0; i < comp_count; i++)
		{
			uint32_t compSize = 0;
			switch (components[i])
			{
			case TRANSFORM_BYTE: compSize = m_transformSystem->Deserialize(ent, t_data);
				break;
			case EARLYVIS_BYTE: compSize = m_earlyVisibilitySystem->Deserialize(ent, t_data);
				break;
			case VIS_BYTE: compSize = m_visibilitySystem->Deserialize(ent, t_data);
				break;
			case STATIC_BYTE: compSize = m_staticMeshSystem->Deserialize(ent, t_data);
				break;
			case LIGHT_BYTE: compSize = m_lightSystem->Deserialize(ent, t_data);
				break;
			case GLIGHT_BYTE: compSize = m_globalLightSystem->Deserialize(ent, t_data);
				break;
			case CAMERA_BYTE: compSize = m_cameraSystem->Deserialize(ent, t_data);
				break;
			case SCRIPT_BYTE: compSize = m_scriptSystem->Deserialize(ent, t_data);
				break;
			}
			t_data += compSize;
		}

		entCount--;
	}

	m_transformSystem->PostLoadParentsResolve();

	return true;
}

void BaseWorld::initMainEntities(WorldHeader header)
{
	envName = header.env_name;

	skyEP = m_entityMgr->CreateEntity();
	m_transformSystem->AddComponent(skyEP);
	m_transformSystem->SetPosition(skyEP, 0, 0, 0);
	m_transformSystem->SetRotation(skyEP, header.env_rot);

	const float far_clip = EngineSettings::EngSets.cam_far_clip;
	const float sky_scale = far_clip * 0.95f;
	m_transformSystem->SetScale(skyEP, sky_scale, sky_scale, sky_scale);

	m_staticMeshSystem->AddComponent(skyEP);
	m_staticMeshSystem->SetMesh(skyEP, ENV_MESH);
	string sky_mat = PATH_ENVS;
	sky_mat += WstringToString(wstring(header.env_name));
	sky_mat += EXT_MATERIAL;
	m_staticMeshSystem->SetMaterial(skyEP, 0, sky_mat);

	EnvProbComponent ep_comp;
	ep_comp.eptex_name = PATH_ENVS;
	ep_comp.eptex_name += WstringToString(wstring(header.env_name));
	ep_comp.specCube = TEX_NULL;
	ep_comp.diffCube = TEX_NULL;
	ep_comp.is_distant = true;
	m_envProbSystem->AddComponent(skyEP, ep_comp);
	m_envProbSystem->SetActive(skyEP, true);
}

bool BaseWorld::saveWorld(string& filename, Entity editorCamera)
{
	ofstream file;
	file.open(filename, std::ios::trunc | std::ios::binary );
	if(!file.is_open())
		return false;

	world_name = filename;

	// header
	WorldHeader header;
	wcscpy_s(header.env_name, envName.data());
	header.env_rot = m_transformSystem->GetVectRotationW(skyEP);
	XMMATRIX cam_mat = m_transformSystem->GetTransformW(editorCamera);
	XMVECTOR temp;
	XMMatrixDecompose(&temp, &header.free_cam_rot, &header.free_cam_pos, cam_mat);

	file.write( (char*)&header, sizeof(WorldHeader) );

	// entities
	uint32_t entCount = m_entityMgr->GetEntityCount();
	if(entCount < 2)
		ERR("Wrong entity count in world!");
	else
		entCount -= 2;

	file.write( (char*)&entCount, sizeof(uint32_t) );

	uint8_t* buffer = new uint8_t[SAVE_BUFFER_SIZE];

	Entity iterator;
	iterator.setnull();
	while( !(iterator = m_entityMgr->GetNextEntity(iterator)).isnull() )
	{
		if( iterator == editorCamera || iterator == skyEP )
			continue;

		string type = m_typeMgr->GetType(iterator);
		uint32_t type_size = (uint32_t)type.size();
		file.write( (char*)&type_size, sizeof(uint32_t) );
		if(type_size > 0)
			file.write( (char*)type.data(), type_size * sizeof(char) );

		string name = m_nameMgr->GetName(iterator);
		uint32_t name_size = (uint32_t)name.size();
		file.write( (char*)&name_size, sizeof(uint32_t) );
		if(name_size > 0)
			file.write( (char*)name.data(), name_size * sizeof(char) );

		////////////////
		bool editor_mesh = m_staticMeshSystem->IsEditorOnly(iterator);		// TODO !!!!!!!!!!!!
		editor_mesh = false;
		////////////////

		string components = "";
		if(m_transformSystem->HasComponent(iterator))
			components += TRANSFORM_BYTE;
		if(m_earlyVisibilitySystem->HasComponent(iterator))
			components += EARLYVIS_BYTE;
		if(!editor_mesh && m_visibilitySystem->HasComponent(iterator))
			components += VIS_BYTE;
		if(!editor_mesh && m_staticMeshSystem->HasComponent(iterator))
			components += STATIC_BYTE;
		if(m_lightSystem->HasComponent(iterator))
			components += LIGHT_BYTE;
		if(m_globalLightSystem->HasComponent(iterator))
			components += GLIGHT_BYTE;
		if(m_cameraSystem->HasComponent(iterator))
			components += CAMERA_BYTE;
		if(m_scriptSystem->HasComponent(iterator))
			components += SCRIPT_BYTE;

		uint32_t comp_count = (uint32_t)components.size();
		file.write( (char*)&comp_count, sizeof(uint32_t) );
		if(comp_count == 0)
			continue;
		file.write( (char*)components.data(), comp_count * sizeof(char) );

		for(uint16_t i = 0; i < comp_count; i++)
		{
			uint32_t compSize = 0;
			switch (components[i])
			{
			case TRANSFORM_BYTE: compSize = m_transformSystem->Serialize(iterator, buffer);
				break;
			case EARLYVIS_BYTE: compSize = m_earlyVisibilitySystem->Serialize(iterator, buffer);
				break;
			case VIS_BYTE: compSize = m_visibilitySystem->Serialize(iterator, buffer);
				break;
			case STATIC_BYTE: compSize = m_staticMeshSystem->Serialize(iterator, buffer);
				break;
			case LIGHT_BYTE: compSize = m_lightSystem->Serialize(iterator, buffer);
				break;
			case GLIGHT_BYTE: compSize = m_globalLightSystem->Serialize(iterator, buffer);
				break;
			case CAMERA_BYTE: compSize = m_cameraSystem->Serialize(iterator, buffer);
				break;
			case SCRIPT_BYTE: compSize = m_scriptSystem->Serialize(iterator, buffer);
				break;
			}

			if(compSize == 0)
				continue;

			file.write( (char*)buffer, compSize );
		}
	}
	_DELETE_ARRAY(buffer);
	
	file.close();
	return true;
}

// World ---------------------
World::World() : BaseWorld()
{
	m_frustumMgr = new FrustumMgr;

	m_entityMgr = new EntityMgr(ENTITY_COUNT);
	m_typeMgr = new TypeMgr(this, ENTITY_COUNT);
	m_nameMgr = new NameMgr(ENTITY_COUNT);

	m_transformSystem = new TransformSystem(this, ENTITY_COUNT);
	m_visibilitySystem = new VisibilitySystem(this, ENTITY_COUNT);
	m_earlyVisibilitySystem = new EarlyVisibilitySystem(this, ENTITY_COUNT);
	m_scriptSystem = new ScriptSystem(this, ENTITY_COUNT);
	
	m_staticMeshSystem = new StaticMeshSystem(this, ENTITY_COUNT);
	m_cameraSystem = new CameraSystem(this, ENTITY_COUNT);
	m_controllerSystem = new ControllerSystem(this, ENTITY_COUNT);
	m_envProbSystem = new EnvProbSystem(this, ENTITY_COUNT);
	m_shadowSystem = new ShadowSystem(this, ENTITY_COUNT);
	m_lightSystem = new LightSystem(this, ENTITY_COUNT);
	m_shadowSystem->SetLightSys(m_lightSystem);

	m_globalLightSystem = new GlobalLightSystem(this, ENTITY_COUNT);
	m_cameraSystem->SetGlobalLightSys(m_globalLightSystem);

	m_lineGeometrySystem = new LineGeometrySystem(this, ENTITY_COUNT);

	m_transformControls = new TransformControls(this);
}

void World::Snapshot(ScenePipeline* scene)
{
#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	m_dt = 0;
	
	m_transformSystem->Update();
	m_lightSystem->Update();
	m_shadowSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegSingle(scene->GetSceneCamera().e);

	m_globalLightSystem->Update();
	
	m_earlyVisibilitySystem->CheckEarlyVisibility();

	m_lightSystem->RegShadowMaps();
	m_globalLightSystem->RegShadowMaps();

	scene->ResolveShadowmaps();

	m_envProbSystem->RegToScene(); // replace system with one-envmap solution
	m_lightSystem->RegToScene();
	m_globalLightSystem->RegToScene();

	m_visibilitySystem->CheckVisibility();

	m_staticMeshSystem->RegToDraw();
	
	m_shadowSystem->RenderShadows();
	m_globalLightSystem->RenderShadows();

	m_shadowSystem->ClearShadowsQueue();
	m_globalLightSystem->ClearShadowsQueue();

	if(scene->StartFrame(&m_world_timer))
	{
		scene->OpaqueForwardStage();
		scene->OpaqueDefferedStage();
		scene->TransparentForwardStage();
		scene->HDRtoLDRStage();
		scene->EndFrame();
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void World::Frame()
{
	if(!b_active)
		return;

	PERF_CPU_BEGIN(_SCENE_UPDATE);

	m_world_timer.Frame();
	m_dt = m_world_timer.dt();
	
	// start update

	m_controllerSystem->Process();

	m_scriptSystem->Update(m_dt);

	m_transformSystem->Update();
	m_lightSystem->Update();
	m_shadowSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegToDraw();

	m_globalLightSystem->Update();
	
	m_earlyVisibilitySystem->CheckEarlyVisibility();

	m_lightSystem->RegShadowMaps();
	m_globalLightSystem->RegShadowMaps();

	for(auto& it: m_scenes)
		it->ResolveShadowmaps();

	m_envProbSystem->RegToScene(); // replace system with one-envmap solution
	m_lightSystem->RegToScene();
	m_globalLightSystem->RegToScene();

	m_visibilitySystem->CheckVisibility();

	PERF_CPU_END(_SCENE_UPDATE);

	PERF_CPU_BEGIN(_SCENE_DRAW);

	m_staticMeshSystem->RegToDraw();
	m_lineGeometrySystem->RegToDraw();
	
	PERF_GPU_TIMESTAMP(_SCENE_SHADOWS);
	m_shadowSystem->RenderShadows();
	m_globalLightSystem->RenderShadows();

	m_shadowSystem->ClearShadowsQueue();
	m_globalLightSystem->ClearShadowsQueue();

	m_transformControls->RegToDraw();

	for(auto& it: m_scenes)
	{
		if(it->StartFrame(&m_world_timer))
		{
			PERF_GPU_TIMESTAMP(_SCENE_FORWARD);
			it->OpaqueForwardStage();
			it->HudStage();

			PERF_GPU_TIMESTAMP(_SCENE_DEFFERED);
			it->OpaqueDefferedStage();
			
			PERF_GPU_TIMESTAMP(_SCENE_APLHA);
			it->TransparentForwardStage();
			
			PERF_GPU_TIMESTAMP(_SCENE_LDR);
			it->HDRtoLDRStage();

			it->EndFrame();
		}
	}

	PERF_CPU_END(_SCENE_DRAW);
}

void World::Close()
{
	_DELETE(m_transformControls);

	BaseWorld::Close();
}

void World::initMainEntities(WorldHeader header)
{
	mainCamera = m_entityMgr->CreateEntity();
	m_transformSystem->AddComponent(mainCamera);
	m_transformSystem->SetPosition(mainCamera, header.free_cam_pos);
	m_transformSystem->SetRotation(mainCamera, header.free_cam_rot);

	const float far_clip = EngineSettings::EngSets.cam_far_clip;
	const float near_clip = EngineSettings::EngSets.cam_near_clip;
	const float fov = EngineSettings::EngSets.cam_fov;

	m_cameraSystem->AddComponent(mainCamera);
	CameraComponent D;
	D.aspect_ratio = 1.0f;
	D.far_clip = far_clip;
	D.near_clip = near_clip;
	D.fov = fov;
	D.active = false;
	m_cameraSystem->SetProps(mainCamera, D);

	FreeCamController* free_ctrl = new FreeCamController();
	free_ctrl->SetPlayerStand(0,0,0);
	free_ctrl->rot_speed = EngineSettings::EngSets.cam_rot_speed;
	free_ctrl->move_speed = EngineSettings::EngSets.cam_move_speed;

	m_controllerSystem->AddComponent(mainCamera, free_ctrl);
	m_controllerSystem->SetActive(mainCamera, true);

	BaseWorld::initMainEntities(header);
}

// World ---------------------
SmallWorld::SmallWorld() : BaseWorld()
{
	m_earlyVisibilitySystem = nullptr;
	m_shadowSystem = nullptr;
	m_lightSystem = nullptr;
	m_globalLightSystem = nullptr;
	m_lineGeometrySystem = nullptr;

	m_frustumMgr = new FrustumMgr;

	m_entityMgr = new EntityMgr(SMALL_ENTITY_COUNT);
	m_typeMgr = new TypeMgr(this, SMALL_ENTITY_COUNT);
	m_nameMgr = new NameMgr(SMALL_ENTITY_COUNT);

	m_transformSystem = new TransformSystem(this, SMALL_ENTITY_COUNT);
	m_visibilitySystem = new VisibilitySystem(this, SMALL_ENTITY_COUNT);
	m_scriptSystem = new ScriptSystem(this, SMALL_ENTITY_COUNT);
	
	m_staticMeshSystem = new StaticMeshSystem(this, SMALL_ENTITY_COUNT);
	m_cameraSystem = new CameraSystem(this, SMALL_ENTITY_COUNT);
	m_controllerSystem = new ControllerSystem(this, SMALL_ENTITY_COUNT);
	m_envProbSystem = new EnvProbSystem(this, SMALL_ENTITY_COUNT);
}

void SmallWorld::Snapshot(ScenePipeline* scene)
{
#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	m_dt = 0;
	
	m_transformSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegSingle(scene->GetSceneCamera().e);
	
	m_envProbSystem->RegToScene(); // replace system with one-envmap solution

	m_visibilitySystem->CheckVisibility();

	m_staticMeshSystem->RegToDraw();
	
	if(scene->StartFrame(&m_world_timer))
	{
		scene->OpaqueForwardStage();
		scene->OpaqueDefferedStage();
		scene->TransparentForwardStage();
		scene->HDRtoLDRStage();
		scene->EndFrame();
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void SmallWorld::Frame()
{
#ifdef _DEV
	bool profiler_state = Profiler::Get()->IsRunning();
	if(profiler_state)
		Profiler::Get()->Stop();
#endif

	if(!b_active)
		return;
	
	m_world_timer.Frame();
	m_dt = m_world_timer.dt();
	
	// start update

	m_controllerSystem->Process();

	m_scriptSystem->Update(m_dt);

	m_transformSystem->Update();

	m_frustumMgr->Clear();
	m_cameraSystem->RegToDraw();
	
	m_envProbSystem->RegToScene(); // replace system with one-envmap solution

	m_visibilitySystem->CheckVisibility();
	
	m_staticMeshSystem->RegToDraw();

	for(auto& it: m_scenes)
	{
		if(it->StartFrame(&m_world_timer))
		{
			it->OpaqueForwardStage();
			it->HudStage();

			it->OpaqueDefferedStage();
			
			it->TransparentForwardStage();
			
			it->HDRtoLDRStage();

			it->EndFrame();
		}
	}

#ifdef _DEV
	if(profiler_state)
		Profiler::Get()->Start();
#endif
}

void SmallWorld::Close()
{
	BaseWorld::Close();
}