#pragma once
#include "ShaderCodeMgr.h"
#include "RenderState.h"

#define SHADERCODE_STR_LEN 256

#define TECHNIQUE_SIZE (sizeof(uint8_t) + SHADERCODE_STR_LEN * 5 + sizeof(uint8_t) + sizeof(D3D11_DEPTH_STENCIL_DESC) \
	+ sizeof(D3D11_BLEND_DESC) + sizeof(D3D11_RASTERIZER_DESC))

#define TECHIQUE_STR_L L"TECHNIQUE_"
#define TECHIQUE_STR "TECHNIQUE_"
#define TECHIQUE_STR_SIZE 10

namespace EngineCore
{
	struct tech_desc
	{
		uint8_t tech_id;
		uint8_t queue;

		string pixelShader;
		string vertexShader;
		string hullShader;
		string domainShader;
		string geometryShader;

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
		D3D11_BLEND_DESC blendDesc;
		D3D11_RASTERIZER_DESC rastDesc;
	};

	class BaseShader
	{
	public:
		BaseShader(string& name);
		virtual ~BaseShader(){}
		
	#ifdef _DEV
		inline uint32_t GetSrcDate() {return filedate;}
		inline void SetSrcDate(uint32_t date) {filedate = date;}
		inline bool IsSimple(){return is_simple;}
	#endif
		
		inline bool IsError() const {return shaderName.empty();}

	protected:
	#ifdef _DEV
		bool CompileTechniques(string& file, string& binFile, DArray<tech_desc>& techsDesc);
		
		bool is_simple;
	#endif

		string shaderName;
		uint32_t filedate;
	};

	class Shader : protected BaseShader
	{
	public:
		Shader(string& name);
		virtual ~Shader();

		void Set(TECHNIQUES tech = TECHNIQUE_DEFAULT);

		inline RENDER_QUEUES GetQueue(TECHNIQUES tech = TECHNIQUE_DEFAULT)
		{return (RENDER_QUEUES)techs_array[tech].queue;}

		inline bool HasTechnique(TECHNIQUES tech)
		{return techs_array[tech].depthState != STATE_NULL;}

		uint16_t* GetCode(TECHNIQUES tech = TECHNIQUE_DEFAULT)
		{
			auto& tq = techs_array[tech];
			if(tq.depthState == STATE_NULL)
				return nullptr;
			return tq.shadersID;
		}

		inline int16_t GetTextureIdBySlot(string& slot, uint8_t shaderType)
		{
			auto& tq = techs_array[TECHNIQUE_DEFAULT];
			auto& code = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[shaderType]);
			auto it = code.input.matTextureMap.find(slot);
			if( it == code.input.matTextureMap.end() )
				return -1;
			return (int16_t)it->second;
		}

		inline int16_t GetFloatIdBySlot(string& slot, uint8_t shaderType)
		{
			auto& tq = techs_array[TECHNIQUE_DEFAULT];
			auto& code = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[shaderType]);
			auto it = code.input.matFloatMap.find(slot);
			if( it == code.input.matFloatMap.end() )
				return -1;
			return (int16_t)it->second;
		}

		inline int16_t GetVectorIdBySlot(string& slot, uint8_t shaderType)
		{
			auto& tq = techs_array[TECHNIQUE_DEFAULT];
			auto& code = ShaderCodeMgr::GetShaderCodeRef(tq.shadersID[shaderType]);
			auto it = code.input.matVectorMap.find(slot);
			if( it == code.input.matVectorMap.end() )
				return -1;
			return (int16_t)it->second;
		}

	private:
		bool initShader();
		
		struct technique_data
		{
			uint16_t shadersID[5];
			uint8_t queue;

			uint16_t depthState;
			uint16_t blendState;
			uint16_t rastState;

			technique_data()
			{
				depthState = STATE_NULL;
				blendState = STATE_NULL;
				rastState = STATE_NULL;
				for(int i=0; i<5; i++)
					shadersID[i] = SHADER_NULL;
				queue = RENDER_QUEUES::SC_OPAQUE;
			}
		};

		SArray<technique_data, TECHNIQUES_COUNT> techs_array;
	};

	class SimpleShader : protected BaseShader
	{
	public:
		SimpleShader(string& name);
		virtual ~SimpleShader();

		void Set();

		inline RENDER_QUEUES GetQueue()
		{return (RENDER_QUEUES)data.queue;}
		
		uint16_t* GetCode()
		{
			if(data.depthState == STATE_NULL)
				return nullptr;
			return data.shadersID;
		}
		
		inline int16_t GetTextureIdBySlot(string& slot)
		{
			auto& code = ShaderCodeMgr::GetShaderCodeRef(data.shadersID[SHADER_PS]);
			auto it = code.input.matTextureMap.find(slot);
			if( it == code.input.matTextureMap.end() )
				return -1;
			return (int16_t)it->second;
		}

		inline int16_t GetFloatIdBySlot(string& slot)
		{
			auto& code = ShaderCodeMgr::GetShaderCodeRef(data.shadersID[SHADER_PS]);
			auto it = code.input.matFloatMap.find(slot);
			if( it == code.input.matFloatMap.end() )
				return -1;
			return (int16_t)it->second;
		}

		inline int16_t GetVectorIdBySlot(string& slot)
		{
			auto& code = ShaderCodeMgr::GetShaderCodeRef(data.shadersID[SHADER_PS]);
			auto it = code.input.matVectorMap.find(slot);
			if( it == code.input.matVectorMap.end() )
				return -1;
			return (int16_t)it->second;
		}

	private:
		bool initShader();
		
		struct technique_data
		{
			uint16_t shadersID[2];
			uint8_t queue;

			uint16_t depthState;
			uint16_t blendState;
			uint16_t rastState;

			technique_data()
			{
				depthState = STATE_NULL;
				blendState = STATE_NULL;
				rastState = STATE_NULL;
				for(int i=0; i<2; i++)
					shadersID[i] = SHADER_NULL;
				queue = RENDER_QUEUES::SC_OPAQUE;
			}
		};

		technique_data data;
	};
}