#include "stdafx.h"
#include "RenderState.h"
#include "Render.h"
#include "macros.h"
#include "StringToData.h"

using namespace EngineCore;

RenderStateMgr *RenderStateMgr::instance = nullptr;

uint16_t RenderStateMgr::GetDepthState(D3D11_DEPTH_STENCIL_DESC& desc)
{
	string rawdata((char*)(&desc), sizeof(D3D11_DEPTH_STENCIL_DESC));
	auto it = instance->depth_map.find(rawdata);
	if(it != instance->depth_map.end())
		return it->second;
		
	if(instance->depth_free.size() == 0)
	{
		ERR("Depth states amount overflow!");
		return STATE_NULL;
	}

	ID3D11DepthStencilState* state = nullptr;
	if( FAILED( DEVICE->CreateDepthStencilState(&desc, &state) ) )
	{
		ERR("Failed to create depth stencil state!");
		return STATE_NULL;
	}

	uint32_t idx = instance->depth_free.front();
	instance->depth_array[idx] = state;

	instance->depth_map.insert(make_pair(rawdata, idx));
	instance->depth_free.pop_front();

	return idx;
}

uint16_t RenderStateMgr::GetBlendState(D3D11_BLEND_DESC& desc)
{
	string rawdata((char*)(&desc), sizeof(D3D11_BLEND_DESC));
	auto it = instance->blend_map.find(rawdata);
	if(it != instance->blend_map.end())
		return it->second;
	
	if(instance->blend_free.size() == 0)
	{
		ERR("Blend states amount overflow!");
		return STATE_NULL;
	}

	ID3D11BlendState* state = nullptr;
	if( FAILED( DEVICE->CreateBlendState(&desc, &state) ) )
	{
		ERR("Failed to create blend state!");
		return STATE_NULL;
	}

	uint32_t idx = instance->blend_free.front();
	instance->blend_array[idx] = state;

	instance->blend_map.insert(make_pair(rawdata, idx));
	instance->blend_free.pop_front();

	return idx;
}

uint16_t RenderStateMgr::GetRSState(D3D11_RASTERIZER_DESC& desc)
{
	// todo: from configs
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = true;

	string rawdata((char*)(&desc), sizeof(D3D11_RASTERIZER_DESC));
	auto it = instance->rast_map.find(rawdata);
	if(it != instance->rast_map.end())
		return it->second;
	
	if(instance->rast_free.size() == 0)
	{
		ERR("Rasterizer states amount overflow!");
		return STATE_NULL;
	}

	ID3D11RasterizerState* state = nullptr;
	if( FAILED( DEVICE->CreateRasterizerState(&desc, &state) ) )
	{
		ERR("Failed to create rasterizer state!");
		return STATE_NULL;
	}

	uint32_t idx = instance->rast_free.front();
	instance->rast_array[idx] = state;

	instance->rast_map.insert(make_pair(rawdata, idx));
	instance->rast_free.pop_front();

	return idx;
}

RenderStateMgr::RenderStateMgr()
{
	if(!instance)
	{
		instance = this;
		
		depth_array.resize(STATES_MAX_COUNT);
		depth_array.assign(nullptr);
		depth_free.resize(STATES_MAX_COUNT);
		for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
			depth_free[i] = i;
		depth_map.reserve(STATES_MAX_COUNT);
		
		blend_array.resize(STATES_MAX_COUNT);
		blend_array.assign(nullptr);
		blend_free.resize(STATES_MAX_COUNT);
		for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
			blend_free[i] = i;
		blend_map.reserve(STATES_MAX_COUNT);
		
		rast_array.resize(STATES_MAX_COUNT);
		rast_array.assign(nullptr);
		rast_free.resize(STATES_MAX_COUNT);
		for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
			rast_free[i] = i;
		rast_map.reserve(STATES_MAX_COUNT);
	}
	else
		ERR("Only one instance of RenderStatesMgr is allowed!");
}

RenderStateMgr::~RenderStateMgr()
{
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		_RELEASE(depth_array[i]);
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		_RELEASE(blend_array[i]);
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		_RELEASE(rast_array[i]);

	instance = nullptr;
}

bool RenderStateMgr::SetDefault()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	uint16_t state = GetDepthState(depthStencilDesc);
	if(state == STATE_NULL)
		return false;
	Render::OMSetDepthState(state);

	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.IndependentBlendEnable = false;
	blendStateDesc.AlphaToCoverageEnable = false;
	blendStateDesc.RenderTarget[0].BlendEnable = false;

	state = GetBlendState(blendStateDesc);
	if(state == STATE_NULL)
		return false;
	Render::OMSetBlendState(state);

	D3D11_RASTERIZER_DESC rastStatedDesc;
	ZeroMemory(&rastStatedDesc, sizeof(D3D11_RASTERIZER_DESC));
	rastStatedDesc.CullMode = D3D11_CULL_BACK;
	rastStatedDesc.DepthBias = 0;
	rastStatedDesc.DepthBiasClamp = 0.0f;
	rastStatedDesc.DepthClipEnable = true;
	rastStatedDesc.FillMode = D3D11_FILL_SOLID;
	rastStatedDesc.FrontCounterClockwise = false;
	rastStatedDesc.ScissorEnable = false;
	rastStatedDesc.SlopeScaledDepthBias = 0.0f;

	state = GetRSState(rastStatedDesc);
	if(state == STATE_NULL)
		return false;
	Render::RSSetState(state);

	Render::SetTopology(IA_TOPOLOGY::TRISLIST);

	return true;
}

// SAMPLERS

SamplerStateMgr *SamplerStateMgr::instance = nullptr;

ID3D11SamplerState* SamplerStateMgr::GetSampler(string name)
{
	auto it = instance->sampler_map.find(name);
	if(it != instance->sampler_map.end())
		return it->second;
		
	ERR("Sampler %s does not exist!", name.c_str());
	return nullptr;
}

SamplerStateMgr::SamplerStateMgr()
{
	if(!instance)
	{
		instance = this;
		sampler_map.reserve(SAMPLERS_INIT_COUNT);
	}
	else
		ERR("Only one instance of SamplerStateMgr is allowed!");
}

SamplerStateMgr::~SamplerStateMgr()
{
	for(auto& it: sampler_map)
		_RELEASE(it.second);

	instance = nullptr;
}

bool SamplerStateMgr::LoadSamplers()
{
#ifdef _DEV
	uint32_t sourceDate = FileIO::GetDateModifRawS(SAMPLERS_SOURCE);
	if(!FileIO::IsExistS(SAMPLERS_BIN) || sourceDate > FileIO::GetDateModifRawS(SAMPLERS_BIN))
	{
		if(!CompileSamplers(sourceDate))
		{
			ERR("Cant read samplers source file %s!", SAMPLERS_SOURCE);
			return false;
		}
	}
	else
#else
	if(!FileIO::IsExistS(SAMPLERS_BIN))
	{
		ERR("Samplers file %s does not exist!", SAMPLERS_BIN);
		return false;
	}
#endif
	{	
		uint8_t* s_data = nullptr;
		uint32_t data_size = 0;

		if( !(s_data = FileIO::ReadFileDataS(SAMPLERS_BIN, &data_size)) )
		{
			ERR("Cant read samplers file %s", SAMPLERS_BIN);
			return false;
		}

		uint32_t file_date = *((uint32_t*)s_data);
			
		uint8_t* t_data = s_data + sizeof(uint32_t);

		uint16_t samplesCount = *(t_data);
		t_data += sizeof(uint16_t);

		for(uint8_t i = 0; i < samplesCount; i++)
		{
			string samplerName((char*)t_data, 0, SAMPLER_STR_LEN);
			t_data += SAMPLER_STR_LEN;
			
			D3D11_SAMPLER_DESC* desc = (D3D11_SAMPLER_DESC*)t_data;
			t_data += sizeof(D3D11_SAMPLER_DESC);

			ID3D11SamplerState* sampler = nullptr;
			if( FAILED( DEVICE->CreateSamplerState(desc, &sampler) ) )
			{
				ERR("Failed to create sampler %s!", samplerName.c_str());
				continue;
			}

			sampler_map.insert(make_pair(samplerName, sampler));
		}
		_DELETE_ARRAY(s_data);
	}
	return true;
}

#ifdef _DEV
bool SamplerStateMgr::CompileSamplers(uint32_t sourceDate)
{
	FileIO techSource(SAMPLERS_SOURCE);
	auto root = techSource.Root();
	if(!root)
		return false;

	for(auto &it: *root)
	{
		string samplerName = it.first;

		if(!it.second.node)
		{
			ERR("Sampler %s must have params in %s", samplerName.c_str(), SAMPLERS_SOURCE);
			continue;
		}

		D3D11_SAMPLER_DESC desc;

		desc.AddressU = StringToData::GetAddressType(techSource.ReadString("AddressU", it.second.node));
		desc.AddressV = StringToData::GetAddressType(techSource.ReadString("AddressV", it.second.node));
		desc.AddressW = StringToData::GetAddressType(techSource.ReadString("AddressW", it.second.node));

		desc.ComparisonFunc = StringToData::GetCompareFunc(techSource.ReadString("ComparisonFunc", it.second.node));
		desc.Filter = StringToData::GetFilter(techSource.ReadString("Filter", it.second.node));

		Vector4 borderColor = techSource.ReadFloat4("BorderColor", it.second.node);
		desc.BorderColor[0] = borderColor.x;
		desc.BorderColor[1] = borderColor.y;
		desc.BorderColor[2] = borderColor.z;
		desc.BorderColor[3] = borderColor.w;

		desc.MipLODBias = techSource.ReadFloat("MipLODBias", it.second.node);
		
		if(it.second.node->find("MaxLOD") == it.second.node->end())
			desc.MaxLOD = D3D11_FLOAT32_MAX;
		else
			desc.MaxLOD = techSource.ReadFloat("MaxLOD", it.second.node);

		desc.MinLOD = techSource.ReadFloat("MinLOD", it.second.node);
		desc.MaxAnisotropy = uint32_t(techSource.ReadInt("MaxAnisotropy", it.second.node));

		ID3D11SamplerState* sampler = nullptr;
		if( FAILED( DEVICE->CreateSamplerState(&desc, &sampler) ) )
		{
			ERR("Failed to create sampler %s!", samplerName.c_str());
			continue;
		}

		sampler_map.insert(make_pair(samplerName, sampler));
	}
	
	uint16_t samplersCount = (uint16_t)sampler_map.size();

	if(samplersCount == 0)
	{
		ERR("No samplers in %s", SAMPLERS_SOURCE);
		return false;
	}

	// save to binary
	uint32_t s_datasize = SAMPLER_SIZE * samplersCount + sizeof(uint16_t);
	uint8_t* s_data = new uint8_t[s_datasize];
	uint8_t* dataPtr = s_data;

	*((uint16_t*)dataPtr) = samplersCount;
	dataPtr += sizeof(uint16_t);

	for(auto &it: sampler_map)
	{
		memcpy((char*)dataPtr, it.first.c_str(), SAMPLER_STR_LEN);
		dataPtr += SAMPLER_STR_LEN;

		it.second->GetDesc((D3D11_SAMPLER_DESC*)dataPtr);
		dataPtr += sizeof(D3D11_SAMPLER_DESC);
	}
	
	if(FileIO::WriteFileDataS(SAMPLERS_BIN, s_data, s_datasize, sourceDate))
	{
		_DELETE_ARRAY(s_data);
		return true;
	}
	_DELETE_ARRAY(s_data);
	return false;
}
#endif