#include "stdafx.h"

#include "EnvProbMgr.h"
#include "RenderMgrs.h"
#include "Render.h"
#include "Utils\Profiler.h"

using namespace EngineCore;

EnvProbMgr::EnvProbMgr(SceneRenderMgr* rndm)
{
	render_mgr = rndm;

	if(!InitBuffers())
	{
		ERR("Cant init EnvProbs buffers");
	}

	hqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_HQ);
	hqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_SQ);
	hqRegedProbsPrev.reserve(ENVPROBS_FRAME_COUNT_LQ);

	hqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_HQ);
	hqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_SQ);
	hqRegedProbs.reserve(ENVPROBS_FRAME_COUNT_LQ);
}

EnvProbMgr::~EnvProbMgr()
{
	render_mgr = nullptr;
}

bool EnvProbMgr::InitBuffers()
{
	DXGI_FORMAT format = EnvProbSystem::GetFormat(EnvProbQuality::EP_HIGH);
	int32_t resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_HIGH);
	int32_t mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_HIGH);

	D3D11_TEXTURE2D_DESC cubeArrayDesc;
	ZeroMemory(&cubeArrayDesc, sizeof(cubeArrayDesc));
	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_HQ * 6;
	cubeArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	cubeArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	cubeArrayDesc.CPUAccessFlags = 0;
	cubeArrayDesc.MiscFlags = 0;
	cubeArrayDesc.Format = format;
	if( FAILED(Render::CreateTexture2D(&cubeArrayDesc, NULL, &hqProbArray)) )
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC cubeArraySRVDesc;
	ZeroMemory(&cubeArraySRVDesc, sizeof(cubeArraySRVDesc));
	cubeArraySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.MostDetailedMip = 0;
	cubeArraySRVDesc.TextureCubeArray.First2DArrayFace = 0;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_HQ;
	cubeArraySRVDesc.Format = format;
	if( FAILED(Render::CreateShaderResourceView(hqProbArray, &cubeArraySRVDesc, &hqProbArraySRV)) )
		return false;

	format = EnvProbSystem::GetFormat(EnvProbQuality::EP_STANDART);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_STANDART);
	mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_STANDART);

	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_SQ * 6;
	cubeArrayDesc.Format = format;
	if( FAILED(Render::CreateTexture2D(&cubeArrayDesc, NULL, &sqProbArray)) )
		return false;

	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_SQ;
	cubeArraySRVDesc.Format = format;
	if( FAILED(Render::CreateShaderResourceView(sqProbArray, &cubeArraySRVDesc, &sqProbArraySRV)) )
		return false;

	format = EnvProbSystem::GetFormat(EnvProbQuality::EP_LOW);
	resolution = EnvProbSystem::GetResolution(EnvProbQuality::EP_LOW);
	mipCount = EnvProbSystem::GetMipsCount(EnvProbQuality::EP_LOW);

	cubeArrayDesc.Width = resolution;
	cubeArrayDesc.Height = resolution;
	cubeArrayDesc.MipLevels = mipCount;
	cubeArrayDesc.ArraySize = ENVPROBS_FRAME_COUNT_LQ * 6;
	cubeArrayDesc.Format = format;
	if( FAILED(Render::CreateTexture2D(&cubeArrayDesc, NULL, &lqProbArray)) )
		return false;

	cubeArraySRVDesc.TextureCubeArray.MipLevels = mipCount;
	cubeArraySRVDesc.TextureCubeArray.NumCubes = ENVPROBS_FRAME_COUNT_LQ;
	cubeArraySRVDesc.Format = format;
	if( FAILED(Render::CreateShaderResourceView(lqProbArray, &cubeArraySRVDesc, &lqProbArraySRV)) )
		return false;

	return true;
}

void EnvProbMgr::AddEnvProb(const EnvProbData& data, const Vector3& camPos)
{
	Vector3 fromCamera = camPos - data.position;
	float priorityDist = max(0.0f, fromCamera.Length() - data.distance) * data.priority;

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

bool CompareEnvProbs(EnvProbData& first, EnvProbData& second)
{
	return first.priority < second.priority;
}

void EnvProbMgr::PrepareEnvProbs()
{
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_HQ>(hqRegedProbs, hqRegedProbsPrev, hqEnvProbs, hqFreeProbIndex);
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_SQ>(sqRegedProbs, sqRegedProbsPrev, sqEnvProbs, sqFreeProbIndex);
	PrepareEnvProbsChannel<ENVPROBS_FRAME_COUNT_LQ>(lqRegedProbs, lqRegedProbsPrev, lqEnvProbs, lqFreeProbIndex);

	/*hqFreeProbIndex.resize(ENVPROBS_FRAME_COUNT_HQ);
	for(int32_t i = 0; i < ENVPROBS_FRAME_COUNT_HQ; i++)
	{
		hqFreeProbIndex[i] = i;
	}

	const size_t arraySize = min<size_t>(ENVPROBS_FRAME_COUNT_HQ, hqEnvProbs.size());
	for(size_t i = 0; i < arraySize; i++)
	{
		EnvProbData& prob = hqEnvProbs[i];

		auto hasProb = hqRegedProbsPrev.find(prob.probId);
		if(hasProb != hqRegedProbsPrev.end())
		{
			hqRegedProbs.insert(make_pair(prob.probId, hasProb->second));
			hqFreeProbIndex.erase_and_pop_back(hasProb->second);
		}
		else
		{
			hqRegedProbs.insert(make_pair(prob.probId, ENVPROBS_NEED_COPY_KEY));
		}
	}

	for(size_t i = 0; i < arraySize; i++)
	{
		EnvProbData& prob = hqEnvProbs[i];

		auto probSlot = hqRegedProbs.find(prob.probId);
		if(probSlot->second == ENVPROBS_NEED_COPY_KEY)
		{
			probSlot->second = hqFreeProbIndex[0];
			hqFreeProbIndex.erase_and_pop_back(0);

			// TODO: copy cube
		}

		// TODO: push to data array
	}

	// TODO: sort data array by priority*/
}

template<size_t FRAME_COUNT>
void EnvProbMgr::PrepareEnvProbsChannel( unordered_map<uint32_t, int32_t>& regedProbs, unordered_map<uint32_t, int32_t>& regedProbsPrev, 
										SArray<EnvProbData, FRAME_COUNT * 4>& envProbs, SArray<int32_t, FRAME_COUNT>& freeProbIndex, ID3D11Texture2D* hqProbArray )
{
	sort(envProbs.begin(), envProbs.end(), CompareEnvProbs );

	regedProbs.clear();

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
			freeProbIndex.erase_and_pop_back(hasProb->second);
		}
		else
		{
			regedProbs.insert(make_pair(prob.probId, ENVPROBS_NEED_COPY_KEY));
		}
	}

	D3D11_TEXTURE2D_DESC desc;
	probArray->GetDesc(&desc);

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
			freeProbIndex.erase_and_pop_back(0);
			
			auto textureCube = TEXTURE_GETPTR(i.probId);
			ID3D11Resource* srcRes = nullptr;
			textureCube->GetResource(srcRes);
			
			for(int32_t face = 0; face < 6; face++)
			{
				int32_t currentRes = desc.Width;
				for(int32_t mipSlice = 0; mipSlice < prob.mips; mipSlice++)
				{
					region.right = currentRes;
					region.bottom = region.right;

					CONTEXT->CopySubresourceRegion(probArray, D3D11CalcSubresource(mipSlice, arrayId * 6 + face, desc.MipLevels), 0, 0, 0, 
						srcRes, D3D11CalcSubresource(mipSlice, face, prob.mips), region);

					currentRes /= 2;
				}
			}
		}

		// TODO: push to data array
	}

	// TODO: sort data array by priority

	swap(regedProbs, regedProbsPrev);
}