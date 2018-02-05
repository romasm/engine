#pragma once

#include "Common.h"
#include "LightBuffers.h"
#include "ECS\Entity.h"
#include "ECS\EnvProbSystem.h"

#define ENVPROBS_FRAME_COUNT_HQ 8
#define ENVPROBS_FRAME_COUNT_SQ 32
#define ENVPROBS_FRAME_COUNT_LQ 64

#define ENVPROBS_HQ_RES 256
#define ENVPROBS_NORM_RES 128
#define ENVPROBS_LOW_RES 32

#define ENVPROBS_NEED_COPY_KEY -1

namespace EngineCore
{
	class SceneRenderMgr;

	class EnvProbMgr
	{
	public:
		EnvProbMgr(SceneRenderMgr* rndm);
		~EnvProbMgr();
		
		void AddEnvProb(const EnvProbData& data, const Vector3& camPos);

		void PrepareEnvProbs();

	private:

		bool InitBuffers();

		template<size_t FRAME_COUNT>
		void PrepareEnvProbsChannel( unordered_map<uint32_t, int32_t>& regedProbs, unordered_map<uint32_t, int32_t>& regedProbsPrev, 
			SArray<EnvProbData, FRAME_COUNT * 4>& envProbs, SArray<int32_t, FRAME_COUNT>& freeProbIndex, ID3D11Texture2D* probArray );

		SceneRenderMgr* render_mgr;		

		SArray<EnvProbData, ENVPROBS_FRAME_COUNT_HQ * 4> hqEnvProbs;
		SArray<EnvProbData, ENVPROBS_FRAME_COUNT_SQ * 4> sqEnvProbs;
		SArray<EnvProbData, ENVPROBS_FRAME_COUNT_LQ * 4> lqEnvProbs;
		
		ID3D11Texture2D* hqProbArray;
		ID3D11ShaderResourceView* hqProbArraySRV;

		ID3D11Texture2D* sqProbArray;
		ID3D11ShaderResourceView* sqProbArraySRV;

		ID3D11Texture2D* lqProbArray;
		ID3D11ShaderResourceView* lqProbArraySRV;

		unordered_map<uint32_t, int32_t> hqRegedProbsPrev;
		unordered_map<uint32_t, int32_t> sqRegedProbsPrev;
		unordered_map<uint32_t, int32_t> lqRegedProbsPrev;

		unordered_map<uint32_t, int32_t> hqRegedProbs;
		unordered_map<uint32_t, int32_t> sqRegedProbs;
		unordered_map<uint32_t, int32_t> lqRegedProbs;

		SArray<int32_t, ENVPROBS_FRAME_COUNT_HQ> hqFreeProbIndex;
		SArray<int32_t, ENVPROBS_FRAME_COUNT_SQ> sqFreeProbIndex;
		SArray<int32_t, ENVPROBS_FRAME_COUNT_LQ> lqFreeProbIndex;
	};

}