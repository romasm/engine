#include "stdafx.h"
#include "Shader.h"
#include "ShaderMgr.h"
#include "Material.h"
#include "ScenePipeline.h"
#include "StringToData.h"

using namespace EngineCore;

BaseShader::BaseShader(string& name)
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
	FileIO techSource(file);
	auto root = techSource.Root();
	if(!root)
		return false;

	auto point = file.rfind('.');
	string shaderPath = file.substr(0, point);

	for(auto &it: *root)
	{
		if(it.first.find(TECHIQUE_STR_L) == string::npos)
			continue;

		if(it.first.length() < TECHIQUE_STR_SIZE + 1)
		{
			ERR("Technique must have a name in %s", file.c_str());
			continue;
		}

		//wstring techName = it.first.substr(TECHIQUE_STR_SIZE);

		if(!it.second.node)
		{
			ERR("Technique %ls must have params in %s", it.first.c_str(), file.c_str());
			continue;
		}

		tech_desc technique;

		technique.tech_id = StringToData::GetTechID(it.first);
		technique.queue = StringToData::GetQueueID(techSource.ReadString(L"Queue", it.second.node));

		technique.pixelShader = WstringToString(techSource.ReadString(L"PixelShader", it.second.node));
		if(technique.pixelShader == "NULL")
			technique.pixelShader = "";

		if(technique.pixelShader.size() > 0)
		{
			auto del = technique.pixelShader.rfind(' ');
			if(del == string::npos)
				technique.pixelShader = shaderPath + "_" + technique.pixelShader;
			else
				technique.pixelShader[del] = '_';
		}

		technique.vertexShader = WstringToString(techSource.ReadString(L"VertexShader", it.second.node));
		if(technique.vertexShader.size() > 0)
		{
			auto del = technique.vertexShader.rfind(' ');
			if(del == string::npos)
				technique.vertexShader = shaderPath + "_" + technique.vertexShader;
			else
				technique.vertexShader[del] = '_';
		}

		technique.hullShader = WstringToString(techSource.ReadString(L"HullShader", it.second.node));
		if(technique.hullShader == "NULL")
			technique.hullShader = "";

		if(technique.hullShader.size() > 0)
		{
			auto del = technique.hullShader.rfind(' ');
			if(del == string::npos)
				technique.hullShader = shaderPath + "_" + technique.hullShader;
			else
				technique.hullShader[del] = '_';
		}

		technique.domainShader = WstringToString(techSource.ReadString(L"DomainShader", it.second.node));
		if(technique.domainShader == "NULL")
			technique.domainShader = "";

		if(technique.domainShader.size() > 0)
		{
			auto del = technique.domainShader.rfind(' ');
			if(del == string::npos)
				technique.domainShader = shaderPath + "_" + technique.domainShader;
			else
				technique.domainShader[del] = '_';
		}

		technique.geometryShader = WstringToString(techSource.ReadString(L"GeometryShader", it.second.node));
		if(technique.geometryShader == "NULL")
			technique.geometryShader = "";

		if(technique.geometryShader.size() > 0)
		{
			auto del = technique.geometryShader.rfind(' ');
			if(del == string::npos)
				technique.geometryShader = shaderPath + "_" + technique.geometryShader;
			else
				technique.geometryShader[del] = '_';
		}

		// D3D11_DEPTH_STENCIL_DESC
		ZeroMemory(&technique.depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		technique.depthStencilDesc.DepthEnable = techSource.ReadBool(L"DepthEnable", it.second.node);
		technique.depthStencilDesc.DepthWriteMask = techSource.ReadBool(L"DepthWrite", it.second.node) ? 
			D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

		technique.depthStencilDesc.DepthFunc = StringToData::GetCompareFunc(techSource.ReadString(L"DepthFunc", it.second.node));
		
		technique.depthStencilDesc.StencilEnable = techSource.ReadBool(L"StencilEnable", it.second.node);
		technique.depthStencilDesc.StencilReadMask = techSource.ReadByte(L"StencilReadMask", it.second.node);
		technique.depthStencilDesc.StencilWriteMask = techSource.ReadByte(L"StencilWriteMask", it.second.node);

		technique.depthStencilDesc.FrontFace.StencilFunc = StringToData::GetCompareFunc(techSource.ReadString(L"FrontFace.StencilFunc", it.second.node));
		technique.depthStencilDesc.FrontFace.StencilFailOp = StringToData::GetStencilOp(techSource.ReadString(L"FrontFace.StencilFailOp", it.second.node));
		technique.depthStencilDesc.FrontFace.StencilDepthFailOp = StringToData::GetStencilOp(techSource.ReadString(L"FrontFace.StencilDepthFailOp", it.second.node));
		technique.depthStencilDesc.FrontFace.StencilPassOp = StringToData::GetStencilOp(techSource.ReadString(L"FrontFace.StencilPassOp", it.second.node));

		technique.depthStencilDesc.BackFace.StencilFunc = StringToData::GetCompareFunc(techSource.ReadString(L"BackFace.StencilFunc", it.second.node));
		technique.depthStencilDesc.BackFace.StencilFailOp = StringToData::GetStencilOp(techSource.ReadString(L"BackFace.StencilFailOp", it.second.node));
		technique.depthStencilDesc.BackFace.StencilDepthFailOp = StringToData::GetStencilOp(techSource.ReadString(L"BackFace.StencilDepthFailOp", it.second.node));
		technique.depthStencilDesc.BackFace.StencilPassOp = StringToData::GetStencilOp(techSource.ReadString(L"BackFace.StencilPassOp", it.second.node));
	
		// D3D11_BLEND_DESC
		ZeroMemory(&technique.blendDesc, sizeof(D3D11_BLEND_DESC));
		technique.blendDesc.AlphaToCoverageEnable = false;
		technique.blendDesc.IndependentBlendEnable = false;
		technique.blendDesc.RenderTarget[0].BlendEnable = techSource.ReadBool(L"BlendEnable", it.second.node);
		technique.blendDesc.RenderTarget[0].BlendOp = StringToData::GetBlendOp(techSource.ReadString(L"BlendOp", it.second.node));
		technique.blendDesc.RenderTarget[0].BlendOpAlpha = StringToData::GetBlendOp(techSource.ReadString(L"BlendOpAlpha", it.second.node));

		technique.blendDesc.RenderTarget[0].SrcBlend = StringToData::GetBlend(techSource.ReadString(L"SrcBlend", it.second.node));
		technique.blendDesc.RenderTarget[0].DestBlend = StringToData::GetBlend(techSource.ReadString(L"DestBlend", it.second.node));
		technique.blendDesc.RenderTarget[0].SrcBlendAlpha = StringToData::GetBlend(techSource.ReadString(L"SrcBlendAlpha", it.second.node));
		technique.blendDesc.RenderTarget[0].DestBlendAlpha = StringToData::GetBlend(techSource.ReadString(L"DestBlendAlpha", it.second.node));

		if(techSource.IsNodeExist(L"RenderTargetWriteMask", it.second.node))
			technique.blendDesc.RenderTarget[0].RenderTargetWriteMask = StringToData::GetRTWriteMask(techSource.ReadString(L"RenderTargetWriteMask", it.second.node));
		else
			technique.blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		// D3D11_RASTERIZER_DESC
		ZeroMemory(&technique.rastDesc, sizeof(D3D11_RASTERIZER_DESC));
		technique.rastDesc.AntialiasedLineEnable = true;
		technique.rastDesc.DepthBias = 0;
		technique.rastDesc.DepthBiasClamp = 0.0f;
		technique.rastDesc.DepthClipEnable = true;
		technique.rastDesc.FrontCounterClockwise = false;
		technique.rastDesc.MultisampleEnable = false;
		technique.rastDesc.ScissorEnable = false;
		technique.rastDesc.SlopeScaledDepthBias = 0.0f;

		technique.rastDesc.FillMode = StringToData::GetFill(techSource.ReadString(L"FillMode", it.second.node));
		technique.rastDesc.CullMode = StringToData::GetCull(techSource.ReadString(L"CullMode", it.second.node));

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

		*((D3D11_DEPTH_STENCIL_DESC*)dataPtr) = it.depthStencilDesc;
		dataPtr += sizeof(D3D11_DEPTH_STENCIL_DESC);

		*((D3D11_BLEND_DESC*)dataPtr) = it.blendDesc;
		dataPtr += sizeof(D3D11_BLEND_DESC);

		*((D3D11_RASTERIZER_DESC*)dataPtr) = it.rastDesc;
		dataPtr += sizeof(D3D11_RASTERIZER_DESC);

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
		return true;
	}
	_DELETE_ARRAY(s_data);
	return false;
}
#endif

///////////////////////////////////

Shader::Shader(string& name) : BaseShader(name)
{
	techs_array.resize(TECHNIQUES_COUNT);
	if(!initShader())
		shaderName = "";
}

Shader::~Shader()
{
	for(uint8_t i = 0; i < TECHNIQUES_COUNT; i++)
	{
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

			tq.depthStencilDesc = *((D3D11_DEPTH_STENCIL_DESC*)t_data);
			t_data += sizeof(D3D11_DEPTH_STENCIL_DESC);
			tq.blendDesc = *((D3D11_BLEND_DESC*)t_data);
			t_data += sizeof(D3D11_BLEND_DESC);
			tq.rastDesc = *((D3D11_RASTERIZER_DESC*)t_data);
			t_data += sizeof(D3D11_RASTERIZER_DESC);

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
	}
	return true;
}

void Shader::Set(TECHNIQUES tech)
{
	auto& tq = techs_array[tech];

	Render::OMSetDepthState(tq.depthState);
	Render::OMSetBlendState(tq.blendState);
	Render::RSSetState(tq.rastState);
	
	auto& shaderVS = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[SHADER_VS]);
	Render::VSSetShader((ID3D11VertexShader*)shaderVS.code, nullptr, 0);
	Render::IASetInputLayout(shaderVS.input.layout);
	if(!shaderVS.input.samplers.empty())
		Render::VSSetSamplers(0, (UINT)shaderVS.input.samplers.size(), shaderVS.input.samplers.data());
	
	auto& shaderPS = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[SHADER_PS]);
	Render::PSSetShader((ID3D11PixelShader*)shaderPS.code, nullptr, 0);
	if(!shaderPS.input.samplers.empty())
		Render::PSSetSamplers(0, (UINT)shaderPS.input.samplers.size(), shaderPS.input.samplers.data());
	
	auto& shaderHS = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[SHADER_HS]);
	Render::HSSetShader((ID3D11HullShader*)shaderHS.code, nullptr, 0);
	if(!shaderHS.input.samplers.empty())
		Render::HSSetSamplers(0, (UINT)shaderHS.input.samplers.size(), shaderHS.input.samplers.data());

	auto& shaderDS = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[SHADER_DS]);
	Render::DSSetShader((ID3D11DomainShader*)shaderDS.code, nullptr, 0);
	if(!shaderDS.input.samplers.empty())
		Render::DSSetSamplers(0, (UINT)shaderDS.input.samplers.size(), shaderDS.input.samplers.data());

	auto& shaderGS = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[SHADER_GS]);
	Render::GSSetShader((ID3D11GeometryShader*)shaderGS.code, nullptr, 0);
	if(!shaderGS.input.samplers.empty())
		Render::GSSetSamplers(0, (UINT)shaderGS.input.samplers.size(), shaderGS.input.samplers.data());
}

//////////////////////////////////////////////

SimpleShader::SimpleShader(string& name) : BaseShader(name)
{
	if(!initShader())
		shaderName = "";
#ifdef _DEV
	is_simple = true;
#endif
}

SimpleShader::~SimpleShader()
{
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

		tq.depthStencilDesc = *((D3D11_DEPTH_STENCIL_DESC*)t_data);
		t_data += sizeof(D3D11_DEPTH_STENCIL_DESC);
		tq.blendDesc = *((D3D11_BLEND_DESC*)t_data);
		t_data += sizeof(D3D11_BLEND_DESC);
		tq.rastDesc = *((D3D11_RASTERIZER_DESC*)t_data);
		t_data += sizeof(D3D11_RASTERIZER_DESC);

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
	return true;
}

void SimpleShader::Set()
{
	Render::OMSetDepthState(data.depthState);
	Render::OMSetBlendState(data.blendState);
	Render::RSSetState(data.rastState);
	
	auto& shaderVS = ShaderCodeMgr::GetShaderCodeRef(data.shadersID[SHADER_VS]);
	Render::VSSetShader((ID3D11VertexShader*)shaderVS.code, nullptr, 0);
	Render::IASetInputLayout(shaderVS.input.layout);
	if(!shaderVS.input.samplers.empty())
		Render::VSSetSamplers(0, (UINT)shaderVS.input.samplers.size(), shaderVS.input.samplers.data());
	
	auto& shaderPS = ShaderCodeMgr::GetShaderCodeRef(data.shadersID[SHADER_PS]);
	Render::PSSetShader((ID3D11PixelShader*)shaderPS.code, nullptr, 0);
	if(!shaderPS.input.samplers.empty())
		Render::PSSetSamplers(0, (UINT)shaderPS.input.samplers.size(), shaderPS.input.samplers.data());
}