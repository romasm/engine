#pragma once
#include "Shader.h"
#include "ShaderMgr.h"
#include "DataTypes.h"
#include "MaterialData.h"
#include "Buffer.h"
#include "TexMgr.h"

#define COMMON_MATERIAL_SHADER_01 PATH_SHADERS"objects/opaque_main"
#define COMMON_MATERIAL_SHADER_02 PATH_SHADERS"objects/alphatest_main"
#define COMMON_MATERIAL_SHADER_03 PATH_SHADERS"objects/transparent_medium"

#define MAT_STR_LEN 256
#define TEX_STR_LEN 512

#define MATERIALS_COUNT 1024

#define DEFFERED_UNLIT_0 "unlit"
#define DEFFERED_IOR "ior"
#define DEFFERED_ASYMMETRY "asymmetry"
#define DEFFERED_ATTENUATION "attenuation"

namespace EngineCore
{
	struct MaterialParamsStructBuffer
	{
		unsigned int unlit;

		float ior;
		float asymmetry;
		float attenuation;

		float padding[4];

		MaterialParamsStructBuffer()
		{
			unlit = 0;
			ior = 0;
			asymmetry = 0;
			attenuation = 0;
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

		inline bool HasTechnique(TECHNIQUES tech)
		{
			auto sh = (Shader*) ShaderMgr::GetShaderPtr(shaderID);
			return sh->HasTechnique(tech);
		}

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
		void SetTextureWithSlotName(ID3D11ShaderResourceView *texture, string& slot, uint8_t shaderType);

		string GetTextureName(uint8_t id, uint8_t shader);
		string GetTextureNameWithSlotName(string slot, uint8_t shaderType);

		bool SetTextureByName(string name, uint8_t id, uint8_t shader);
		bool SetTextureByNameWithSlotName(string name, string slot, uint8_t shaderType);

		void ClearTextures();

		void SetVector(XMFLOAT4& vect, uint8_t id, uint8_t shaderType);
		void SetFloat(float f, uint8_t id, uint8_t shaderType);
		void SetVectorWithSlotName(XMFLOAT4& vect, string slot, uint8_t shaderType);
		void SetFloatWithSlotName(float f, string slot, uint8_t shaderType);

		XMFLOAT4 GetVector(uint8_t id, uint8_t shader);
		float GetFloat(uint8_t id, uint8_t shader);
		XMFLOAT4 GetVectorWithSlotName(string slot, uint8_t shaderType);
		float GetFloatWithSlotName(string slot, uint8_t shaderType);

		inline void SetSR(luaSRV view, uint8_t id, uint8_t shader) {SetTexture(view.srv, id, shader);}
		inline void SetSRWithSlotName(luaSRV view, string slot, uint8_t shader) {SetTextureWithSlotName(view.srv, slot, shader);}

		void SetDefferedParam(float data, uint8_t i);
		void SetDefferedParamWithSlotName(float data, string slot);
		float GetDefferedParam(uint8_t i);
		float GetDefferedParamWithSlotName(string slot);

		bool SetShader(string shaderName);

		inline string GetName() {return materialName;}
		inline string GetShaderName() {return ShaderMgr::Get()->GetShaderName(shaderID);}
		
		inline bool IsError() const {return shaderID == SHADER_NULL;}

		bool Save();

		bool IsTransparent();

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<Material>("Material")
					.addFunction("SetVectorByID", &Material::SetVector)
					.addFunction("SetVector", &Material::SetVectorWithSlotName)
					.addFunction("SetFloatByID", &Material::SetFloat)
					.addFunction("SetFloat", &Material::SetFloatWithSlotName)
					.addFunction("GetVectorByID", &Material::GetVector)
					.addFunction("GetVector", &Material::GetVectorWithSlotName)
					.addFunction("GetFloatByID", &Material::GetFloat)
					.addFunction("GetFloat", &Material::GetFloatWithSlotName)

					.addFunction("SetDefferedParamByID", &Material::SetDefferedParam)
					.addFunction("SetDefferedParam", &Material::SetDefferedParamWithSlotName)
					.addFunction("GetDefferedParamByID", &Material::GetDefferedParam)
					.addFunction("GetDefferedParam", &Material::GetDefferedParamWithSlotName)

					.addFunction("SetTextureNameByID", &Material::SetTextureByName)
					.addFunction("SetTextureName", &Material::SetTextureByNameWithSlotName)
					.addFunction("SetShaderResourceByID", &Material::SetSR)
					.addFunction("SetShaderResource", &Material::SetSRWithSlotName)
					.addFunction("GetTextureNameByID", &Material::GetTextureName)
					.addFunction("GetTextureName", &Material::GetTextureNameWithSlotName)

					.addFunction("ClearTextures", &Material::ClearTextures)
					.addFunction("GetName", &Material::GetName)
					.addFunction("GetShaderName", &Material::GetShaderName)
					.addFunction("SetShader", &Material::SetShader)
					.addFunction("Save", &Material::Save)

					.addFunction("IsTransparent", &Material::IsTransparent)
				.endClass();
		}

	private:
		bool loadMat();
		bool createMat();

	#ifdef _DEV
		bool ñonvertMat(string& nameBin);
	#endif

		bool initBuffers();
		void closeBuffers();
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
		void SetTextureWithSlotName(ID3D11ShaderResourceView *texture, string& slot);
		
		string GetTextureName(uint8_t id);
		string GetTextureNameWithSlotName(string slot);

		bool SetTextureByName(string name, uint8_t id);
		bool SetTextureByNameWithSlotName(string name, string slot);
		void ClearTextures();

		void SetVector(XMFLOAT4& vect, uint8_t id);
		void SetVectorWithSlotName(XMFLOAT4& vect, string slot);
		void SetFloat(float f, uint8_t id);
		void SetFloatWithSlotName(float f, string slot);

		XMFLOAT4 GetVector(uint8_t id);
		XMFLOAT4 GetVectorWithSlotName(string slot);
		float GetFloat(uint8_t id);
		float GetFloatWithSlotName(string slot);

		inline void SetSR(luaSRV view, uint8_t id) {SetTexture(view.srv, id);}
		inline void SetSRWithSlotName(luaSRV view, string slot) {SetTextureWithSlotName(view.srv, slot);}
		
		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<SimpleShaderInst>("SimpleShaderInst")
					.addFunction("SetVectorByID", &SimpleShaderInst::SetVector)
					.addFunction("SetVector", &SimpleShaderInst::SetVectorWithSlotName)
					.addFunction("SetFloatByID", &SimpleShaderInst::SetFloat)
					.addFunction("SetFloat", &SimpleShaderInst::SetFloatWithSlotName)
					.addFunction("GetVectorByID", &SimpleShaderInst::GetVector)
					.addFunction("GetVector", &SimpleShaderInst::GetVectorWithSlotName)
					.addFunction("GetFloatByID", &SimpleShaderInst::GetFloat)
					.addFunction("GetFloat", &SimpleShaderInst::GetFloatWithSlotName)
					.addFunction("SetTextureNameByID", &SimpleShaderInst::SetTextureByName)
					.addFunction("SetTextureName", &SimpleShaderInst::SetTextureByNameWithSlotName)
					.addFunction("SetShaderResourceByID", &SimpleShaderInst::SetSR)
					.addFunction("SetShaderResource", &SimpleShaderInst::SetSRWithSlotName)
					.addFunction("GetTextureNameByID", &SimpleShaderInst::GetTextureName)
					.addFunction("GetTextureName", &SimpleShaderInst::GetTextureNameWithSlotName)
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