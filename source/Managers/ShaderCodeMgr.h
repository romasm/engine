#pragma once
#include "stdafx.h"
#include "Common.h"
#include "RenderState.h"

#define SHADERCODE(name, type) ShaderCodeMgr::Get()->GetShaderCode(name, type)
#define SHADERCODE_DROP(name, type) ShaderCodeMgr::Get()->DeleteShaderCode(name, type)
#define GET_SHADER(id) ShaderCodeMgr::Get()->GetShaderCodeRef(id)

#define SHADER_MAX_COUNT 256
#define SHADER_INIT_COUNT 64
#define LAYOUT_INIT_COUNT 8
#define SHADER_NULL SHADER_MAX_COUNT

#define SHADER_COMPILE_TARGET "5_0"

#define SHADER_NAME_DEL '#'
#define SHADER_DEFINE_BEG '['
#define SHADER_DEFINE_END ']'

namespace EngineCore::RHI { struct GfxSampler; struct GfxInputLayout; }

namespace EngineCore
{
#define REGISTER_NULL 255
	
	struct ShaderInput
	{
		unordered_map<string, uint8_t> constantBuffers;
		unordered_map<string, uint8_t> resourceBuffers;
		unordered_map<string, uint8_t> rwBuffers;

		DArray<RHI::GfxSampler*> samplers;

		// material data
		uint8_t matInfo_Register;
		uint8_t matInfo_FloatCount;
		unordered_map<string, uint8_t> matFloatMap;

		uint8_t matInfo_VectorCount;
		unordered_map<string, uint8_t> matVectorMap;

		uint8_t matTextures_StartRegister;
		uint8_t matTextures_Count;
		unordered_map<string, uint8_t> matTextureMap;

		uint8_t matId_Register;

		// mesh data
		uint8_t matrixBuf_Register;
		uint8_t matrixBoneBuf_Register;

		RHI::GfxInputLayout* layout;
		
		void Reset()
		{
			constantBuffers.clear();
			resourceBuffers.clear();
			rwBuffers.clear();
			samplers.destroy();

			matInfo_Register = REGISTER_NULL;
			matInfo_FloatCount = 0;
			matInfo_VectorCount = 0;
			matId_Register = REGISTER_NULL;
			matrixBuf_Register = REGISTER_NULL;
			matrixBoneBuf_Register = REGISTER_NULL;
			matTextures_StartRegister = REGISTER_NULL;
			matTextures_Count = 0;

			layout = nullptr;
		}

		ShaderInput()
		{
			Reset();
		}
	};

	struct CodeHandle
	{
		void* code;
		uint32_t refcount;
		uint32_t filedate;

		string name;

		ShaderInput input;

		// Raw DXBC/DXIL bytecode kept alive for DX12 PSO creation.
		// Populated by AddShaderToList / CheckForReload alongside the DX11
		// shader object stored in `code`.
		std::vector<uint8_t> bytecode;

		// Returns true if the shader was loaded (DX11: code ptr; DX12: bytecode)
		bool IsValid() const { return code != nullptr || !bytecode.empty(); }

		CodeHandle()
		{
			code = nullptr;
			refcount = 0;
			filedate = 0;
		}
	};

	class ShaderCodeMgr
	{
	public:
		ShaderCodeMgr();
		~ShaderCodeMgr();
	
		inline static ShaderCodeMgr* Get(){return instance;}

		uint16_t GetShaderCode(const string& name, uint8_t type);
		void DeleteShaderCode(uint16_t id, uint8_t type);

		inline static CodeHandle& GetShaderCodeRef(uint16_t id)
		{
			if(id == SHADER_NULL) return instance->null_shader;
			return instance->shader_array[id];
		}

		// Returns the raw shader bytecode (DXBC or DXIL) for DX12 PSO creation.
		// Returns an empty vector for the null shader or an unloaded slot.
		inline static const std::vector<uint8_t>& GetShaderBytecode(uint16_t id)
		{
			static const std::vector<uint8_t> empty;
			if(id == SHADER_NULL || !instance) return empty;
			return instance->shader_array[id].bytecode;
		}

	#ifdef _DEV
		void CheckForReload();
	#endif

	private:

	#ifdef _DEV
		ID3DBlob* CompileShader(string& file, string& binFile, string& entryPoint, string& defines, uint8_t type, uint32_t* date = nullptr);
	#endif

		bool SpreadShaderName(const string& shaderName, string& outFile, string& outEntry, string& outDefines);
		D3D_SHADER_MACRO* ConstructDefinesArray(const string& defines, uint32_t& outCount);
		void DeleteDefinesArray(D3D_SHADER_MACRO* definesArray, uint32_t count);

		uint16_t AddShaderToList(const string& name, uint8_t type);
		uint16_t FindShaderInList(const string& name, uint8_t type);

		inline DXGI_FORMAT GetInputFormat(D3D_REGISTER_COMPONENT_TYPE component, BYTE mask);
		
		bool GetInputData(ShaderInput& HInput, uint8_t* data, uint32_t size, uint8_t type);
		RHI::GfxInputLayout* GetVertexLayout(uint8_t* data, uint32_t size);

		static ShaderCodeMgr *instance;

		unordered_map<string, uint16_t> shader_map[6];
		unordered_map<string, RHI::GfxInputLayout*> layout_map;

		SArray<CodeHandle, SHADER_MAX_COUNT> shader_array;
		SDeque<uint16_t, SHADER_MAX_COUNT> shader_free;

		CodeHandle null_shader;
	};
}