#include "stdafx.h"
#include "Material.h"
#include "ShaderMgr.h"
#include "TexMgr.h"
#include "World.h"
#include "ScenePipeline.h"
#include "Hud.h"

using namespace EngineCore;

Material::Material(string& name)
{
	materialName = name;
	defferedParams = nullptr;
	for(uint8_t i = 0; i < 5; i++)
	{
		inputBuf[i] = nullptr;
		offsetFloat[i] = 0;
		vectorsReg[i] = REGISTER_NULL;
		matrixReg[i] = REGISTER_NULL;
		texReg[i] = REGISTER_NULL;
	}
	idBuf = nullptr;
	shaderID = SHADER_NULL;
	sceneReg = REGISTER_NULL;
	scene_id = 0;
	
	if(materialName[0] == '$')
	{
		if(!createMat())
		{
			ERR("Cant create material from shader %s", materialName.c_str() + 1);
			return;
		}
	}
	else
	{
		if(!loadMat())
		{
			ERR("Cant load material %s", materialName.c_str());
			return;
		}
	}

	if(!initBuffers())
	{
		ERR("Cant initialize buffers for material %s", materialName.c_str());
		return;
	}

	b_dirty = true;
}

Material::~Material()
{
	ClearTextures();

	_DELETE(defferedParams);

	for(uint8_t i = 0; i < 5; i++)
	{
		textures[i].destroy();
		dataVector[i].destroy();
		
		_RELEASE(inputBuf[i]);
	}

	_RELEASE(idBuf);

	ShaderMgr::Get()->DeleteShader(shaderID);
	materialName.clear();
}

bool Material::loadMat()
{	
#ifdef _DEV
	if(!FileIO::IsExist(materialName))
		return ñonvertMat(materialName);
#endif
	
	uint32_t data_size = 0;

	unique_ptr<uint8_t> s_data(FileIO::ReadFileData(materialName, &data_size));
	if( !s_data.get() )
	{
		ERR("Cant read material file %s !", materialName.c_str());
		return false;
	}

	uint8_t* t_data = s_data.get();

	string shaderName((char*)t_data, MAT_STR_LEN);
	shaderName = shaderName.substr( 0, shaderName.find(char(0)) );
	t_data += MAT_STR_LEN;

	uint8_t isDeffered = *t_data;
	t_data += sizeof(uint8_t);

	if(isDeffered == 1)
	{
		defferedParams = new MaterialParamsStructBuffer(*(MaterialParamsStructBuffer*)t_data);
		t_data += sizeof(MaterialParamsStructBuffer);
	}

	shaderID = ShaderMgr::Get()->GetShader(shaderName, false);
	if(shaderID == SHADER_NULL)
		return false;
	auto shaderPtr = (Shader*) ShaderMgr::Get()->GetShaderPtr(shaderID);
	if(!shaderPtr)
		return false;
	uint16_t* codeIds = shaderPtr->GetCode();
	if(!codeIds)
		return false;

	for(uint8_t i = 0; i < 5; i++)
	{
		if(codeIds[i] == SHADER_NULL)
			continue;

		auto& Hcode = ShaderCodeMgr::GetShaderCodeRef(codeIds[i]);
		if(!Hcode.code)
			return false;

		vectorsReg[i] = Hcode.input.matInfo_Register;
		texReg[i] = Hcode.input.matTextures_StartRegister;
		matrixReg[i] = Hcode.input.matrixBuf_Register;

		if(i == SHADER_PS)
			sceneReg = Hcode.input.matId_Register;

		textures[i].create(Hcode.input.matTextures_Count);

		if(Hcode.input.matInfo_FloatCount / 4 * 4 != Hcode.input.matInfo_FloatCount)
		{
			WRN("Material data buffer does not aligned, floats count must be multiples of 4 in %s !", materialName.c_str());
		}

		uint16_t vects_count = Hcode.input.matInfo_FloatCount / 4 + Hcode.input.matInfo_VectorCount;
		
		dataVector[i].create(vects_count);
		offsetFloat[i] = Hcode.input.matInfo_VectorCount;

		if(vects_count > 0)
		{
			uint32_t vsize = (uint32_t)vects_count * sizeof(XMFLOAT4);
			dataVector[i].resize(vects_count);
			memcpy(dataVector[i].data(), t_data, vsize);
			t_data += vsize;
		}

		for(uint8_t j = 0; j < textures[i].capacity(); j++)
		{
			string texName((char*)t_data, TEX_STR_LEN);
			texName = texName.substr( 0, texName.find(char(0)) );
			t_data += TEX_STR_LEN;

			TextureHandle* tex = textures[i].push_back();
			tex->texture = (uint64_t)RELOADABLE_TEXTURE(texName, true); // todo
		}
	}

	return true;
}

bool Material::createMat()
{
	string shaderName = materialName.substr(1);

	shaderID = ShaderMgr::Get()->GetShader(shaderName, false);
	if(shaderID == SHADER_NULL)
		return false;
	auto shaderPtr = (Shader*) ShaderMgr::Get()->GetShaderPtr(shaderID);
	if(!shaderPtr)
		return false;
	uint16_t* codeIds = shaderPtr->GetCode();

	for(uint8_t i = 0; i < 5; i++)
	{
		if(codeIds[i] == SHADER_NULL)
			continue;

		auto& Hcode = ShaderCodeMgr::GetShaderCodeRef(codeIds[i]);
		if(!Hcode.code)
			return false;

		vectorsReg[i] = Hcode.input.matInfo_Register;
		texReg[i] = Hcode.input.matTextures_StartRegister;
		matrixReg[i] = Hcode.input.matrixBuf_Register;

		if(i == SHADER_PS)
		{
			sceneReg = Hcode.input.matId_Register;
			if(sceneReg != REGISTER_NULL)
				defferedParams = new MaterialParamsStructBuffer;
		}

		if(Hcode.input.matInfo_FloatCount / 4 * 4 != Hcode.input.matInfo_FloatCount)
		{
			WRN("Material data buffer does not aligned, floats count must be multiples of 4 in %s !", materialName.c_str());
		}

		uint16_t vects_count = Hcode.input.matInfo_FloatCount / 4 + Hcode.input.matInfo_VectorCount;
		
		dataVector[i].create(vects_count);
		dataVector[i].resize(vects_count);
		dataVector[i].assign(XMFLOAT4(0,0,0,0));

		offsetFloat[i] = Hcode.input.matInfo_VectorCount;

		textures[i].create(Hcode.input.matTextures_Count);
		textures[i].resize(Hcode.input.matTextures_Count);
	}

	return true;
}

bool Material::Save()
{
	if(materialName.size() > 0 && materialName[0] == '&')
		return false;

	uint32_t data_size = MAT_STR_LEN + sizeof(uint8_t);

	if(defferedParams)
		data_size += sizeof(MaterialParamsStructBuffer);

	for(uint8_t i = 0; i < 5; i++)
	{
		data_size += (uint32_t)dataVector[i].size() * sizeof(XMFLOAT4);
		data_size += (uint32_t)textures[i].size() * TEX_STR_LEN;
	}

	unique_ptr<uint8_t> data(new uint8_t[data_size]);
	uint8_t* t_data = data.get();

	ZeroMemory(t_data, data_size);

	string shaderName = ShaderMgr::Get()->GetShaderName(shaderID);
	memcpy(t_data, shaderName.data(), shaderName.size());
	t_data += MAT_STR_LEN;

	*t_data = defferedParams ? 1 : 0;
	t_data += sizeof(uint8_t);

	if(defferedParams)
	{
		memcpy(t_data, defferedParams, sizeof(MaterialParamsStructBuffer));
		t_data += sizeof(MaterialParamsStructBuffer);
	}

	for(uint8_t i = 0; i < 5; i++)
	{
		auto dataVectorSize = dataVector[i].size() * sizeof(XMFLOAT4);
		memcpy(t_data, dataVector[i].data(), dataVectorSize);
		t_data += dataVectorSize;

		for(uint8_t j = 0; j < textures[i].size(); j++)
		{
			string textureName = GetTextureName(j, i);
			memcpy(t_data, textureName.data(), textureName.size());
			t_data += TEX_STR_LEN;
		}
	}

	if(!FileIO::WriteFileData(materialName, data.get(), data_size))
	{
		ERR("Cant write material to file %s !", materialName.c_str());
		return false;
	}
	return true;
}

#ifdef _DEV
bool Material::ñonvertMat(string& nameBin)
{
	string nameText = nameBin;
	nameText[nameText.size() - 1] = 'a';

	FileIO file(nameText);
	auto root = file.Root();
	if(!root)
	{
		ERR("Material file (*.mtb or *.mta) does not exist!");
		return false;
	}

	string shaderName = file.ReadString("shader", root);
	
	shaderID = ShaderMgr::Get()->GetShader(shaderName, false);
	if(shaderID == SHADER_NULL)
		return false;
	auto shaderPtr = (Shader*) ShaderMgr::Get()->GetShaderPtr(shaderID);
	if(!shaderPtr)
		return false;
	uint16_t* codeIds = shaderPtr->GetCode();
	if(!codeIds)
		return false;

	sceneReg = REGISTER_NULL;

	for(uint8_t i = 0; i < 5; i++)
	{
		if(codeIds[i] == SHADER_NULL)
		{
			offsetFloat[i] = 0;
			vectorsReg[i] = REGISTER_NULL;
			texReg[i] = REGISTER_NULL;
			matrixReg[i] = REGISTER_NULL;
			continue;
		}

		auto& Hcode = ShaderCodeMgr::GetShaderCodeRef(codeIds[i]);
		if(!Hcode.code)
			return false;

		vectorsReg[i] = Hcode.input.matInfo_Register;
		texReg[i] = Hcode.input.matTextures_StartRegister;
		matrixReg[i] = Hcode.input.matrixBuf_Register;

		if(i == SHADER_PS)
			sceneReg = Hcode.input.matId_Register;

		textures[i].create(Hcode.input.matTextures_Count);
		textures[i].resize(Hcode.input.matTextures_Count);

		if(Hcode.input.matInfo_FloatCount / 4 * 4 != Hcode.input.matInfo_FloatCount)
		{
			WRN("Material data buffer does not aligned, floats count must be multiples of 4 in %s !", materialName.c_str());
		}

		uint16_t vects_count = Hcode.input.matInfo_FloatCount / 4 + Hcode.input.matInfo_VectorCount;
		
		dataVector[i].create(vects_count);
		dataVector[i].resize(vects_count);
		dataVector[i].assign(XMFLOAT4(0,0,0,0));
		offsetFloat[i] = Hcode.input.matInfo_VectorCount;
	}

	auto nodeDeffered = file.Node(L"deffered", root);
	if( sceneReg != REGISTER_NULL && nodeDeffered)
	{
		defferedParams = new MaterialParamsStructBuffer;
		
		defferedParams->unlit = (uint32_t)file.ReadFloat("f0", nodeDeffered);
		defferedParams->subscattering = (uint32_t)file.ReadFloat("f1", nodeDeffered);
		defferedParams->ss_distortion = file.ReadFloat("f2", nodeDeffered);
		defferedParams->ss_direct_translucency = file.ReadFloat("f3", nodeDeffered);
		defferedParams->ss_direct_pow = file.ReadFloat("f4", nodeDeffered);
		defferedParams->ss_indirect_translucency = file.ReadFloat("f5", nodeDeffered);
	}
	
	const char node_names[5][3] = { "ps", "vs", "ds", "hs", "gs" };

	for(uint8_t i = 0; i < 5; i++)
	{
		auto stageNode = file.Node(node_names[i], root);

		for(uint8_t j = 0; j < offsetFloat[i]; j++)
			dataVector[i][j] = file.ReadFloat4("v" + IntToString(j), stageNode);

		for(uint8_t j = 0; j < 4 * (dataVector[i].size() - offsetFloat[i]); j++)
		{
			float fdata = file.ReadFloat("f" + IntToString(j), stageNode);
			SetFloat(fdata, j, i);
		}

		for(uint8_t j = 0; j < textures[i].size(); j++)
		{			
			string texName = file.ReadString("t" + IntToString(j), stageNode);
			TextureHandle& tex = textures[i][j];
			tex.texture = (uint64_t)RELOADABLE_TEXTURE(texName, true); // todo
		}
	}

	return Save();
}
#endif

bool Material::initBuffers()
{
	for(uint8_t i = 0; i < 5; i++)
	{
		inputBuf[i] = nullptr;
		if(dataVector[i].size() == 0)
			continue;

		inputBuf[i] = Buffer::CreateConstantBuffer(DEVICE, (int)dataVector[i].size() * sizeof(XMFLOAT4), false);
		if (!inputBuf[i])
			return false;
	}

	if(!defferedParams || sceneReg == REGISTER_NULL)
		return true;

	idBuf = Buffer::CreateConstantBuffer(Render::Device(), sizeof(uint32_t) * 4, false);
	if (!idBuf)
		return false;

	return true;
}

bool Material::SetTextureByName(string& name, uint8_t id, uint8_t shader)
{
	if(id >= textures[shader].size())
		return false;
	TextureHandle& tex = textures[shader][id];
	if(!tex.is_ptr)
		TEXTURE_DROP(tex.texture);

	auto texid = RELOADABLE_TEXTURE(name, true); // todo
	tex.texture = (uint64_t)texid;

	if(texid == TEX_NULL)
		return false;
	return true;
}

void Material::SetTexture(ID3D11ShaderResourceView *texture, uint8_t id, uint8_t shader)
{
	if(id >= textures[shader].size())
		return;
	TextureHandle& tex = textures[shader][id];
	if(!tex.is_ptr)
		TEXTURE_DROP(tex.texture);

	tex.texture = reinterpret_cast<uint64_t>(texture);
	tex.is_ptr = true;
}

string Material::GetTextureName(uint8_t id, uint8_t shader)
{
	if(id >= textures[shader].size())
		return string();
	TextureHandle& tex = textures[shader][id];
	if(tex.is_ptr)
		return string();
	return TexMgr::GetTextureName((uint32_t)tex.texture);;
}

void Material::ClearTextures()
{
	for(uint8_t i = 0; i < 5; i++)
		for(uint8_t j = 0; j < textures[i].size(); j++)
		{
			TextureHandle& tex = textures[i][j];
			if(tex.is_ptr)
				tex.texture = TEX_NULL;
			else
				TEXTURE_DROP(tex.texture);
			tex.is_ptr = false;
		}
}

void Material::updateBuffers()
{
	for(uint i=0; i<5; i++)
		if(inputBuf[i] != nullptr)
			Render::UpdateSubresource(inputBuf[i], 0, NULL, dataVector[i].data(), 0, 0);
	b_dirty = false;
}

void Material::AddToFrameBuffer(MaterialParamsStructBuffer* buf, uint32_t* i)
{
	*buf = *defferedParams;
	scene_id = *i << 16;
	*i = *i + 1;
}

#define SETTEXTURES(stage) if( textures[SHADER_##stage].size() > 0 ){	\
		uint8_t currentReg = texReg[SHADER_##stage];						\
		for(uint8_t i = 0; i < textures[SHADER_##stage].size(); i++){	\
			auto& tex = textures[SHADER_##stage][i];						\
			if(tex.is_ptr){											\
				auto ptr = reinterpret_cast<ID3D11ShaderResourceView*>(tex.texture); \
				Render::##stage##SetShaderResources(currentReg, 1, &ptr);}	\
			else													\
				Render::##stage##SetShaderTexture(currentReg, (uint32_t)tex.texture); \
			currentReg++;}}

void Material::Set(TECHNIQUES tech)
{
	auto shaderPtr = (Shader*) ShaderMgr::GetShaderPtr(shaderID);
	if(!shaderPtr->HasTechnique(tech))
		return;

	if(b_dirty)
		updateBuffers();		
	
	SETTEXTURES(VS)
	SETTEXTURES(PS)
	SETTEXTURES(DS)
	SETTEXTURES(HS)
	SETTEXTURES(GS)
	
	if(idBuf)
	{
		uint32_t id[4];
		id[0] = scene_id;
		Render::UpdateSubresource(idBuf, 0, NULL, id, 0, 0);
		Render::PSSetConstantBuffers(sceneReg, 1, &idBuf);
	}

	if(inputBuf[SHADER_PS] != nullptr)
		Render::PSSetConstantBuffers(vectorsReg[SHADER_PS], 1, &inputBuf[SHADER_PS]);

	if(inputBuf[SHADER_VS] != nullptr)
		Render::VSSetConstantBuffers(vectorsReg[SHADER_VS], 1, &inputBuf[SHADER_VS]);

	if(inputBuf[SHADER_DS] != nullptr)
		Render::DSSetConstantBuffers(vectorsReg[SHADER_DS], 1, &inputBuf[SHADER_DS]);

	if(inputBuf[SHADER_HS] != nullptr)
		Render::HSSetConstantBuffers(vectorsReg[SHADER_HS], 1, &inputBuf[SHADER_HS]);
	
	if(inputBuf[SHADER_GS] != nullptr)
		Render::GSSetConstantBuffers(vectorsReg[SHADER_GS], 1, &inputBuf[SHADER_GS]);

	shaderPtr->Set(tech);
}

void Material::SetMatrixBuffer(ID3D11Buffer* matrixBuf)
{
	if(matrixReg[SHADER_VS] != REGISTER_NULL)
		Render::Context()->VSSetConstantBuffers(matrixReg[SHADER_VS], 1, &matrixBuf);
	if(matrixReg[SHADER_PS] != REGISTER_NULL)
		Render::Context()->PSSetConstantBuffers(matrixReg[SHADER_PS], 1, &matrixBuf);
	if(matrixReg[SHADER_HS] != REGISTER_NULL)
		Render::Context()->HSSetConstantBuffers(matrixReg[SHADER_HS], 1, &matrixBuf);
	if(matrixReg[SHADER_DS] != REGISTER_NULL)
		Render::Context()->DSSetConstantBuffers(matrixReg[SHADER_DS], 1, &matrixBuf);
	if(matrixReg[SHADER_GS] != REGISTER_NULL)
		Render::Context()->GSSetConstantBuffers(matrixReg[SHADER_GS], 1, &matrixBuf);	
}

void Material::SetVector(XMFLOAT4& vect, uint8_t id, uint8_t shader)
{
	if(id >= offsetFloat[shader])
		return;
	dataVector[shader][id] = vect;
	b_dirty = true;
}

void Material::SetFloat(float f, uint8_t id, uint8_t shader)
{
	if(id >= (dataVector[shader].size() - offsetFloat[shader]) * 4)
		return;

	uint8_t v_num = uint8_t(id / 4);
	uint8_t f_num = id - v_num * 4;
	v_num += offsetFloat[shader];

	switch (f_num)
	{
	case 0:
		dataVector[shader][v_num].x = f;
		break;
	case 1:
		dataVector[shader][v_num].y = f;
		break;
	case 2:
		dataVector[shader][v_num].z = f;
		break;
	case 3:
		dataVector[shader][v_num].w = f;
		break;
	}
	b_dirty = true;
}

XMFLOAT4 Material::GetVector(uint8_t id, uint8_t shader)
{
	if(id >= offsetFloat[shader])
		return XMFLOAT4(0,0,0,0);
	return dataVector[shader][id];
}

float Material::GetFloat(uint8_t id, uint8_t shader)
{
	if(id >= (dataVector[shader].size() - offsetFloat[shader]) * 4)
		return 0.0f;

	uint8_t v_num = uint8_t(id / 4);
	uint8_t f_num = id - v_num * 4;
	v_num += offsetFloat[shader];

	switch (f_num)
	{
	case 0:
		return dataVector[shader][v_num].x;
		break;
	case 1:
		return dataVector[shader][v_num].y;
		break;
	case 2:
		return dataVector[shader][v_num].z;
		break;
	case 3:
		return dataVector[shader][v_num].w;
		break;
	}
	return 0.0f;
}

void Material::SetDefferedParam(float data, uint8_t i)
{
	if(!defferedParams) return;
	switch (i)
	{
	case 0:
		defferedParams->unlit = unsigned int(data);
		break;
	case 1:
		defferedParams->subscattering = unsigned int(data);
		break;
	case 2:
		defferedParams->ss_distortion = data;
		break;
	case 3:
		defferedParams->ss_direct_translucency = data;
		break;
	case 4:
		defferedParams->ss_direct_pow = data;
		break;
	case 5:
		defferedParams->ss_indirect_translucency = data;
		break;
	}
}

float Material::GetDefferedParam(uint8_t i)
{
	if(!defferedParams) return 0;
	switch (i)
	{
	case 0:	return float(defferedParams->unlit);
	case 1:	return float(defferedParams->subscattering);
	case 2:	return defferedParams->ss_distortion;
	case 3:	return defferedParams->ss_direct_translucency;
	case 4:	return defferedParams->ss_direct_pow;
	case 5:	return defferedParams->ss_indirect_translucency;
	}
	return 0;
}

/////////////////////////////////////////////

SimpleShaderInst::SimpleShaderInst(string& shaderName)
{
	inputBuf = nullptr;
	offsetFloat = 0;
	vectorsReg = REGISTER_NULL;
	texReg = REGISTER_NULL;
	matrixReg = REGISTER_NULL;
	shaderID = SHADER_NULL;

	if(!initInst(shaderName))
	{
		ERR("Cant initialize shader instance for %s", shaderName.c_str());
		return;
	}

	if(!initBuffers())
	{
		ERR("Cant initialize buffers for instance of %s", shaderName.c_str());
		return;
	}

	b_dirty = true;
}

SimpleShaderInst::~SimpleShaderInst()
{
	ClearTextures();
	
	textures.destroy();
	dataVector.destroy();

	_RELEASE(inputBuf);

	ShaderMgr::Get()->DeleteShader(shaderID);
}

bool SimpleShaderInst::initInst(string& shaderName)
{
	shaderID = ShaderMgr::Get()->GetShader(shaderName, true);
	if(shaderID == SHADER_NULL)
		return false;
	auto shaderPtr = (SimpleShader*) ShaderMgr::Get()->GetShaderPtr(shaderID);
	if(!shaderPtr)
		return false;
	uint16_t* codeIds = shaderPtr->GetCode();
	
	auto& PScode = ShaderCodeMgr::GetShaderCodeRef(codeIds[SHADER_PS]);
	if(!PScode.code)
		return false;

	auto& VScode = ShaderCodeMgr::GetShaderCodeRef(codeIds[SHADER_VS]);
	if(!VScode.code)
		return false;

	vectorsReg = PScode.input.matInfo_Register;
	texReg = PScode.input.matTextures_StartRegister;
	matrixReg = VScode.input.matrixBuf_Register;

	textures.create(PScode.input.matTextures_Count);
	textures.resize(PScode.input.matTextures_Count);

	if(PScode.input.matInfo_FloatCount / 4 * 4 != PScode.input.matInfo_FloatCount)
	{
		WRN("Material data buffer does not aligned, floats count must be multiples of 4 in %s !", shaderName.c_str());
	}

	uint16_t vects_count = PScode.input.matInfo_FloatCount / 4 + PScode.input.matInfo_VectorCount;
	dataVector.create(vects_count);
	dataVector.resize(vects_count);
	dataVector.assign(XMFLOAT4(0,0,0,0));
	
	offsetFloat = PScode.input.matInfo_VectorCount;

	return true;
}

bool SimpleShaderInst::initBuffers()
{
	inputBuf = nullptr;
	if(dataVector.size() == 0)
		return true;

	inputBuf = Buffer::CreateConstantBuffer(DEVICE, (int)dataVector.size() * sizeof(XMFLOAT4), false);
	if (!inputBuf)
		return false;

	return true;
}

void SimpleShaderInst::updateBuffers()
{
	if(inputBuf != nullptr)
		Render::UpdateSubresource(inputBuf, 0, NULL, dataVector.data(), 0, 0);
	b_dirty = false;
}

void SimpleShaderInst::Set()
{
	if(b_dirty)
		updateBuffers();		
	
	if( textures.size() > 0 )
	{	
		uint8_t currentReg = texReg;						
		for(uint8_t i = 0; i < textures.size(); i++)
		{	
			auto& tex = textures[i];						
			if(tex.is_ptr)
			{											
				auto ptr = reinterpret_cast<ID3D11ShaderResourceView*>(tex.texture); 
				Render::PSSetShaderResources(currentReg, 1, &ptr);
			}	
			else													
				Render::PSSetShaderTexture(currentReg, (uint32_t)tex.texture); 
			currentReg++;
		}
	}
	
	if(inputBuf != nullptr)
		Render::PSSetConstantBuffers(vectorsReg, 1, &inputBuf);

	((SimpleShader*)ShaderMgr::GetShaderPtr(shaderID))->Set();
}

void SimpleShaderInst::SetMatrixBuffer(ID3D11Buffer* matrixBuf)
{
	if(matrixReg != REGISTER_NULL)
		Render::Context()->VSSetConstantBuffers(matrixReg, 1, &matrixBuf);
}

void SimpleShaderInst::SetVector(XMFLOAT4& vect, uint8_t id)
{
	if(id >= offsetFloat)
		return;
	dataVector[id] = vect;
	b_dirty = true;
}

void SimpleShaderInst::SetFloat(float f, uint8_t id)
{
	if(id >= (dataVector.size() - offsetFloat) * 4)
		return;

	uint8_t v_num = uint8_t(id / 4);
	uint8_t f_num = id - v_num * 4;
	v_num += offsetFloat;

	switch (f_num)
	{
	case 0:
		dataVector[v_num].x = f;
		break;
	case 1:
		dataVector[v_num].y = f;
		break;
	case 2:
		dataVector[v_num].z = f;
		break;
	case 3:
		dataVector[v_num].w = f;
		break;
	}
	b_dirty = true;
}

XMFLOAT4 SimpleShaderInst::GetVector(uint8_t id)
{
	if(id >= offsetFloat)
		return XMFLOAT4(0,0,0,0);
	return dataVector[id];
}

float SimpleShaderInst::GetFloat(uint8_t id)
{
	if(id >= (dataVector.size() - offsetFloat) * 4)
		return 0.0f;

	uint8_t v_num = uint8_t(id / 4);
	uint8_t f_num = id - v_num * 4;
	v_num += offsetFloat;

	switch (f_num)
	{
	case 0:
		return dataVector[v_num].x;
		break;
	case 1:
		return dataVector[v_num].y;
		break;
	case 2:
		return dataVector[v_num].z;
		break;
	case 3:
		return dataVector[v_num].w;
		break;
	}
	return 0.0f;
}

bool SimpleShaderInst::SetTextureByName(string& name, uint8_t id)
{
	if(id >= textures.size())
		return false;
	TextureHandle& tex = textures[id];
	if(!tex.is_ptr)
		TEXTURE_DROP(tex.texture);

	auto texid = RELOADABLE_TEXTURE(name, true); // todo
	tex.texture = (uint64_t)texid;

	if(texid == TEX_NULL)
		return false;
	return true;
}

void SimpleShaderInst::SetTexture(ID3D11ShaderResourceView *texture, uint8_t id)
{
	if(id >= textures.size())
		return;
	TextureHandle& tex = textures[id];
	if(!tex.is_ptr)
		TEXTURE_DROP(tex.texture);

	tex.texture = reinterpret_cast<uint64_t>(texture);
	tex.is_ptr = true;
}

string SimpleShaderInst::GetTextureName(uint8_t id)
{
	if(id >= textures.size())
		return string();
	TextureHandle& tex = textures[id];
	if(tex.is_ptr)
		return string();
	return TexMgr::GetTextureName((uint32_t)tex.texture);;
}

void SimpleShaderInst::ClearTextures()
{
	for(uint8_t j = 0; j < textures.size(); j++)
	{
		TextureHandle& tex = textures[j];
		if(tex.is_ptr)
			tex.texture = TEX_NULL;
		else
			TEXTURE_DROP(tex.texture);
		tex.is_ptr = false;
	}
}