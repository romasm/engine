#include "stdafx.h"
#include "ShaderMgr.h"
#include "Common.h"

#include "ScenePipeline.h"
#include "RenderMgrs.h"
#include "TransformControls.h"
#include "ECS\LineGeometrySystem.h"
#include "ECS\EnvProbSystem.h"
#include "Generate_PBS_Env_LUT.h"

using namespace EngineCore;

ShaderMgr* ShaderMgr::instance = nullptr;
string ShaderMgr::null_name = "";

ShaderMgr::ShaderMgr()
{
	if(!instance)
	{
		instance = this;

		shader_array.resize(SHADER_MAX_COUNT);
		shader_free.resize(SHADER_MAX_COUNT);
		for(uint32_t i=0; i<SHADER_MAX_COUNT; i++)
			shader_free[i] = i;
		shader_map.reserve(SHADER_INIT_COUNT);
	}
	else
		ERR("Only one instance of ShaderMgr is allowed!");
}

ShaderMgr::~ShaderMgr()
{
	for(uint32_t i=0; i<SHADER_MAX_COUNT; i++)
	{
		_DELETE(shader_array[i].shader);
		shader_array[i].name.erase();
	}

	instance = nullptr;
}

void ShaderMgr::PreloadShaders()
{
	GetShader(string(SP_MATERIAL_DEPTH_OPAC_DIR), true);
	GetShader(string(SP_MATERIAL_DEFFERED_OPAC_DIR), true);
	GetShader(string(SP_MATERIAL_HBAO), true);
	GetShader(string(SP_MATERIAL_HBAO_PERPECTIVE_CORRECT), true);
	GetShader(string(SP_MATERIAL_AO), true);
	GetShader(string(SP_MATERIAL_HDR), true);
	GetShader(string(SP_MATERIAL_COMBINE), true);
	GetShader(string(SP_MATERIAL_HIZ_DEPTH_CC), true);
	GetShader(string(SP_MATERIAL_HIZ_DEPTH_UC), true);
	GetShader(string(SP_MATERIAL_HIZ_DEPTH_CU), true);
	GetShader(string(SP_MATERIAL_HIZ_DEPTH_UU), true);
	GetShader(string(SP_MATERIAL_OPAQUE_BLUR), true);
	GetShader(string(SP_MATERIAL_AVGLUM), true);
	GetShader(string(SP_MATERIAL_BLOOM_FIND), true);
	GetShader(string(SP_MATERIAL_AA_EDGE), true);
	GetShader(string(SP_MATERIAL_AA_BLEND), true);
	GetShader(string(SP_MATERIAL_AA), true);
	GetShader(string(SP_SHADER_SSR), true);
	GetShader(string(SP_SHADER_HIZ_SHADOWS), true);

	GetShader(string(LG_SHADER), false);
	GetShader(string(LG_SHADER_SPHERE), false);

	GetShader(string(ENVPROBS_MAT), true);
	GetShader(string(ENVPROBS_MIPS_MAT), true);
	GetShader(string(ENVPROBS_DIFF_MAT), true);

	GetShader(string(PATH_SHADERS"gui/color"), true);
	GetShader(string(PATH_SHADERS"gui/font_default"), true);
	GetShader(string(PATH_SHADERS"gui/group_arrow"), true);
	GetShader(string(PATH_SHADERS"gui/h_picker"), true);
	GetShader(string(PATH_SHADERS"gui/rect"), true);
	GetShader(string(PATH_SHADERS"gui/rect_icon"), true);
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

uint16_t ShaderMgr::GetShader(string& name, bool simple)
{
	uint32_t res = SHADER_NULL;
	if(name.length() == 0)
		return res;

	res = FindShaderInList(name);
	if(res != SHADER_NULL)
		return res;

	res = AddShaderToList(name, simple);
	if(res != SHADER_NULL)
		return res;

	ERR("Cant load shader %s , using no shader!", name.c_str());
	return res;
}

uint16_t ShaderMgr::AddShaderToList(string& name, bool simple)
{
	if(shader_free.size() == 0)
	{
		ERR("Shader resources amount overflow!");
		return SHADER_NULL;
	}

	uint32_t idx = shader_free.front();
	auto& handle = shader_array[idx];

	if(simple)
		handle.shader = (BaseShader*) new SimpleShader(name);
	else
		handle.shader = (BaseShader*) new Shader(name);

	if(handle.shader->IsError())
	{
		_DELETE(handle.shader);
		return SHADER_NULL;
	}
	LOG("Shader loaded %s", name.c_str());

	handle.name = name;
	handle.refcount = 1;

	shader_map.insert(make_pair(name, idx));
	shader_free.pop_front();

	return idx;
}

uint16_t ShaderMgr::FindShaderInList(string& name)
{
	auto it = shader_map.find(name);
	if(it == shader_map.end())
		return SHADER_NULL;

	auto& handle = shader_array[it->second];
	handle.refcount++;
	return it->second;
}

void ShaderMgr::DeleteShader(uint16_t id)
{
	if(id == SHADER_NULL)
		return;
	
	auto& handle = shader_array[id];

	if(handle.refcount == 1)
	{
		_DELETE(handle.shader);
		LOG("Shader droped %s", handle.name.c_str());
		handle.refcount = 0;

		shader_free.push_back(id);

		shader_map.erase(handle.name);

		handle.name.erase();
	}
	else if(handle.refcount == 0)
	{
		WRN("Shader %s has already deleted!", handle.name.c_str());
	}
	else
		handle.refcount--;
}

#ifdef _DEV
void ShaderMgr::UpdateShaders()
{
	for(auto& it: shader_map)
	{
		auto& handle = shader_array[it.second];

		string srcFilename = it.first + EXT_SHADER_SOURCE;

		uint32_t last_date = FileIO::GetDateModifRaw(srcFilename);
		if(last_date == handle.shader->GetSrcDate())
			continue;
				
		BaseShader* newShader;
		if(handle.shader->IsSimple())
			newShader = (BaseShader*) new SimpleShader((string&)it.first);
		else
			newShader = (BaseShader*) new Shader((string&)it.first);

		BaseShader* remove = handle.shader;

		handle.shader = newShader;
		handle.shader->SetSrcDate(last_date);

		_DELETE(remove);
	}
}
#endif