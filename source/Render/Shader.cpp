#include "stdafx.h"
#include "Shader.h"
#include "ShaderMgr.h"
#include "Material.h"
#include "ScenePipeline.h"
#include "StringToData.h"
#include "Render.h"

using namespace EngineCore;

BaseShader::BaseShader(const string& name)
{
	shaderName = name;
	filedate = 0;
#ifdef _DEV
	is_simple = false;
#endif
}

#ifdef _DEV
bool BaseShader::CompileTechniques(string& file, string& binFile, DArray<tech_desc>& techsDesc)
{
	LOG("Compiling techniques in %s", file.c_str());

	FileIO techSource(file);
	auto root = techSource.Root();
	if(!root)
		return false;

	auto point = file.rfind('.');
	string shaderPath = file.substr(0, point);

	for(auto &it: *root)
	{
		if(it.first.find(TECHIQUE_STR) == string::npos)
			continue;

		if(it.first.length() < TECHIQUE_STR_SIZE + 1)
		{
			ERR("Technique must have a name in %s", file.c_str());
			continue;
		}
		
		if(!it.second.node)
		{
			ERR("Technique %s must have params in %s", it.first.c_str(), file.c_str());
			continue;
		}

		tech_desc technique;

		technique.tech_id = StringToData::GetTechID(it.first);
		technique.queue = StringToData::GetQueueID(techSource.ReadString("Queue", it.second.node));

		technique.pixelShader = techSource.ReadString("PixelShader", it.second.node);
		if(technique.pixelShader == "NULL")
			technique.pixelShader = "";

		if(technique.pixelShader.size() > 0)
		{
			auto del = technique.pixelShader.rfind(' ');
			if(del == string::npos)
				technique.pixelShader = shaderPath + SHADER_NAME_DEL + technique.pixelShader;
			else
				technique.pixelShader[del] = SHADER_NAME_DEL;

			technique.pixelShader += SHADER_NAME_DEL + techSource.ReadString("PixelShaderDefines", it.second.node);
		}

		technique.vertexShader = techSource.ReadString("VertexShader", it.second.node);
		if(technique.vertexShader.size() > 0)
		{
			auto del = technique.vertexShader.rfind(' ');
			if(del == string::npos)
				technique.vertexShader = shaderPath + SHADER_NAME_DEL + technique.vertexShader;
			else
				technique.vertexShader[del] = SHADER_NAME_DEL;

			technique.vertexShader += SHADER_NAME_DEL + techSource.ReadString("VertexShaderDefines", it.second.node);
		}

		technique.hullShader = techSource.ReadString("HullShader", it.second.node);
		if(technique.hullShader == "NULL")
			technique.hullShader = "";

		if(technique.hullShader.size() > 0)
		{
			auto del = technique.hullShader.rfind(' ');
			if(del == string::npos)
				technique.hullShader = shaderPath + SHADER_NAME_DEL + technique.hullShader;
			else
				technique.hullShader[del] = SHADER_NAME_DEL;

			technique.hullShader += SHADER_NAME_DEL + techSource.ReadString("HullShaderDefines", it.second.node);
		}

		technique.domainShader = techSource.ReadString("DomainShader", it.second.node);
		if(technique.domainShader == "NULL")
			technique.domainShader = "";

		if(technique.domainShader.size() > 0)
		{
			auto del = technique.domainShader.rfind(' ');
			if(del == string::npos)
				technique.domainShader = shaderPath + SHADER_NAME_DEL + technique.domainShader;
			else
				technique.domainShader[del] = SHADER_NAME_DEL;

			technique.domainShader += SHADER_NAME_DEL + techSource.ReadString("DomainShaderDefines", it.second.node);
		}

		technique.geometryShader = techSource.ReadString("GeometryShader", it.second.node);
		if(technique.geometryShader == "NULL")
			technique.geometryShader = "";

		if(technique.geometryShader.size() > 0)
		{
			auto del = technique.geometryShader.rfind(' ');
			if(del == string::npos)
				technique.geometryShader = shaderPath + SHADER_NAME_DEL + technique.geometryShader;
			else
				technique.geometryShader[del] = SHADER_NAME_DEL;

			technique.geometryShader += SHADER_NAME_DEL + techSource.ReadString("GeometryShaderDefines", it.second.node);
		}

		// DepthStencilDesc
		memset(&technique.depthStencilDesc, 0, sizeof(RHI::DepthStencilDesc));
		technique.depthStencilDesc.depthEnable = techSource.ReadBool("DepthEnable", it.second.node);
		technique.depthStencilDesc.depthWriteMask = techSource.ReadBool("DepthWrite", it.second.node) ?
			RHI::DepthWriteAll : RHI::DepthWriteZero;

		technique.depthStencilDesc.depthFunc = StringToData::GetCompareFunc(techSource.ReadString("DepthFunc", it.second.node));

		technique.depthStencilDesc.stencilEnable = techSource.ReadBool("StencilEnable", it.second.node);
		technique.depthStencilDesc.stencilReadMask = techSource.ReadByte("StencilReadMask", it.second.node);
		technique.depthStencilDesc.stencilWriteMask = techSource.ReadByte("StencilWriteMask", it.second.node);

		technique.depthStencilDesc.frontFace.stencilFunc = StringToData::GetCompareFunc(techSource.ReadString("FrontFace.StencilFunc", it.second.node));
		technique.depthStencilDesc.frontFace.stencilFailOp = StringToData::GetStencilOp(techSource.ReadString("FrontFace.StencilFailOp", it.second.node));
		technique.depthStencilDesc.frontFace.stencilDepthFailOp = StringToData::GetStencilOp(techSource.ReadString("FrontFace.StencilDepthFailOp", it.second.node));
		technique.depthStencilDesc.frontFace.stencilPassOp = StringToData::GetStencilOp(techSource.ReadString("FrontFace.StencilPassOp", it.second.node));

		technique.depthStencilDesc.backFace.stencilFunc = StringToData::GetCompareFunc(techSource.ReadString("BackFace.StencilFunc", it.second.node));
		technique.depthStencilDesc.backFace.stencilFailOp = StringToData::GetStencilOp(techSource.ReadString("BackFace.StencilFailOp", it.second.node));
		technique.depthStencilDesc.backFace.stencilDepthFailOp = StringToData::GetStencilOp(techSource.ReadString("BackFace.StencilDepthFailOp", it.second.node));
		technique.depthStencilDesc.backFace.stencilPassOp = StringToData::GetStencilOp(techSource.ReadString("BackFace.StencilPassOp", it.second.node));

		// BlendDesc
		memset(&technique.blendDesc, 0, sizeof(RHI::BlendDesc));
		technique.blendDesc.alphaToCoverageEnable = techSource.ReadBool("AlphaToCoverageEnable", it.second.node);
		technique.blendDesc.independentBlendEnable = false; // todo???
		technique.blendDesc.renderTarget[0].blendEnable = techSource.ReadBool("BlendEnable", it.second.node);
		technique.blendDesc.renderTarget[0].blendOp = StringToData::GetBlendOp(techSource.ReadString("BlendOp", it.second.node));
		technique.blendDesc.renderTarget[0].blendOpAlpha = StringToData::GetBlendOp(techSource.ReadString("BlendOpAlpha", it.second.node));

		technique.blendDesc.renderTarget[0].srcBlend = StringToData::GetBlend(techSource.ReadString("SrcBlend", it.second.node));
		technique.blendDesc.renderTarget[0].destBlend = StringToData::GetBlend(techSource.ReadString("DestBlend", it.second.node));
		technique.blendDesc.renderTarget[0].srcBlendAlpha = StringToData::GetBlend(techSource.ReadString("SrcBlendAlpha", it.second.node));
		technique.blendDesc.renderTarget[0].destBlendAlpha = StringToData::GetBlend(techSource.ReadString("DestBlendAlpha", it.second.node));

		if(techSource.IsNodeExist("RenderTargetWriteMask", it.second.node))
			technique.blendDesc.renderTarget[0].renderTargetWriteMask = StringToData::GetRTWriteMask(techSource.ReadString("RenderTargetWriteMask", it.second.node));
		else
			technique.blendDesc.renderTarget[0].renderTargetWriteMask = RHI::ColorWriteAll;

		// RasterizerDesc
		memset(&technique.rastDesc, 0, sizeof(RHI::RasterizerDesc));

		if(techSource.IsNodeExist("AntialiasedLineEnable", it.second.node))
			technique.rastDesc.antialiasedLineEnable = techSource.ReadBool("AntialiasedLineEnable", it.second.node);
		else
			technique.rastDesc.antialiasedLineEnable = true;

		technique.rastDesc.depthBias = techSource.ReadInt("DepthBias", it.second.node);
		technique.rastDesc.depthBiasClamp = techSource.ReadFloat("DepthBiasClamp", it.second.node);

		if(techSource.IsNodeExist("DepthClipEnable", it.second.node))
			technique.rastDesc.depthClipEnable = techSource.ReadBool("DepthClipEnable", it.second.node);
		else
			technique.rastDesc.depthClipEnable = true;

		technique.rastDesc.frontCounterClockwise = techSource.ReadBool("FrontCounterClockwise", it.second.node);
		technique.rastDesc.multisampleEnable = techSource.ReadBool("MultisampleEnable", it.second.node);
		technique.rastDesc.scissorEnable = techSource.ReadBool("ScissorEnable", it.second.node);
		technique.rastDesc.slopeScaledDepthBias = techSource.ReadFloat("SlopeScaledDepthBias", it.second.node);

		technique.rastDesc.fillMode = StringToData::GetFill(techSource.ReadString("FillMode", it.second.node));
		technique.rastDesc.cullMode = StringToData::GetCull(techSource.ReadString("CullMode", it.second.node));

		techsDesc.push_back(technique);
	}
	
	uint8_t thechsCount = (uint8_t)techsDesc.size();

	if(thechsCount == 0)
	{
		ERR("No techniques in %s", file.c_str());
		return false;
	}

	// save to binary
	
	uint32_t s_datasize = TECHNIQUE_SIZE * thechsCount + sizeof(uint8_t);
	uint8_t* s_data = new uint8_t[s_datasize];
	uint8_t* dataPtr = s_data;

	*dataPtr = thechsCount;
	dataPtr += sizeof(uint8_t);

	for(auto &it: techsDesc)
	{
		*dataPtr = it.tech_id;
		dataPtr += sizeof(uint8_t);

		*((uint16_t*)dataPtr) = it.queue;
		dataPtr += sizeof(uint8_t);

		*((RHI::DepthStencilDesc*)dataPtr) = it.depthStencilDesc;
		dataPtr += sizeof(RHI::DepthStencilDesc);

		*((RHI::BlendDesc*)dataPtr) = it.blendDesc;
		dataPtr += sizeof(RHI::BlendDesc);

		*((RHI::RasterizerDesc*)dataPtr) = it.rastDesc;
		dataPtr += sizeof(RHI::RasterizerDesc);

		ZeroMemory(dataPtr, SHADERCODE_STR_LEN * 5);

		memcpy((char*)dataPtr, it.pixelShader.c_str(), it.pixelShader.size());
		dataPtr += SHADERCODE_STR_LEN;
		memcpy((char*)dataPtr, it.vertexShader.c_str(), it.vertexShader.size());
		dataPtr += SHADERCODE_STR_LEN;
		memcpy((char*)dataPtr, it.hullShader.c_str(), it.hullShader.size());
		dataPtr += SHADERCODE_STR_LEN;
		memcpy((char*)dataPtr, it.domainShader.c_str(), it.domainShader.size());
		dataPtr += SHADERCODE_STR_LEN;
		memcpy((char*)dataPtr, it.geometryShader.c_str(), it.geometryShader.size());
		dataPtr += SHADERCODE_STR_LEN;
	}

	if(FileIO::WriteFileData(binFile, s_data, s_datasize, filedate))
	{
		_DELETE_ARRAY(s_data);
		LOG_GOOD("Techniques in %s compiled successfully", file.c_str());
		return true;
	}
	_DELETE_ARRAY(s_data);
	return false;
}
#endif

///////////////////////////////////

Shader::Shader(const string& name) : BaseShader(name)
{
	techs_array.resize(TECHNIQUES_COUNT);
	if(!initShader())
		shaderName = "";
}

Shader::~Shader()
{
	for(uint8_t i = 0; i < TECHNIQUES_COUNT; i++)
	{
		if(techs_array[i].pso)
			GFX_DEVICE->DestroyPSO(techs_array[i].pso);
		for(uint8_t j = 0; j < 5; j++)
			ShaderCodeMgr::Get()->DeleteShaderCode(techs_array[i].shadersID[j], j);
	}
	shaderName.clear();
}

bool Shader::initShader()
{
	string techsBin = shaderName + EXT_SHADER_TECHS;

	DArray<tech_desc> techsDesc;
	uint8_t thechsCount = 0;

#ifdef _DEV
	string techsSource = shaderName + EXT_SHADER_SOURCE;

	uint32_t sourceDate = FileIO::GetDateModifRaw(techsSource);
	if(!FileIO::IsExist(techsBin) || FileIO::GetDateModifRaw(techsBin) < sourceDate)
	{
		filedate = sourceDate;
		if(!CompileTechniques(techsSource, techsBin, techsDesc))
		{
			ERR("Cant read technique source file %s !", techsSource.c_str());
			return false;
		}
		thechsCount = (uint8_t)techsDesc.size();
	}
	else
#else
	if(!FileIO::IsExist(techsBin))
	{
		ERR("Technique file %s does not exist!", techsBin.c_str());
		return false;
	}
#endif
	{	
		uint8_t* s_data = nullptr;
		uint32_t data_size = 0;

		if( !(s_data = FileIO::ReadFileData(techsBin, &data_size)) )
		{
			ERR("Cant read technique file %s !", techsBin.c_str());
			return false;
		}

		filedate = *((uint32_t*)s_data);
		
		uint8_t* t_data = s_data + sizeof(uint32_t);

		thechsCount = *(t_data);
		t_data += sizeof(uint8_t);

		techsDesc.reserve(thechsCount);

		for(uint8_t i = 0; i < thechsCount; i++)
		{
			tech_desc& tq = techsDesc.push_back();

			tq.tech_id = *t_data;
			t_data += sizeof(tq.tech_id);
			
			tq.queue = *t_data;
			t_data += sizeof(tq.queue);

			tq.depthStencilDesc = *((RHI::DepthStencilDesc*)t_data);
			t_data += sizeof(RHI::DepthStencilDesc);
			tq.blendDesc = *((RHI::BlendDesc*)t_data);
			t_data += sizeof(RHI::BlendDesc);
			tq.rastDesc = *((RHI::RasterizerDesc*)t_data);
			t_data += sizeof(RHI::RasterizerDesc);

			tq.pixelShader = string((char*)t_data, SHADERCODE_STR_LEN);
			tq.pixelShader = tq.pixelShader.substr( 0, tq.pixelShader.find(char(0)) );
			t_data += SHADERCODE_STR_LEN;
			tq.vertexShader = string((char*)t_data, SHADERCODE_STR_LEN);
			tq.vertexShader = tq.vertexShader.substr( 0, tq.vertexShader.find(char(0)) );
			t_data += SHADERCODE_STR_LEN;
			tq.hullShader = string((char*)t_data, SHADERCODE_STR_LEN);
			tq.hullShader = tq.hullShader.substr( 0, tq.hullShader.find(char(0)) );
			t_data += SHADERCODE_STR_LEN;
			tq.domainShader = string((char*)t_data, SHADERCODE_STR_LEN);
			tq.domainShader = tq.domainShader.substr( 0, tq.domainShader.find(char(0)) );
			t_data += SHADERCODE_STR_LEN;
			tq.geometryShader = string((char*)t_data, SHADERCODE_STR_LEN);
			tq.geometryShader = tq.geometryShader.substr( 0, tq.geometryShader.find(char(0)) );
			t_data += SHADERCODE_STR_LEN;
		}

		_DELETE_ARRAY(s_data);
	}

	for(uint8_t i = 0; i < thechsCount; i++)
	{
		technique_data& tech = techs_array[techsDesc[i].tech_id];
		
		tech.depthState = RenderStateMgr::GetDepthState(techsDesc[i].depthStencilDesc);
		tech.blendState = RenderStateMgr::GetBlendState(techsDesc[i].blendDesc);
		tech.rastState = RenderStateMgr::GetRSState(techsDesc[i].rastDesc);

		tech.queue = techsDesc[i].queue;

		if(techsDesc[i].vertexShader.empty())
		{
			ERR("Vertex shader must be specified in %s !", techsBin.c_str());
			continue;
		}

		tech.shadersID[SHADER_VS] = ShaderCodeMgr::Get()->GetShaderCode(techsDesc[i].vertexShader, SHADER_VS);
		if(tech.shadersID[SHADER_VS] == SHADER_NULL)
		{
			ERR("Cant get vertex shader %s in file %s !", techsDesc[i].vertexShader.c_str(), techsBin.c_str());
			continue;
		}

		if(!techsDesc[i].pixelShader.empty())
		{
			tech.shadersID[SHADER_PS] = ShaderCodeMgr::Get()->GetShaderCode(techsDesc[i].pixelShader, SHADER_PS);
			if(tech.shadersID[SHADER_PS] == SHADER_NULL)
			{
				ERR("Cant get pixel shader %s in file %s !", techsDesc[i].pixelShader.c_str(), techsBin.c_str());
				continue;
			}
		}
		else
			tech.shadersID[SHADER_PS] = SHADER_NULL;

		if(!techsDesc[i].hullShader.empty())
		{
			tech.shadersID[SHADER_HS] = ShaderCodeMgr::Get()->GetShaderCode(techsDesc[i].hullShader, SHADER_HS);
			if(tech.shadersID[SHADER_HS] == SHADER_NULL)
			{
				ERR("Cant get hull shader %s in file %s !", techsDesc[i].hullShader.c_str(), techsBin.c_str());
				continue;
			}
		}
		else
			tech.shadersID[SHADER_HS] = SHADER_NULL;

		if(!techsDesc[i].domainShader.empty())
		{
			tech.shadersID[SHADER_DS] = ShaderCodeMgr::Get()->GetShaderCode(techsDesc[i].domainShader, SHADER_DS);
			if(tech.shadersID[SHADER_DS] == SHADER_NULL)
			{
				ERR("Cant get domain shader %s in file %s !", techsDesc[i].domainShader.c_str(), techsBin.c_str());
				continue;
			}
		}
		else
			tech.shadersID[SHADER_DS] = SHADER_NULL;

		if(!techsDesc[i].geometryShader.empty())
		{
			tech.shadersID[SHADER_GS] = ShaderCodeMgr::Get()->GetShaderCode(techsDesc[i].geometryShader, SHADER_GS);
			if(tech.shadersID[SHADER_GS] == SHADER_NULL)
			{
				ERR("Cant get geometric shader %s in file %s !", techsDesc[i].geometryShader.c_str(), techsBin.c_str());
				continue;
			}
		}
		else
			tech.shadersID[SHADER_GS] = SHADER_NULL;

		RHI::GraphicsPSODesc psoDesc = {};
		psoDesc.vertexShaderID   = tech.shadersID[SHADER_VS];
		psoDesc.pixelShaderID    = tech.shadersID[SHADER_PS];
		psoDesc.hullShaderID     = tech.shadersID[SHADER_HS];
		psoDesc.domainShaderID   = tech.shadersID[SHADER_DS];
		psoDesc.geometryShaderID = tech.shadersID[SHADER_GS];
		psoDesc.depthStateID     = tech.depthState;
		psoDesc.blendStateID     = tech.blendState;
		psoDesc.rastStateID      = tech.rastState;
		psoDesc.depthStencilDesc = techsDesc[i].depthStencilDesc;
		psoDesc.blendDesc        = techsDesc[i].blendDesc;
		psoDesc.rasterizerDesc   = techsDesc[i].rastDesc;
		psoDesc.inputLayout      = nullptr;
		tech.pso = GFX_DEVICE->CreateGraphicsPSO(psoDesc);
	}
	return true;
}

void Shader::Set(TECHNIQUES tech)
{
	auto& tq = techs_array[tech];
	if(!tq.pso)
		return;

	GFX_CMD->SetPipelineState(tq.pso);
}

//////////////////////////////////////////////

SimpleShader::SimpleShader(const string& name) : BaseShader(name)
{
	if(!initShader())
		shaderName = "";
#ifdef _DEV
	is_simple = true;
#endif
}

SimpleShader::~SimpleShader()
{
	if(data.pso)
		GFX_DEVICE->DestroyPSO(data.pso);
	for(uint8_t j = 0; j < 2; j++)
		ShaderCodeMgr::Get()->DeleteShaderCode(data.shadersID[j], j);
	shaderName.clear();
}

bool SimpleShader::initShader()
{
	string techsBin = shaderName + EXT_SHADER_TECHS;

	DArray<tech_desc> techsDesc;
	techsDesc.reserve(1);

#ifdef _DEV
	string techsSource = shaderName + EXT_SHADER_SOURCE;

	uint32_t sourceDate = FileIO::GetDateModifRaw(techsSource);
	if(!FileIO::IsExist(techsBin) || FileIO::GetDateModifRaw(techsBin) < sourceDate)
	{
		filedate = sourceDate;
		if(!CompileTechniques(techsSource, techsBin, techsDesc))
		{
			ERR("Cant read technique source file %s !", techsSource.c_str());
			return false;
		}
	}
	else
#else
	if(!FileIO::IsExist(techsBin))
	{
		ERR("Technique file %s does not exist!", techsBin.c_str());
		return false;
	}
#endif
	{	
		uint8_t* s_data = nullptr;
		uint32_t data_size = 0;

		if( !(s_data = FileIO::ReadFileData(techsBin, &data_size)) )
		{
			ERR("Cant read technique file %s !", techsBin.c_str());
			return false;
		}

		filedate = *((uint32_t*)s_data);
		
		uint8_t* t_data = s_data + sizeof(uint32_t);

		uint8_t thechsCount = *(t_data);
		t_data += sizeof(uint8_t);

		if(thechsCount != 1)
			WRN("Wrong techniques count in %s !", techsBin.c_str());

		tech_desc& tq = techsDesc.push_back();

		tq.tech_id = *t_data;
		t_data += sizeof(uint8_t);
			
		tq.queue = *t_data;
		t_data += sizeof(uint8_t);

		tq.depthStencilDesc = *((RHI::DepthStencilDesc*)t_data);
		t_data += sizeof(RHI::DepthStencilDesc);
		tq.blendDesc = *((RHI::BlendDesc*)t_data);
		t_data += sizeof(RHI::BlendDesc);
		tq.rastDesc = *((RHI::RasterizerDesc*)t_data);
		t_data += sizeof(RHI::RasterizerDesc);

		tq.pixelShader = string((char*)t_data, SHADERCODE_STR_LEN);
		tq.pixelShader = tq.pixelShader.substr( 0, tq.pixelShader.find(char(0)) );
		t_data += SHADERCODE_STR_LEN;
		tq.vertexShader = string((char*)t_data, SHADERCODE_STR_LEN);
		tq.vertexShader = tq.vertexShader.substr( 0, tq.vertexShader.find(char(0)) );
		t_data += SHADERCODE_STR_LEN;
		tq.hullShader = string((char*)t_data, SHADERCODE_STR_LEN);
		tq.hullShader = tq.hullShader.substr( 0, tq.hullShader.find(char(0)) );
		t_data += SHADERCODE_STR_LEN;
		tq.domainShader = string((char*)t_data, SHADERCODE_STR_LEN);
		tq.domainShader = tq.domainShader.substr( 0, tq.domainShader.find(char(0)) );
		t_data += SHADERCODE_STR_LEN;
		tq.geometryShader = string((char*)t_data, SHADERCODE_STR_LEN);
		tq.geometryShader = tq.geometryShader.substr( 0, tq.geometryShader.find(char(0)) );
		t_data += SHADERCODE_STR_LEN;
		
		_DELETE_ARRAY(s_data);
	}

	auto& tqDesc = techsDesc[0];

	data.depthState = RenderStateMgr::GetDepthState(tqDesc.depthStencilDesc);
	data.blendState = RenderStateMgr::GetBlendState(tqDesc.blendDesc);
	data.rastState = RenderStateMgr::GetRSState(tqDesc.rastDesc);

	data.queue = tqDesc.queue;

	if(tqDesc.vertexShader.empty())
	{
		ERR("Vertex shader must be specified in %s !", techsBin.c_str());
		return false;
	}

	data.shadersID[SHADER_VS] = ShaderCodeMgr::Get()->GetShaderCode(tqDesc.vertexShader, SHADER_VS);
	if(data.shadersID[SHADER_VS] == SHADER_NULL)
	{
		ERR("Cant get vertex shader %s in file %s !", tqDesc.vertexShader.c_str(), techsBin.c_str());
		return false;
	}

	if(!tqDesc.pixelShader.empty())
	{
		data.shadersID[SHADER_PS] = ShaderCodeMgr::Get()->GetShaderCode(tqDesc.pixelShader, SHADER_PS);
		if(data.shadersID[SHADER_PS] == SHADER_NULL)
		{
			ERR("Cant get pixel shader %s in file %s !", tqDesc.pixelShader.c_str(), techsBin.c_str());
			return false;
		}
	}
	else
		data.shadersID[SHADER_PS] = SHADER_NULL;

	RHI::GraphicsPSODesc psoDesc = {};
	psoDesc.vertexShaderID   = data.shadersID[SHADER_VS];
	psoDesc.pixelShaderID    = data.shadersID[SHADER_PS];
	psoDesc.hullShaderID     = SHADER_NULL;
	psoDesc.domainShaderID   = SHADER_NULL;
	psoDesc.geometryShaderID = SHADER_NULL;
	psoDesc.depthStateID     = data.depthState;
	psoDesc.blendStateID     = data.blendState;
	psoDesc.rastStateID      = data.rastState;
	psoDesc.depthStencilDesc = tqDesc.depthStencilDesc;
	psoDesc.blendDesc        = tqDesc.blendDesc;
	psoDesc.rasterizerDesc   = tqDesc.rastDesc;
	psoDesc.inputLayout      = nullptr;
	data.pso = GFX_DEVICE->CreateGraphicsPSO(psoDesc);

	return true;
}

void SimpleShader::Set()
{
	if(!data.pso)
		return;

	GFX_CMD->SetPipelineState(data.pso);
}