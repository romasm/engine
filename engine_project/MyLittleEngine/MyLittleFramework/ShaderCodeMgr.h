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

namespace EngineCore
{
#define REGISTER_NULL 255

	struct CodeInput
	{
		uint8_t matInfo_Register;
		uint8_t matInfo_FloatCount;
		uint8_t matInfo_VectorCount;

		uint8_t matTextures_StartRegister;
		uint8_t matTextures_Count;

		uint8_t matId_Register;

		uint8_t matrixBuf_Register;

		DArray<sampler_ptr> samplers;

		ID3D11InputLayout* layout;

		CodeInput()
		{
			layout = nullptr;
			matInfo_Register = REGISTER_NULL;
			matInfo_FloatCount = 0;
			matInfo_VectorCount = 0;
			matId_Register = REGISTER_NULL;
			matrixBuf_Register = REGISTER_NULL;
			matTextures_StartRegister = REGISTER_NULL;
			matTextures_Count = 0;
		}
	};

	struct CodeHandle
	{
		void* code;
		uint32_t refcount;
		uint32_t filedate;

		string name;

		CodeInput input;

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

		uint16_t GetShaderCode(string& name, uint8_t type);
		void DeleteShaderCode(uint16_t id, uint8_t type);

		inline static CodeHandle& GetShaderCodeRef(uint16_t id) 
		{
			if(id == SHADER_NULL) return instance->null_shader;
			return instance->shader_array[id];
		}

	#ifdef _DEV
		void UpdateShadersCode();
	#endif

	private:

	#ifdef _DEV
		ID3DBlob* CompileShader(string& file, string& binFile, string& entryPoint, uint8_t type, uint32_t* date = nullptr);
	#endif

		uint16_t AddShaderToList(string& name, uint8_t type);
		uint16_t FindShaderInList(string& name, uint8_t type);

		inline DXGI_FORMAT GetInputFormat(D3D_REGISTER_COMPONENT_TYPE component, BYTE mask);
		
		bool GetInputData(CodeInput& HInput, uint8_t* data, uint32_t size, uint8_t type);
		ID3D11InputLayout* GetVertexLayout(uint8_t* data, uint32_t size);

		static ShaderCodeMgr *instance;

		unordered_map<string, uint16_t> shader_map[6];
		unordered_map<string, ID3D11InputLayout*> layout_map;

		SArray<CodeHandle, SHADER_MAX_COUNT> shader_array;
		SDeque<uint16_t, SHADER_MAX_COUNT> shader_free;

		CodeHandle null_shader;
	};
}