#pragma once
#include "Shader.h"
#include "ShaderMgr.h"
#include "Buffer.h"
#include "TexMgr.h"
#include "Material.h"
#include "Render.h"

namespace EngineCore
{
	class Compute
	{
	public:
		Compute(const string& shader) {	Init(shader); }
		Compute(const char* shader) { Init(string(shader)); }

		~Compute()
		{
			if(computePSO)
				GFX_DEVICE->DestroyPSO(computePSO);
			computePSO = nullptr;

			SHADERCODE_DROP(shaderID, SHADER_CS);
			shaderID = SHADER_NULL;

			rwAttachments.assign(nullptr);
			resAttachments.assign(nullptr);
			cbAttachments.assign(nullptr);
		}

		void Dispatch(uint32_t x, uint32_t y, uint32_t z)
		{
			// SetPipelineState must come first: on DX12 it sets the root signature,
			// which invalidates all prior root parameter bindings (CBVs especially).
			GFX_CMD->SetPipelineState(computePSO);

			if(!resAttachments.empty())
				GFX_CMD->SetCSResources(0, (uint32_t)resAttachments.size(), resAttachments.data());
			if (!cbAttachments.empty())
				GFX_CMD->SetCSConstantBuffers(0, (uint32_t)cbAttachments.size(), cbAttachments.data());
			if (!rwAttachments.empty())
				GFX_CMD->SetCSUnorderedAccessViews(0, (uint32_t)rwAttachments.size(), rwAttachments.data());

			GFX_CMD->Dispatch(x, y, z);

			// Unbind compute shader
			GFX_CMD->SetPipelineState(nullptr);

			static RHI::GfxUAV* s_nullUAVs[32] = {};
			static RHI::GfxSRV* s_nullSRVs[128] = {};
			static RHI::GfxBuffer* s_nullCBs[16] = {};
			GFX_CMD->SetCSUnorderedAccessViews(0, (uint32_t)rwAttachments.size(), s_nullUAVs);
			GFX_CMD->SetCSConstantBuffers(0, (uint32_t)cbAttachments.size(), s_nullCBs);
			GFX_CMD->SetCSResources(0, (uint32_t)resAttachments.size(), s_nullSRVs);
		}

		// ----------------------------- rw buffers
		uint8_t AttachRWResource(uint8_t slotId, RHI::GfxUAV* rw)
		{
#ifdef _DEV
			if ((size_t)slotId >= rwAttachments.size())
			{
				ERR("Wrong rw buffer attachment id %u", (uint32_t)slotId);
				return REGISTER_NULL;
			}
#endif

			rwAttachments[slotId] = rw;
			return slotId;
		}

		uint8_t AttachRWResource(const string& slotName, RHI::GfxUAV* rw)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.rwBuffers.find(slotName);
			if (it == handle.input.rwBuffers.end())
			{
				ERR("Wrong rw buffer attachment name %s", slotName.c_str());
				return REGISTER_NULL;
			}

			rwAttachments[it->second] = rw;
			return it->second;
		}
		inline uint8_t AttachRWResource(char* slotName, RHI::GfxUAV* rw) { return AttachRWResource(string(slotName), rw); }

		// if no slotId -> detach all resources
		void DetachRWResource(uint8_t slotId = REGISTER_NULL)
		{
			if (slotId == REGISTER_NULL)
			{
				rwAttachments.assign(nullptr);
				return;
			}

			if((size_t)slotId < rwAttachments.size())
				rwAttachments[slotId] = nullptr;
		}

		void DetachRWResource(const string& slotName)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.rwBuffers.find(slotName);
			if (it == handle.input.rwBuffers.end())
			{
				ERR("Wrong rw buffer detachment name %s", slotName.c_str());
				return;
			}

			rwAttachments[it->second] = nullptr;
		}
		inline void DetachRWResource(char* slotName) { DetachRWResource(string(slotName)); }

		// ----------------------------- constant buffers
		uint8_t AttachConstantBuffer(uint8_t slotId, RHI::GfxBuffer* cb)
		{
#ifdef _DEV
			if ((size_t)slotId >= cbAttachments.size())
			{
				ERR("Wrong constant buffer attachment id %u", (uint32_t)slotId);
				return REGISTER_NULL;
			}
#endif

			cbAttachments[slotId] = cb;
			return slotId;
		}

		uint8_t AttachConstantBuffer(const string& slotName, RHI::GfxBuffer* cb)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.constantBuffers.find(slotName);
			if (it == handle.input.constantBuffers.end())
			{
				ERR("Wrong constant buffer attachment name %s", slotName.c_str());
				return REGISTER_NULL;
			}

			cbAttachments[it->second] = cb;
			return it->second;
		}
		inline uint8_t AttachConstantBuffer(char* slotName, RHI::GfxBuffer* cb) { return AttachConstantBuffer(string(slotName), cb); }

		// if no slotId -> detach all resources
		void DetachConstantBuffer(uint8_t slotId = REGISTER_NULL)
		{
			if (slotId == REGISTER_NULL)
			{
				cbAttachments.assign(nullptr);
				return;
			}

			if ((size_t)slotId < cbAttachments.size())
				cbAttachments[slotId] = nullptr;
		}

		void DetachConstantBuffer(const string& slotName)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.constantBuffers.find(slotName);
			if (it == handle.input.constantBuffers.end())
			{
				ERR("Wrong constant buffer detachment name %s", slotName.c_str());
				return;
			}

			cbAttachments[it->second] = nullptr;
		}
		inline void DetachConstantBuffer(char* slotName) { DetachConstantBuffer(string(slotName)); }

		// ----------------------------- shader resources
		uint8_t AttachResource(uint8_t slotId, RHI::GfxSRV* res)
		{
#ifdef _DEV
			if ((size_t)slotId >= resAttachments.size())
			{
				ERR("Wrong resource buffer attachment id %u", (uint32_t)slotId);
				return REGISTER_NULL;
			}
#endif

			resAttachments[slotId] = res;
			return slotId;
		}

		uint8_t AttachResource(const string& slotName, RHI::GfxSRV* res)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.resourceBuffers.find(slotName);
			if (it == handle.input.resourceBuffers.end())
			{
				ERR("Wrong resource buffer attachment name %s", slotName.c_str());
				return REGISTER_NULL;
			}

			resAttachments[it->second] = res;
			return it->second;
		}
		inline uint8_t AttachResource(char* slotName, RHI::GfxSRV* res) { return AttachResource(string(slotName), res); }

		// if no slotId -> detach all resources
		void DetachResource(uint8_t slotId = REGISTER_NULL)
		{
			if (slotId == REGISTER_NULL)
			{
				resAttachments.assign(nullptr);
				return;
			}

			if ((size_t)slotId < resAttachments.size())
				resAttachments[slotId] = nullptr;
		}

		void DetachResource(const string& slotName)
		{
			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			auto it = handle.input.resourceBuffers.find(slotName);
			if (it == handle.input.resourceBuffers.end())
			{
				ERR("Wrong resource buffer detachment name %s", slotName.c_str());
				return;
			}

			resAttachments[it->second] = nullptr;
		}
		inline void DetachResource(char* slotName) { DetachResource(string(slotName)); }

		// -----------------------------
		static void Preload(string& shader)
		{
			auto res = ShaderCodeMgr::Get()->GetShaderCode(shader, SHADER_CS);
			if (res != SHADER_NULL)
				LOG("Compute shader %s preloaded", shader.c_str());
		}

	private:
		void Init(const string& shader)
		{
			shaderID = ShaderCodeMgr::Get()->GetShaderCode(shader, SHADER_CS);
			if(shaderID == SHADER_NULL)
				ERR("Cant init compute shader %s !", shader.c_str());

			// Create compute PSO
			RHI::ComputePSODesc psoDesc = {};
			psoDesc.computeShaderID = shaderID;
			computePSO = GFX_DEVICE->CreateComputePSO(psoDesc);

			auto& handle = ShaderCodeMgr::Get()->GetShaderCodeRef(shaderID);

			cbAttachments.clear();
			cbAttachments.resize(handle.input.constantBuffers.size());
			cbAttachments.assign(nullptr);

			resAttachments.clear();
			resAttachments.resize(handle.input.resourceBuffers.size());
			resAttachments.assign(nullptr);

			rwAttachments.clear();
			rwAttachments.resize(handle.input.rwBuffers.size());
			rwAttachments.assign(nullptr);
		}

		uint16_t shaderID;
		RHI::GfxPipelineState* computePSO = nullptr;

		DArray<RHI::GfxBuffer*> cbAttachments;
		DArray<RHI::GfxSRV*> resAttachments;
		DArray<RHI::GfxUAV*> rwAttachments;
	};
}