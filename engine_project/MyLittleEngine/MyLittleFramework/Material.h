#pragma once
#include "Shader.h"
#include "ShaderMgr.h"
#include "DataTypes.h"
#include "MaterialData.h"
#include "Buffer.h"
#include "TexMgr.h"

#define MAT_STR_LEN 256
#define TEX_STR_LEN 512

#define MATERIALS_COUNT 1024

namespace EngineCore
{
	struct MaterialParamsStructBuffer
	{
		unsigned int unlit;

		unsigned int subscattering;
		float ss_distortion;
		float ss_direct_translucency;
		float ss_direct_pow;
		float ss_indirect_translucency;

		float padding[2];

		MaterialParamsStructBuffer()
		{
			unlit = 0;
			subscattering = 0;
			ss_distortion = 0;
			ss_direct_translucency = 0;
			ss_direct_pow = 0;
			ss_indirect_translucency = 0;
		}
	};

	struct TextureHandle
	{
		uint64_t texture;
		bool is_ptr;
		TextureHandle()
		{
			texture = TEX_NULL;
			is_ptr = false;
		}
	};

	class Material
	{
	public:
		Material(string& name);
		~Material();

		void Set(TECHNIQUES tech = TECHNIQUE_DEFAULT);

		inline RENDER_QUEUES GetTechQueue(TECHNIQUES tech = TECHNIQUE_DEFAULT, bool* hasTech = nullptr)
		{
			auto sh = (Shader*) ShaderMgr::GetShaderPtr(shaderID);
			if(!sh)
				return RENDER_QUEUES::GUI_2D;
			if(hasTech)
				*hasTech = sh->HasTechnique(tech);
			return sh->GetQueue(tech);
		}
		
		void SetMatrixBuffer(ID3D11Buffer* matrixBuf);

		void AddToFrameBuffer(MaterialParamsStructBuffer* buf, uint32_t* i);
		
		void SetTexture(ID3D11ShaderResourceView *texture, uint8_t id, uint8_t shaderType);
		string GetTextureName(uint8_t id, uint8_t shader);
		bool SetTextureByName(string& name, uint8_t id, uint8_t shader);
		inline bool SetTextureByName_lua(string name, uint8_t id, uint8_t shader)
		{return SetTextureByName(name, id, shader);}
		void ClearTextures();

		void SetVector(XMFLOAT4& vect, uint8_t id, uint8_t shaderType);
		void SetFloat(float f, uint8_t id, uint8_t shaderType);

		XMFLOAT4 GetVector(uint8_t id, uint8_t shader);
		float GetFloat(uint8_t id, uint8_t shader);

		inline void SetSR(luaSRV view, uint8_t id, uint8_t shader) {SetTexture(view.srv, id, shader);}

		void SetDefferedParam(float data, uint8_t i);
		float GetDefferedParam(uint8_t i);

		inline string GetName() {return materialName;}
		inline string GetShaderName() {return ShaderMgr::Get()->GetShaderName(shaderID);}
		
		inline bool IsError() const {return shaderID == SHADER_NULL;}

		bool Save();

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<Material>("Material")
					.addFunction("SetVector", &Material::SetVector)
					.addFunction("SetFloat", &Material::SetFloat)
					.addFunction("GetVector", &Material::GetVector)
					.addFunction("GetFloat", &Material::GetFloat)
					.addFunction("SetDefferedParam", &Material::SetDefferedParam)
					.addFunction("GetDefferedParam", &Material::GetDefferedParam)
					.addFunction("SetTextureByName", &Material::SetTextureByName_lua)
					.addFunction("SetTexture", &Material::SetSR)
					.addFunction("GetTextureName", &Material::GetTextureName)
					.addFunction("ClearTextures", &Material::ClearTextures)
					.addFunction("GetName", &Material::GetName)
					.addFunction("GetShaderName", &Material::GetShaderName)
					.addFunction("Save", &Material::Save)
				.endClass();
		}

	private:
		bool loadMat();
		bool createMat();

	#ifdef _DEV
		bool ñonvertMat(string& nameBin);
	#endif

		bool initBuffers();
		void updateBuffers();

		string materialName;
		uint16_t shaderID;

		RArray<TextureHandle> textures[5];

		RArray<XMFLOAT4> dataVector[5];
		uint8_t offsetFloat[5];

		MaterialParamsStructBuffer* defferedParams;
		
		ID3D11Buffer* inputBuf[5];
		ID3D11Buffer* idBuf;

		uint32_t scene_id;

		uint8_t vectorsReg[5];
		uint8_t sceneReg;
		uint8_t texReg[5];
		uint8_t matrixReg[5];

		bool b_dirty;
	};

	class SimpleShaderInst
	{
	public:
		SimpleShaderInst(string& shaderName);
		~SimpleShaderInst();

		void Set();

		void SetMatrixBuffer(ID3D11Buffer* matrixBuf);

		void SetTexture(ID3D11ShaderResourceView *texture, uint8_t id);
		string GetTextureName(uint8_t id);
		bool SetTextureByName(string& name, uint8_t id);
		inline bool SetTextureByName_lua(string name, uint8_t id){return SetTextureByName(name, id);}
		void ClearTextures();

		void SetVector(XMFLOAT4& vect, uint8_t id);
		void SetFloat(float f, uint8_t id);

		XMFLOAT4 GetVector(uint8_t id);
		float GetFloat(uint8_t id);

		inline void SetSR(luaSRV view, uint8_t id) {SetTexture(view.srv, id);}
		
		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<SimpleShaderInst>("SimpleShaderInst")
					.addFunction("SetVector", &SimpleShaderInst::SetVector)
					.addFunction("SetFloat", &SimpleShaderInst::SetFloat)
					.addFunction("GetVector", &SimpleShaderInst::GetVector)
					.addFunction("GetFloat", &SimpleShaderInst::GetFloat)
					.addFunction("SetTextureByName", &SimpleShaderInst::SetTextureByName_lua)
					.addFunction("SetTexture", &SimpleShaderInst::SetSR)
					.addFunction("GetTextureName", &SimpleShaderInst::GetTextureName)
					.addFunction("ClearTextures", &SimpleShaderInst::ClearTextures)
				.endClass();
		}

	private:
		bool initInst(string& shaderName);
		bool initBuffers();
		void updateBuffers();

		uint16_t shaderID;

		RArray<TextureHandle> textures;

		RArray<XMFLOAT4> dataVector;
		uint8_t offsetFloat;
		
		ID3D11Buffer* inputBuf;
		
		uint8_t vectorsReg;
		uint8_t texReg;
		uint8_t matrixReg;

		bool b_dirty;
	};
}