#pragma once
#include "stdafx.h"
#include "Shader.h"

namespace EngineCore
{
	class ShaderMgr
	{
	public:
		ShaderMgr();
		~ShaderMgr();
		
		inline static ShaderMgr* Get(){return instance;}

		uint16_t GetShader(string& name, bool simple);
		void DeleteShader(uint16_t id);
		
		void PreloadShaders();

		inline static BaseShader* GetShaderPtr(uint16_t id)
		{
			if(id == SHADER_NULL)
				return nullptr;
			return instance->shader_array[id].shader;
		}
		inline static string& GetShaderName(uint16_t id)
		{
			if(id == SHADER_NULL)
				return null_name;
			return instance->shader_array[id].name;
		}
		
	#ifdef _DEV
		void UpdateShaders();
	#endif

	private:
		uint16_t AddShaderToList(string& name, bool simple);
		uint16_t FindShaderInList(string& name);

		struct ShaderHandle
		{
			BaseShader* shader;
			uint32_t refcount;
			string name;

			ShaderHandle()
			{
				shader = nullptr;
				refcount = 0;
			}
		};

		static ShaderMgr *instance;

		static string null_name;

		unordered_map<string, uint16_t> shader_map;

		SArray<ShaderHandle, SHADER_MAX_COUNT> shader_array;
		SDeque<uint16_t, SHADER_MAX_COUNT> shader_free;
	};
}