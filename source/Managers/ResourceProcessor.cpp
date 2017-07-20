#include "stdafx.h"
#include "ResourceProcessor.h"

#include "WorldMgr.h"
#include "FontMgr.h"
#include "MaterialMgr.h"
#include "ShaderCodeMgr.h"
#include "ShaderMgr.h"
#include "MeshMgr.h"
#include "TexMgr.h"
#include "TexLoader.h"
#include "MeshLoader.h"

using namespace EngineCore;

ResourceProcessor *ResourceProcessor::instance = nullptr;

ResourceProcessor::ResourceProcessor()
{
	if(!instance)
	{
		instance = this;

		loadingQueue = new RQueueLockfree<ResourceSlot>(LOADING_QUEUE_SIZE);
		postLoadingQueue = new RQueueLockfree<ResourceSlot>(LOADING_QUEUE_SIZE);

		loaderRunning = true;
		loader = new thread(&ResourceProcessor::ThreadMain, &(*this));
		
		shaderMgr = new ShaderMgr;
		shaderCodeMgr = new ShaderCodeMgr;
		fontMgr = new FontMgr;
		worldMgr = new WorldMgr;
		texMgr = new TexMgr;
		materialMgr = new MaterialMgr;
		meshMgr = new MeshMgr;
	}
	else
		ERR("Only one instance of ResourceProcessor is allowed!");
}

ResourceProcessor::~ResourceProcessor()
{
#ifdef _DEV
	JOBSYSTEM->deletePeriodicalJob(SHADER_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(SHADERCODE_JOB_NAME);
#endif
	JOBSYSTEM->deletePeriodicalJob(TEXTURE_JOB_NAME);
	JOBSYSTEM->deletePeriodicalJob(STMESH_JOB_NAME);

	loaderRunning = false;
	v_loadingRequest.notify_all();

	if(loader->joinable())
		loader->join();
	delete loader;

	_DELETE(worldMgr);
	_DELETE(fontMgr);
	_DELETE(meshMgr);		
	_DELETE(materialMgr);
	_DELETE(shaderMgr);
	_DELETE(texMgr);
	_DELETE(shaderCodeMgr);

	_DELETE(loadingQueue);
	_DELETE(postLoadingQueue);
	
	instance = nullptr;
}

void ResourceProcessor::Tick()
{
#ifdef _EDITOR
	if( meshMgr->IsBBoxesDirty() )
		worldMgr->PostMeshesReload();
#endif

	ResourceSlot loadedSlot;
	while(loadingQueue->pop(loadedSlot))
	{
		switch(loadedSlot.type)
		{
		case ResourceType::TEXTURE:
			texMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		case ResourceType::MESH:
			meshMgr->OnPostLoadMainThread(loadedSlot.id, loadedSlot.callback, loadedSlot.status);
			break;

		default:
			continue;
		}
	}
}

void ResourceProcessor::ThreadMain()
{
	DBG_SHORT("Start loading tread %u ", JobSystem::GetThreadID());

	while(loaderRunning)
	{
		// wait loading request
		{
			unique_lock<mutex> l(m_loading);
			v_loadingRequest.wait(l);
			l.unlock();
		}

		ResourceSlot loadingSlot;
		while(loadingQueue->pop(loadingSlot))
		{
			loadingSlot.status = LoadingStatus::FAILED;

			switch(loadingSlot.type)
			{
			case ResourceType::TEXTURE:
				{
					string& fileName = texMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = TexLoader::LoadFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							texMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			case ResourceType::MESH:
				{
					string& fileName = meshMgr->GetName(loadingSlot.id);
					uint32_t size = 0;
					uint8_t* data = FileIO::ReadFileData(fileName, &size);
					if(data)
					{
						auto loadedData = MeshLoader::LoadStaticMeshFromMemory(fileName, data, size);
						_DELETE_ARRAY(data);

						if(loadedData)
						{
							meshMgr->OnLoad(loadingSlot.id, loadedData);
							loadingSlot.status = LOADED;
						}
					}
				}
				break;

			case ResourceType::SHADER:
				ERR("TODO: Move shader & shader code loading\\compiling in ResourceProcessor");
				break;
				
			default:
				continue;
			}

			if(!postLoadingQueue->push(loadingSlot))
				WRN("Resource post loading queue overflow!");
		}
	}

	DBG_SHORT("End loading tread %u ", JobSystem::GetThreadID());
}

bool ResourceProcessor::QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback)
{
	if(!loadingQueue->push(ResourceSlot(id, type, callback)))
	{
		WRN("Resource loading queue overflow!");
		return false;
	}
	v_loadingRequest.notify_one();
	return true;
}

void ResourceProcessor::AddUpdateJobs()
{
#ifdef _EDITOR

	#ifdef _DEV
		JOBSYSTEM->addPeriodicalJob(SHADER_JOB_NAME, JOB_F_MEMBER(ShaderMgr, ShaderMgr::Get(), CheckForReload), 
			SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
		JOBSYSTEM->addPeriodicalJob(SHADERCODE_JOB_NAME, JOB_F_MEMBER(ShaderCodeMgr, ShaderCodeMgr::Get(), CheckForReload), 
			SHADERS_UPDATE_PERIOD, JobPriority::BACKGROUND);
	#endif

	JOBSYSTEM->addPeriodicalJob(TEXTURE_JOB_NAME, JOB_F_MEMBER(TexMgr, TexMgr::Get(), CheckForReload), 
		TEXTURES_UPDATE_PERIOD, JobPriority::BACKGROUND);
	JOBSYSTEM->addPeriodicalJob(STMESH_JOB_NAME, JOB_F_MEMBER(MeshMgr, MeshMgr::Get(), CheckForReload), 
		STMESHES_UPDATE_PERIOD, JobPriority::BACKGROUND);

#endif
}

// TODO: move preloading managment to Lua
void ResourceProcessor::Preload()
{
	shaderCodeMgr->PreloadPureCodes();
	shaderMgr->PreloadShaders();

	texMgr->PreloadTextures();

	meshMgr->PreloadStMeshes();
		
	fontMgr->PreloadFonts();
	
	// force update
	texMgr->UpdateTextures();
}

void TexMgr::PreloadTextures()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetTexture(string(TEX_NOISE2D), reload);
	GetTexture(string(TEX_PBSENVLUT), reload);
	GetTexture(string(TEX_SMAA_AREA), reload);
	GetTexture(string(TEX_SMAA_SEARCH), reload);
	GetTexture(string(TEX_HBAO_DITHER), reload);

	GetTexture(string(TEX_HAMMERSLEY), reload);

	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/arrow_down"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/arrow_right"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/maximize"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/minimize"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/move"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/pipet"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/reset"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/restore"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/rotate"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/scale"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/select"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/win_close"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/assign_asset"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/clear_str"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/delete"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/copy_mat"EXT_TEXTURE), reload);
	GetTexture(string(PATH_SYS_TEXTURES"editor_hud/new_mat"EXT_TEXTURE), reload);
}

#define MAT_MESH PATH_SYS_MESHES "mat_sphere" EXT_STATIC

void MeshMgr::PreloadStMeshes()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetStMesh(string(ENV_MESH), reload);
	GetStMesh(string(MAT_MESH), reload);
}

void ShaderMgr::PreloadShaders()
{
	GetShader(string(SP_MATERIAL_DEPTH_OPAC_DIR), true);
	GetShader(string(SP_MATERIAL_HBAO), true);
	GetShader(string(SP_MATERIAL_HBAO_PERPECTIVE_CORRECT), true);
	GetShader(string(SP_MATERIAL_AO), true);
	GetShader(string(SP_MATERIAL_HDR), true);
	GetShader(string(SP_MATERIAL_COMBINE), true);
	GetShader(string(SP_MATERIAL_HIZ_DEPTH), true);
	GetShader(string(SP_MATERIAL_OPAQUE_BLUR), true);
	GetShader(string(SP_MATERIAL_AVGLUM), true);
	GetShader(string(SP_MATERIAL_BLOOM_FIND), true);
	GetShader(string(SP_MATERIAL_AA_EDGE), true);
	GetShader(string(SP_MATERIAL_AA_BLEND), true);
	GetShader(string(SP_MATERIAL_AA), true);
	GetShader(string(SP_SHADER_SSR), true);

	GetShader(string(SP_SHADER_SCREENSHOT), true);

	GetShader(string(LG_SHADER), false);
	GetShader(string(LG_SHADER_SPHERE), false);

	GetShader(string(COMMON_MATERIAL_SHADER_01), false);
	GetShader(string(COMMON_MATERIAL_SHADER_02), false);
	GetShader(string(COMMON_MATERIAL_SHADER_03), false);

	GetShader(string(ENVPROBS_MAT), true);
	GetShader(string(ENVPROBS_MIPS_MAT), true);
	GetShader(string(ENVPROBS_DIFF_MAT), true);

	GetShader(string(PATH_SHADERS"gui/color"), true);
	GetShader(string(PATH_SHADERS"gui/font_default"), true);
	GetShader(string(PATH_SHADERS"gui/group_arrow"), true);
	GetShader(string(PATH_SHADERS"gui/h_picker"), true);
	GetShader(string(PATH_SHADERS"gui/rect"), true);
	GetShader(string(PATH_SHADERS"gui/rect_icon"), true);
	GetShader(string(PATH_SHADERS"gui/rect_color_icon_alpha"), true);
	GetShader(string(PATH_SHADERS"gui/rect_icon_bg"), true);
	GetShader(string(PATH_SHADERS"gui/shadow"), true);
	GetShader(string(PATH_SHADERS"gui/sv_picker"), true);
	GetShader(string(PATH_SHADERS"gui/t_picker"), true);
	GetShader(string(PATH_SHADERS"gui/viewport"), true);
	GetShader(string(PATH_SHADERS"gui/viewport_2darr"), true);
	GetShader(string(PATH_SHADERS"gui/viewport_cube"), true);

#ifdef _DEV
	GetShader(string(DFG_mat), true);
	GetShader(string(NOISE2D_mat), true);
#endif
}

void ShaderCodeMgr::PreloadPureCodes()
{
	Compute::Preload( COMPUTE_VOXEL_INJECT_LIGHT );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "1" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "2" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "4" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_EMITTANCE "8" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "1" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "2" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "4" );
	Compute::Preload( COMPUTE_VOXEL_DOWNSAMPLE_MOVE "8" );
	Compute::Preload( SHADER_DEFFERED_OPAQUE_IBL );
	Compute::Preload( SHADER_DEFFERED_OPAQUE_FULL );	
}

void FontMgr::PreloadFonts()
{
	GetFont(string(PATH_FONTS "opensans_light_18px"));
	GetFont(string(PATH_FONTS "opensans_light_20px"));
	GetFont(string(PATH_FONTS "opensans_light_25px"));
	GetFont(string(PATH_FONTS "opensans_light_40px"));
	GetFont(string(PATH_FONTS "opensans_light_70px"));
	GetFont(string(PATH_FONTS "opensans_normal_12px"));
	GetFont(string(PATH_FONTS "opensans_normal_14px"));
	GetFont(string(PATH_FONTS "opensans_normal_16px"));
	GetFont(string(PATH_FONTS "opensans_normal_18px"));
	GetFont(string(PATH_FONTS "opensans_normal_20px"));
	GetFont(string(PATH_FONTS "opensans_normal_25px"));
	GetFont(string(PATH_FONTS "opensans_normal_40px"));
	GetFont(string(PATH_FONTS "opensans_normal_70px"));
}