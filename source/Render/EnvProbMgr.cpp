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
	targetData->priority = priorityDist;
}

bool CompareEnvProbs(EnvProbData& first, EnvProbData& second)
{
	return first.priority < second.priority;
}

void EnvProbMgr::PrepareEnvProbs()
{
	sort(hqEnvProbs.begin(), hqEnvProbs.end(), CompareEnvProbs );
	sort(sqEnvProbs.begin(), sqEnvProbs.end(), CompareEnvProbs );
	sort(lqEnvProbs.begin(), lqEnvProbs.end(), CompareEnvProbs );

	hqRegedProbs.clear();
	sqRegedProbs.clear();
	lqRegedProbs.clear();

	for(size_t i = 0; i < min<size_t>(ENVPROBS_FRAME_COUNT_HQ, hqEnvProbs.size()); i++)
	{
		EnvProbData& prob = hqEnvProbs[i];

		auto hasProb = hqRegedProbsPrev.find(prob.probId);
		if(hasProb != hqRegedProbsPrev.end())
		{
			hqRegedProbs.insert(make_pair(prob.probId, hasProb->second));
		}
		else
		{
			hqRegedProbs.insert(make_pair(prob.probId, ENVPROBS_NEED_COPY_KEY));
		}
		// TODO: push prob render data
	}
}