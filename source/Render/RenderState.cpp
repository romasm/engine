#include "stdafx.h"
#include "RenderState.h"
#include "Render.h"
#include "macros.h"
#include "StringToData.h"

using namespace EngineCore;

RenderStateMgr *RenderStateMgr::instance = nullptr;

uint16_t RenderStateMgr::GetDepthState(RHI::DepthStencilDesc& desc)
{
	string rawdata((char*)(&desc), sizeof(RHI::DepthStencilDesc));
	auto it = instance->depth_map.find(rawdata);
	if(it != instance->depth_map.end())
		return it->second;

	if(instance->depth_free.size() == 0)
	{
		ERR("Depth states amount overflow!");
		return STATE_NULL;
	}

	void* state = GFX_DEVICE->CreateDepthStencilStateObject(desc);

	uint32_t idx = instance->depth_free.front();
	instance->depth_array[idx] = state;

	instance->depth_map.insert(make_pair(rawdata, idx));
	instance->depth_free.pop_front();

	return idx;
}

uint16_t RenderStateMgr::GetBlendState(RHI::BlendDesc& desc)
{
	string rawdata((char*)(&desc), sizeof(RHI::BlendDesc));
	auto it = instance->blend_map.find(rawdata);
	if(it != instance->blend_map.end())
		return it->second;

	if(instance->blend_free.size() == 0)
	{
		ERR("Blend states amount overflow!");
		return STATE_NULL;
	}

	void* state = GFX_DEVICE->CreateBlendStateObject(desc);

	uint32_t idx = instance->blend_free.front();
	instance->blend_array[idx] = state;

	instance->blend_map.insert(make_pair(rawdata, idx));
	instance->blend_free.pop_front();

	return idx;
}

uint16_t RenderStateMgr::GetRSState(RHI::RasterizerDesc& desc)
{
	// todo: from configs
	desc.multisampleEnable = false;
	desc.antialiasedLineEnable = true;

	string rawdata((char*)(&desc), sizeof(RHI::RasterizerDesc));
	auto it = instance->rast_map.find(rawdata);
	if(it != instance->rast_map.end())
		return it->second;

	if(instance->rast_free.size() == 0)
	{
		ERR("Rasterizer states amount overflow!");
		return STATE_NULL;
	}

	void* state = GFX_DEVICE->CreateRasterizerStateObject(desc);

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
	// DX11 state objects are COM — Release them; DX12 stores nullptr
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		if(depth_array[i]) static_cast<IUnknown*>(depth_array[i])->Release();
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		if(blend_array[i]) static_cast<IUnknown*>(blend_array[i])->Release();
	for(uint32_t i=0; i<STATES_MAX_COUNT; i++)
		if(rast_array[i]) static_cast<IUnknown*>(rast_array[i])->Release();

	instance = nullptr;
}

bool RenderStateMgr::SetDefault()
{
	RHI::DepthStencilDesc depthStencilDesc;
	memset(&depthStencilDesc, 0, sizeof(RHI::DepthStencilDesc));
	depthStencilDesc.depthEnable = true;
	depthStencilDesc.depthWriteMask = RHI::DepthWriteAll;
	depthStencilDesc.depthFunc = RHI::ComparisonLess;

	uint16_t state = GetDepthState(depthStencilDesc);
	if(state == STATE_NULL)
		return false;
	GFX_CMD->SetDepthStencilState(GetDepthStatePtr(state), 1);

	RHI::BlendDesc blendStateDesc;
	memset(&blendStateDesc, 0, sizeof(RHI::BlendDesc));
	blendStateDesc.independentBlendEnable = false;
	blendStateDesc.alphaToCoverageEnable = false;
	blendStateDesc.renderTarget[0].blendEnable = false;

	state = GetBlendState(blendStateDesc);
	if(state == STATE_NULL)
		return false;
	GFX_CMD->SetBlendState(GetBlendStatePtr(state), nullptr, 0xffffffff);

	RHI::RasterizerDesc rastStatedDesc;
	memset(&rastStatedDesc, 0, sizeof(RHI::RasterizerDesc));
	rastStatedDesc.cullMode = RHI::CullBack;
	rastStatedDesc.depthBias = 0;
	rastStatedDesc.depthBiasClamp = 0.0f;
	rastStatedDesc.depthClipEnable = true;
	rastStatedDesc.fillMode = RHI::FillSolid;
	rastStatedDesc.frontCounterClockwise = false;
	rastStatedDesc.scissorEnable = false;
	rastStatedDesc.slopeScaledDepthBias = 0.0f;

	state = GetRSState(rastStatedDesc);
	if(state == STATE_NULL)
		return false;
	GFX_CMD->SetRasterizerState(GetRSStatePtr(state));

	GFX_CMD->SetTopology(RHI::Topology::TriangleList);

	return true;
}

// SAMPLERS

SamplerStateMgr *SamplerStateMgr::instance = nullptr;

RHI::GfxSampler* SamplerStateMgr::GetSampler(string name)
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
		delete it.second;

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

			RHI::SamplerDesc* desc = reinterpret_cast<RHI::SamplerDesc*>(t_data);
			t_data += sizeof(RHI::SamplerDesc);

			RHI::GfxSampler* sampler = GFX_DEVICE->CreateSampler(*desc);
			if(!sampler)
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

	// Keep descs for binary serialization (GfxSampler has no GetDesc())
	std::vector<std::pair<string, RHI::SamplerDesc>> samplerDescs;

	for(auto &it: *root)
	{
		string samplerName = it.first;

		if(!it.second.node)
		{
			ERR("Sampler %s must have params in %s", samplerName.c_str(), SAMPLERS_SOURCE);
			continue;
		}

		RHI::SamplerDesc desc;
		memset(&desc, 0, sizeof(desc));

		desc.addressU = StringToData::GetAddressType(techSource.ReadString("AddressU", it.second.node));
		desc.addressV = StringToData::GetAddressType(techSource.ReadString("AddressV", it.second.node));
		desc.addressW = StringToData::GetAddressType(techSource.ReadString("AddressW", it.second.node));

		desc.comparisonFunc = StringToData::GetCompareFunc(techSource.ReadString("ComparisonFunc", it.second.node));
		desc.filter = StringToData::GetFilter(techSource.ReadString("Filter", it.second.node));

		Vector4 borderColor = techSource.ReadFloat4("BorderColor", it.second.node);
		desc.borderColor[0] = borderColor.x;
		desc.borderColor[1] = borderColor.y;
		desc.borderColor[2] = borderColor.z;
		desc.borderColor[3] = borderColor.w;

		desc.mipLODBias = techSource.ReadFloat("MipLODBias", it.second.node);

		if(it.second.node->find("MaxLOD") == it.second.node->end())
			desc.maxLOD = FLT_MAX;
		else
			desc.maxLOD = techSource.ReadFloat("MaxLOD", it.second.node);

		desc.minLOD = techSource.ReadFloat("MinLOD", it.second.node);
		desc.maxAnisotropy = uint32_t(techSource.ReadInt("MaxAnisotropy", it.second.node));

		RHI::GfxSampler* sampler = GFX_DEVICE->CreateSampler(desc);
		if(!sampler)
		{
			ERR("Failed to create sampler %s!", samplerName.c_str());
			continue;
		}

		sampler_map.insert(make_pair(samplerName, sampler));
		samplerDescs.push_back(make_pair(samplerName, desc));
	}

	uint16_t samplersCount = (uint16_t)samplerDescs.size();

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

	for(auto &entry: samplerDescs)
	{
		memcpy((char*)dataPtr, entry.first.c_str(), SAMPLER_STR_LEN);
		dataPtr += SAMPLER_STR_LEN;

		memcpy(dataPtr, &entry.second, sizeof(RHI::SamplerDesc));
		dataPtr += sizeof(RHI::SamplerDesc);
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
