#include "stdafx.h"
#include "World.h"
#include "Common.h"
#include "ECS/TypeMgr.h"
#include "Utils/Profiler.h"

using namespace EngineCore;

World::World()
{
	b_active = false;
	
	world_name = "";
	ID = 0;

	m_frustumMgr = new FrustumMgr;
	m_entityMgr = new EntityMgr;
	m_typeMgr = new TypeMgr(this);
	m_nameMgr = new NameMgr;

	m_transformSystem = new TransformSystem(this);
	m_visibilitySystem = new VisibilitySystem(this);
	m_earlyVisibilitySystem = new EarlyVisibilitySystem(this);
	m_scriptSystem = new ScriptSystem(this);
	
	m_staticMeshSystem = new StaticMeshSystem(this);
	m_cameraSystem = new CameraSystem(this);
	m_controllerSystem = new ControllerSystem(this);
	m_envProbSystem = new EnvProbSystem(this);
	m_shadowSystem = new ShadowSystem(this);
	m_lightSystem = new LightSystem(this);
	m_shadowSystem->SetLightSys(m_lightSystem);

	m_globalLightSystem = new GlobalLightSystem(this);
	m_cameraSystem->SetGlobalLightSys(m_globalLightSystem);

	m_lineGeometrySystem = new LineGeometrySystem(this);

	m_transformControls = new TransformControls(this);

	m_scenes.reserve(16);

	m_dt = 0;
}

void World::SetDirty(Entity e)
{
	m_transformSystem->SetDirty(e);
	m_visibilitySystem->SetDirty(e);
	m_earlyVisibilitySystem->SetDirty(e);
	m_staticMeshSystem->SetDirty(e);
	m_cameraSystem->SetDirty(e);
	m_envProbSystem->SetDirty(e);
	m_lightSystem->SetDirty(e);
	m_shadowSystem->SetDirty(e);
	m_globalLightSystem->SetDirty(e);
	m_lineGeometrySystem->SetDirty(e);
}

bool World::Init(string filename)
{
	if(world_name.size() > 0)
		return false;

	WorldHeader header;

	if( !loadWorld(filename, header) )
	{
		ERR("Не удалось загрузить мир %ls", filename.data());
		return false;
	}

	initMainEntities(header);

	world_name = filename;

	m_world_timer.Start();
	
	return true;
}

bool World::Init()
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

void World::Close()
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

	_DELETE(m_transformControls);

	_DELETE(m_typeMgr);
	_DELETE(m_nameMgr);
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

void World::DestroyEntity(Entity e)
{
	// immidiate
	m_transformSystem->DeleteComponent(e);
	m_visibilitySystem->DeleteComponent(e);
	m_earlyVisibilitySystem->DeleteComponent(e);
	m_scriptSystem->DeleteComponent(e);
	m_staticMeshSystem->DeleteComponent(e);
	m_cameraSystem->DeleteComponent(e);
	m_controllerSystem->DeleteComponent(e);
	m_envProbSystem->DeleteComponent(e);
	m_lightSystem->DeleteComponent(e);
	m_shadowSystem->DeleteComponents(e);
	m_globalLightSystem->DeleteComponent(e);
	m_lineGeometrySystem->DeleteComponent(e);

	m_typeMgr->ClearType(e);
	m_nameMgr->ClearName(e);

	return m_entityMgr->Destroy(e);
}

Entity World::CopyEntity(Entity e)
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
	m_earlyVisibilitySystem->CopyComponent(e, newEnt);
	m_scriptSystem->CopyComponent(e, newEnt);

	m_staticMeshSystem->CopyComponent(e, newEnt);
	m_cameraSystem->CopyComponent(e, newEnt);
	//m_envProbSystem->CopyComponent(e, newEnt);
	m_lightSystem->CopyComponent(e, newEnt);
	m_globalLightSystem->CopyComponent(e, newEnt);

	m_typeMgr->SetType(newEnt, m_typeMgr->GetType(e));

	return newEnt;
}

#define SAVE_BUFFER_SIZE 16384

bool World::loadWorld(string filename, WorldHeader& header)
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

void World::initMainEntities(WorldHeader header)
{
	envName = header.env_name;

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

	skyEP = m_entityMgr->CreateEntity();
	m_transformSystem->AddComponent(skyEP);
	m_transformSystem->SetPosition(skyEP, 0, 0, 0);
	m_transformSystem->SetRotation(skyEP, header.env_rot);
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

bool World::SaveWorld(string filename)
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
	XMMATRIX cam_mat = m_transformSystem->GetTransformW(mainCamera);
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
		if( iterator == mainCamera || iterator == skyEP )
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