#include "stdafx.h"

#include "EnvProbMgr.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Frustum.h"
#include "Utils\Profiler.h"
#include "DX11Types.h"

using namespace EngineCore;

EnvProbMgr::EnvProbMgr(bool onlySky)
{
	hqProbArray = nullptr;
	hqProbArraySRV = nullptr;
	sqProbArray = nullptr;
	sqProbArraySRV = nullptr;
	lqProbArray = nullptr;
	lqProbArraySRV = nullptr;

 	if(!InitBuffers())
	{
		ERR("Cant init EnvProbs buffers");
	}

	hqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_HQ);
	sqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_SQ);
	lqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_LQ);

	hqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_HQ);
	sqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_SQ);
	lqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_LQ);
}

EnvProbMgr::~EnvProbMgr()
{
	hqProbsBufferGPU.Release();
	sqProbsBufferGPU.Release();
	lqProbsBufferGPU.Release();

	_DELETE(hqProbArraySRV);
	_DELETE(hqProbArray);
	_DELETE(sqProbArraySRV);
	_DELETE(sqProbArray);
	_DELETE(lqProbArraySRV);
	_DELETE(lqProbArray);
}

bool EnvProbMgr::InitBuffers()
{
	DXGI_FORMAT format = EnvProbSystem::GetFormat(EnvProbQuality::EP_HIGH);
	int32_t resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_HIGH);
	int32_t mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_HIGH);

	// Cube array textures use D3D11_RESOURCE_MISC_TEXTURECUBE with arraySize*6;
	// raw DX11 creation is kept because RHI CreateTexture doesn't handle cube arrays.
	D3D11_TEXTURE2D_DESC cubeArrayDesc;
	ZeroMemory(&cubeArrayDesc, sizeof(cubeArrayDesc));
	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_HQ * 6;
	cubeArrayDesc.SampleDesc.Count = 1;
	cubeArrayDesc.SampleDesc.Quality = 0;
	cubeArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	cubeArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	cubeArrayDesc.CPUAccessFlags = 0;
	cubeArrayDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	cubeArrayDesc.Format = format;
	{
		ID3D11Texture2D* rawTexture = nullptr;
		if( FAILED(Render::Device()->CreateTexture2D(&cubeArrayDesc, NULL, &rawTexture)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11Texture();
		wrapped->texture2D = rawTexture;
		wrapped->width = resolution;
		wrapped->height = resolution;
		wrapped->depth = ENVPROBS_FRAME_COUNT_HQ * 6;
		wrapped->mipLevels = mipCount;
		wrapped->format = format;
		hqProbArray = wrapped;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC cubeArraySRVDesc;
	ZeroMemory(&cubeArraySRVDesc, sizeof(cubeArraySRVDesc));
	cubeArraySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.MostDetailedMip = 0;
	cubeArraySRVDesc.TextureCubeArray.First2DArrayFace = 0;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_HQ;
	cubeArraySRVDesc.Format = format;
	{
		ID3D11ShaderResourceView* rawSRV = nullptr;
		if( FAILED(Render::Device()->CreateShaderResourceView(RHI::DX11::Cast(hqProbArray)->texture2D, &cubeArraySRVDesc, &rawSRV)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11SRV();
		wrapped->view = rawSRV;
		hqProbArraySRV = wrapped;
	}

	format = EnvProbSystem::GetFormat(EnvProbQuality::EP_STANDART);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_STANDART);
	mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_STANDART);

	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_SQ * 6;
	cubeArrayDesc.Format = format;
	{
		ID3D11Texture2D* rawTexture = nullptr;
		if( FAILED(Render::Device()->CreateTexture2D(&cubeArrayDesc, NULL, &rawTexture)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11Texture();
		wrapped->texture2D = rawTexture;
		wrapped->width = resolution;
		wrapped->height = resolution;
		wrapped->depth = ENVPROBS_FRAME_COUNT_SQ * 6;
		wrapped->mipLevels = mipCount;
		wrapped->format = format;
		sqProbArray = wrapped;
	}

	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_SQ;
	cubeArraySRVDesc.Format = format;
	{
		ID3D11ShaderResourceView* rawSRV = nullptr;
		if( FAILED(Render::Device()->CreateShaderResourceView(RHI::DX11::Cast(sqProbArray)->texture2D, &cubeArraySRVDesc, &rawSRV)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11SRV();
		wrapped->view = rawSRV;
		sqProbArraySRV = wrapped;
	}

	format = EnvProbSystem::GetFormat(EnvProbQuality::EP_LOW);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_LOW);
	mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_LOW);

	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_LQ * 6;
	cubeArrayDesc.Format = format;
	{
		ID3D11Texture2D* rawTexture = nullptr;
		if( FAILED(Render::Device()->CreateTexture2D(&cubeArrayDesc, NULL, &rawTexture)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11Texture();
		wrapped->texture2D = rawTexture;
		wrapped->width = resolution;
		wrapped->height = resolution;
		wrapped->depth = ENVPROBS_FRAME_COUNT_LQ * 6;
		wrapped->mipLevels = mipCount;
		wrapped->format = format;
		lqProbArray = wrapped;
	}

	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_LQ;
	cubeArraySRVDesc.Format = format;
	{
		ID3D11ShaderResourceView* rawSRV = nullptr;
		if( FAILED(Render::Device()->CreateShaderResourceView(RHI::DX11::Cast(lqProbArray)->texture2D, &cubeArraySRVDesc, &rawSRV)) )
			return false;
		auto* wrapped = new RHI::DX11::DX11SRV();
		wrapped->view = rawSRV;
		lqProbArraySRV = wrapped;
	}

	hqProbsBufferGPU = Buffer::CreateStructedBuffer(ENVPROBS_FRAME_COUNT_HQ, sizeof(EnvProbRenderData), true);
	sqProbsBufferGPU = Buffer::CreateStructedBuffer(ENVPROBS_FRAME_COUNT_SQ, sizeof(EnvProbRenderData), true);
	lqProbsBufferGPU = Buffer::CreateStructedBuffer(ENVPROBS_FRAME_COUNT_LQ, sizeof(EnvProbRenderData), true);

	return true;
}

void EnvProbMgr::AddEnvProb(const EnvProbData& data, const Vector3& camPos)
{
	Vector3 fromCamera = camPos - data.position;
	float priorityDist = max(0.0f, fromCamera.Length() - data.distance * 0.5f) * data.priority;

	EnvProbData* targetData = nullptr;
	switch (data.quality)
	{
	case EnvProbQuality::EP_HIGH:
		targetData = hqEnvProbs.push_back();
		if(!targetData)
			ERR("HQ EnvProbes frame queue overflow!");
		break;

	case EnvProbQuality::EP_STANDART:
		targetData = sqEnvProbs.push_back();
		if(!targetData)
			WRN("SQ EnvProbes frame queue overflow!");
		break;

	case EnvProbQuality::EP_LOW:
		targetData = lqEnvProbs.push_back();
		if(!targetData)
			WRN("LQ EnvProbes frame queue overflow!");
		break;
	}

	if(!targetData)
		return;

	*targetData = data;
	targetData->priorityDist = priorityDist;
}

void EnvProbMgr::ForceUpdate(uint32_t probId)
{
	if(!hqRegedProbsPrev.empty())
		hqRegedProbsPrev.erase(probId);
	if (!sqRegedProbsPrev.empty())
		sqRegedProbsPrev.erase(probId);
	if (!lqRegedProbsPrev.empty())
		lqRegedProbsPrev.erase(probId);
}

bool CompareEnvProbs(EnvProbData& first, EnvProbData& second)
{
	return first.priority < second.priority;
}

bool CompareEnvProbsRenderData(EnvProbRenderData& first, EnvProbRenderData& second)
{
	return first.mipsTypeAdressPriority.w > second.mipsTypeAdressPriority.w;
}

void EnvProbMgr::PrepareEnvProbs()
{
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_HQ>(hqRegedProbs, hqRegedProbsPrev, hqEnvProbs, hqFreeProbIndex, hqProbArray, hqProbsBuffer);
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_SQ>(sqRegedProbs, sqRegedProbsPrev, sqEnvProbs, sqFreeProbIndex, sqProbArray, sqProbsBuffer);
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_LQ>(lqRegedProbs, lqRegedProbsPrev, lqEnvProbs, lqFreeProbIndex, lqProbArray, lqProbsBuffer);

	if(!hqProbsBuffer.empty())
		GFX_CMD->UpdateBuffer(hqProbsBufferGPU.buf, hqProbsBuffer.data(), sizeof(EnvProbRenderData) * hqProbsBuffer.size());
	if(!sqProbsBuffer.empty())
		GFX_CMD->UpdateBuffer(sqProbsBufferGPU.buf, sqProbsBuffer.data(), sizeof(EnvProbRenderData) * sqProbsBuffer.size());
	if(!lqProbsBuffer.empty())
		GFX_CMD->UpdateBuffer(lqProbsBufferGPU.buf, lqProbsBuffer.data(), sizeof(EnvProbRenderData) * lqProbsBuffer.size());
}

template<size_t FRAME_COUNT>
void EnvProbMgr::PrepareEnvProbsChannel( unordered_map<uint32_t, int32_t>& regedProbs, unordered_map<uint32_t, int32_t>& regedProbsPrev,
										SArray<EnvProbData, FRAME_COUNT * 4>& envProbs, SArray<int32_t, FRAME_COUNT>& freeProbIndex,
										RHI::GfxTexture* probArray, SArray<EnvProbRenderData, FRAME_COUNT>& probsBuffer )
{
	sort(envProbs.begin(), envProbs.end(), CompareEnvProbs );

	regedProbs.clear();
	probsBuffer.clear();

	freeProbIndex.resize(FRAME_COUNT);
	for(int32_t i = 0; i < FRAME_COUNT; i++)
	{
		freeProbIndex[i] = i;
	}

	const size_t arraySize = min<size_t>(FRAME_COUNT, envProbs.size());
	for(size_t i = 0; i < arraySize; i++)
	{
		EnvProbData& prob = envProbs[i];

		auto hasProb = regedProbsPrev.find(prob.probId);
		if(hasProb != regedProbsPrev.end())
		{
			regedProbs.insert(make_pair(prob.probId, hasProb->second));
			freeProbIndex[hasProb->second] = -1;
		}
		else
		{
			regedProbs.insert(make_pair(prob.probId, ENVPROBS_NEED_COPY_KEY));
		}
	}

	for (int32_t i = FRAME_COUNT - 1; i >= 0; i--)
	{
		if( freeProbIndex[i] == -1)
			freeProbIndex.erase_and_pop_back(i);
	}

	// Extract raw DX11 texture for subresource copies
	auto* dx11ProbArray = RHI::DX11::Cast(probArray);
	D3D11_TEXTURE2D_DESC desc;
	dx11ProbArray->texture2D->GetDesc(&desc);

	D3D11_BOX region;
	region.front = 0;
	region.back = 1;
	region.left = 0;
	region.top = 0;

	for(size_t i = 0; i < arraySize; i++)
	{
		EnvProbData& prob = envProbs[i];

		auto probSlot = regedProbs.find(prob.probId);
		if(probSlot->second == ENVPROBS_NEED_COPY_KEY)
		{
			const int32_t arrayId = freeProbIndex[0];
			probSlot->second = arrayId;
			freeProbIndex.erase_and_pop_back((size_t)0);
			
			auto textureCube = TEXTURE_GETPTR(prob.probId);
			auto* dx11View = RHI::DX11::Cast(textureCube)->view;
			ID3D11Resource* srcRes = nullptr;
			dx11View->GetResource(&srcRes);
			
			for(int32_t face = 0; face < 6; face++)
			{
				int32_t currentRes = desc.Width;
				for(uint32_t mipSlice = 0; mipSlice < prob.mips; mipSlice++)
				{
					region.right = currentRes;
					region.bottom = region.right;

					CONTEXT->CopySubresourceRegion(dx11ProbArray->texture2D, D3D11CalcSubresource(mipSlice, arrayId * 6 + face, desc.MipLevels), 0, 0, 0,
						srcRes, D3D11CalcSubresource(mipSlice, face, prob.mips), &region);

					currentRes /= 2;
				}
			}
		}

		probsBuffer.push_back(EnvProbRenderData(prob.position, prob.distance, prob.offset, prob.fade, prob.mips, prob.type, 
			probSlot->second, prob.priority, prob.shape, prob.invTransform));
	}

	sort(probsBuffer.begin(), probsBuffer.end(), CompareEnvProbsRenderData );
	swap(regedProbs, regedProbsPrev);
	envProbs.clear();
}