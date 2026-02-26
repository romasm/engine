#include "stdafx.h"

#include "EnvProbMgr.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Frustum.h"
#include "Utils\Profiler.h"

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
	auto createCubeArray = [](int32_t cubeCount, int32_t resolution, int32_t mipCount, DXGI_FORMAT format,
		RHI::GfxTexture*& outTexture, RHI::GfxSRV*& outSRV) -> bool
	{
		RHI::TextureDesc texDesc = {};
		texDesc.dimension = RHI::TextureDimension::CubeMapArray;
		texDesc.width     = resolution;
		texDesc.height    = resolution;
		texDesc.depth     = cubeCount;
		texDesc.mipLevels = mipCount;
		texDesc.format    = format;
		texDesc.allowSRV  = true;
		outTexture = GFX_DEVICE->CreateTexture(texDesc);
		if(!outTexture) return false;

		outSRV = GFX_DEVICE->CreateSRV(outTexture, format, 0, mipCount);
		if(!outSRV) return false;
		return true;
	};

	DXGI_FORMAT format = EnvProbSystem::GetFormat(EnvProbQuality::EP_HIGH);
	int32_t resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_HIGH);
	int32_t mipCount   = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_HIGH);
	if(!createCubeArray(ENVPROBS_FRAME_COUNT_HQ, resolution, mipCount, format, hqProbArray, hqProbArraySRV))
		return false;

	format     = EnvProbSystem::GetFormat(EnvProbQuality::EP_STANDART);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_STANDART);
	mipCount   = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_STANDART);
	if(!createCubeArray(ENVPROBS_FRAME_COUNT_SQ, resolution, mipCount, format, sqProbArray, sqProbArraySRV))
		return false;

	format     = EnvProbSystem::GetFormat(EnvProbQuality::EP_LOW);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_LOW);
	mipCount   = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_LOW);
	if(!createCubeArray(ENVPROBS_FRAME_COUNT_LQ, resolution, mipCount, format, lqProbArray, lqProbArraySRV))
		return false;

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

	const uint32_t arrayWidth    = probArray->width;
	const uint32_t arrayMipCount = probArray->mipLevels;

	for(size_t i = 0; i < arraySize; i++)
	{
		EnvProbData& prob = envProbs[i];

		auto probSlot = regedProbs.find(prob.probId);
		if(probSlot->second == ENVPROBS_NEED_COPY_KEY)
		{
			const int32_t arrayId = freeProbIndex[0];
			probSlot->second = arrayId;
			freeProbIndex.erase_and_pop_back((size_t)0);

			auto* textureCubeSRV = TEXTURE_GETPTR(prob.probId);
			auto* sourceTexture  = textureCubeSRV->sourceTexture;
			if(!sourceTexture) continue;

			for(int32_t face = 0; face < 6; face++)
			{
				int32_t currentRes = arrayWidth;
				for(uint32_t mipSlice = 0; mipSlice < prob.mips; mipSlice++)
				{
					RHI::GfxBox srcBox = { 0, 0, 0, (uint32_t)currentRes, (uint32_t)currentRes, 1 };

					uint32_t destSubresource = RHI::CalcSubresource(mipSlice, arrayId * 6 + face, arrayMipCount);
					uint32_t srcSubresource  = RHI::CalcSubresource(mipSlice, face, prob.mips);

					GFX_CMD->CopyTextureRegion(probArray, destSubresource, 0, 0, 0,
						sourceTexture, srcSubresource, &srcBox);

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