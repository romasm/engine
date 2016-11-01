#include "stdafx.h"
#include "ShaderCodeMgr.h"
#include "Shader.h"
#include "macros.h"
#include "Render.h"
#include "ScenePipeline.h"
#include "Log.h"

using namespace EngineCore;

ShaderCodeMgr *ShaderCodeMgr::instance = nullptr;

ShaderCodeMgr::ShaderCodeMgr()
{
	if(!instance)
	{
		instance = this;

		shader_array.resize(SHADER_MAX_COUNT);
		shader_free.resize(SHADER_MAX_COUNT);
		for(uint32_t i=0; i<SHADER_MAX_COUNT; i++)
			shader_free[i] = i;
		for(uint32_t i=0; i<6; i++)
			shader_map[i].reserve(SHADER_INIT_COUNT);
		layout_map.reserve(LAYOUT_INIT_COUNT);
	}
	else
		ERR("Only one instance of ShaderCodeMgr is allowed!");
}

ShaderCodeMgr::~ShaderCodeMgr()
{
	for(auto &it: layout_map)
		_RELEASE(it.second);

	for(uint32_t i=0; i<SHADER_MAX_COUNT; i++)
	{
		if(shader_array[i].code)
			((ID3D11DeviceChild*)shader_array[i].code)->Release();
		shader_array[i].name.erase();
	}

	instance = nullptr;
}

void ShaderCodeMgr::PreloadPureCodes()
{
	COMPUTE( COMPUTE_VOXEL_INJECT_LIGHT, InjectLightToVolume );
}

uint16_t ShaderCodeMgr::GetShaderCode(string& name, uint8_t type)
{
	uint16_t res = SHADER_NULL;
	if(name.length() == 0 || type > SHADER_CS)
		return res;

	res = FindShaderInList(name, type);
	if(res != SHADER_NULL)
		return res;

	res = AddShaderToList(name, type);
	if(res != SHADER_NULL)
		return res;

	ERR("Cant load shader code %s", name.c_str());

	return res;
}

void ShaderCodeMgr::DeleteShaderCode(uint16_t id, uint8_t type)
{
	if(id == SHADER_NULL || type > SHADER_CS)
		return;
	
	auto& handle = shader_array[id];

	if(handle.refcount == 1)
	{
		if(handle.code)
		{
			((ID3D11DeviceChild*)handle.code)->Release();
			handle.code = nullptr;
		}

		handle.input.layout = nullptr;
		handle.input.samplers.destroy();
		handle.input = CodeInput();

		handle.refcount = 0;

		shader_free.push_back(id);

		shader_map[type].erase(handle.name);

		handle.name.clear();
	}
	else if(handle.refcount == 0)
	{
		WRN("Shader code %s has already deleted!", handle.name.c_str());
	}
	else
		handle.refcount--;
}

uint16_t ShaderCodeMgr::FindShaderInList(string& name, uint8_t type)
{
	auto it = shader_map[type].find(name);
	if(it == shader_map[type].end())
		return SHADER_NULL;

	auto& handle = shader_array[it->second];
	handle.refcount++;
	return it->second;
}

uint16_t ShaderCodeMgr::AddShaderToList(string& name, uint8_t type)
{
	uint8_t* s_data = nullptr;
	ID3DBlob* s_blob = nullptr;

	uint8_t* data_code = nullptr;
	uint32_t data_size = 0;
	uint32_t file_date = 0;

	if(shader_free.size() == 0)
	{
		ERR("Shader code amount overflow!");
		return SHADER_NULL;
	}

	string bcFilename = name + EXT_SHADER_BYTECODE;
	
#ifdef _DEV
	auto del = name.rfind('_');
	if(del == string::npos || del == name.size() - 1)
	{
		ERR("Wrong shader path %s !", name.c_str());
		return SHADER_NULL;
	}

	string srcFilename = name.substr(0, del) + EXT_SHADER_SOURCE;
	string entryPoint = name.substr(del+1);

	if(!FileIO::IsExist(bcFilename) || FileIO::GetDateModifRaw(bcFilename) < FileIO::GetDateModifRaw(srcFilename))
	{
		s_blob = CompileShader(srcFilename, bcFilename, entryPoint, type, &file_date);
		if(!s_blob)
			return SHADER_NULL;
		data_code = (uint8_t*)s_blob->GetBufferPointer();
		data_size = (uint32_t)s_blob->GetBufferSize();
	}
	else
#else
	if(!FileIO::IsExist(bcFilename))
	{
		ERR("Shader file %s does not exist!", bcFilename.c_str());
		return SHADER_NULL;
	}
#endif
	{	
		if( !(s_data = FileIO::ReadFileData(bcFilename, &data_size)) )
		{
			ERR("Cant read shader file %s", bcFilename.c_str());
			return SHADER_NULL;
		}
		data_size -= sizeof(uint32_t);
		data_code = s_data + sizeof(uint32_t);
		file_date = *((uint32_t*)s_data);
	}

	uint32_t idx = shader_free.front();
	auto& HCode = shader_array[idx];

	HRESULT hr = S_OK;
	switch (type)
	{
	case SHADER_PS:hr = DEVICE->CreatePixelShader(data_code, data_size, NULL, (ID3D11PixelShader**)(&HCode.code));break;
	case SHADER_VS:hr = DEVICE->CreateVertexShader(data_code, data_size, NULL, (ID3D11VertexShader**)(&HCode.code));break;
	case SHADER_HS:hr = DEVICE->CreateHullShader(data_code, data_size, NULL, (ID3D11HullShader**)(&HCode.code));break;
	case SHADER_DS:hr = DEVICE->CreateDomainShader(data_code, data_size, NULL, (ID3D11DomainShader**)(&HCode.code));break;
	case SHADER_GS:hr = DEVICE->CreateGeometryShader(data_code, data_size, NULL, (ID3D11GeometryShader**)(&HCode.code));break;
	case SHADER_CS:hr = DEVICE->CreateComputeShader(data_code, data_size, NULL, (ID3D11ComputeShader**)(&HCode.code));break;
	}
	if( FAILED(hr) )
	{
		ERR("Cant create shader %s", name.c_str());
		_DELETE_ARRAY(s_data);
		_RELEASE(s_blob);
		return SHADER_NULL;
	}
	
	if(!GetInputData(HCode.input, data_code, data_size, type))
	{
		ERR("Wrong shader input data in %s !", name.c_str());
		((ID3D11DeviceChild*)HCode.code)->Release();
		HCode.code = nullptr;
		_DELETE_ARRAY(s_data);
		_RELEASE(s_blob);
		return SHADER_NULL;
	}

	_RELEASE(s_blob);
	_DELETE_ARRAY(s_data);

	HCode.refcount = 1;
	HCode.filedate = file_date;
	HCode.name = name;

	shader_map[type].insert(make_pair(name, idx));
	shader_free.pop_front();

	return idx;
}

#ifdef _DEV
void ShaderCodeMgr::UpdateShadersCode()
{
	for(uint8_t type = 0; type < 6; type++)
		for(auto& it: shader_map[type])
		{
			auto& shader_code = shader_array[it.second];

			auto del = it.first.rfind('_');
			if(del == string::npos)
			{
				ERR("Wrong shader path %s !", it.first.c_str());
				continue;
			}
			string srcFilename = it.first.substr(0, del) + EXT_SHADER_SOURCE;

			uint32_t last_date = FileIO::GetDateModifRaw(srcFilename);
			if(last_date == shader_code.filedate)
				continue;
			
			shader_code.filedate = last_date;

			string entryPoint = it.first.substr(del+1);
			string bcFilename = it.first + EXT_SHADER_BYTECODE;

			uint32_t date = 0;
			auto blob = CompileShader(srcFilename, bcFilename, entryPoint, type, &date);
			if(!blob)
				continue;

			void* code_ptr = nullptr;
			HRESULT hr;
			switch (type)
			{
			case SHADER_PS:hr = DEVICE->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11PixelShader**)(&code_ptr));break;
			case SHADER_VS:hr = DEVICE->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11VertexShader**)(&code_ptr));break;
			case SHADER_HS:hr = DEVICE->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11HullShader**)(&code_ptr));break;
			case SHADER_DS:hr = DEVICE->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11DomainShader**)(&code_ptr));break;
			case SHADER_GS:hr = DEVICE->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11GeometryShader**)(&code_ptr));break;
			case SHADER_CS:hr = DEVICE->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, (ID3D11ComputeShader**)(&code_ptr));break;
			}
			_RELEASE(blob);
			if( FAILED(hr) )
			{
				ERR("Cant create shader %s", it.first.c_str());
				continue;
			}

			shader_code.code = code_ptr;
		}
}

ID3DBlob* ShaderCodeMgr::CompileShader(string& file, string& binFile, string& entryPoint, uint8_t type, uint32_t* date)
{
	if(file.size() <= 3)
	{
		ERR("Shader source filename is too short!");
		return nullptr;
	}

	uint32_t code_size = 0;
	uint8_t* code_text = FileIO::ReadFileData(file, &code_size);
	if( !code_text )
	{
		ERR("Cant read shader source file %s", file.c_str());
		return nullptr;
	}

	string target;
	switch(type)
	{
	case SHADER_VS:
		target = "vs_"SHADER_COMPILE_TARGET;
		break;
	case SHADER_PS:
		target = "ps_"SHADER_COMPILE_TARGET;
		break;
	case SHADER_HS: 
		target = "hs_"SHADER_COMPILE_TARGET;
		break;
	case SHADER_DS:
		target = "ds_"SHADER_COMPILE_TARGET;
		break;
	case SHADER_GS:
		target = "gs_"SHADER_COMPILE_TARGET;
		break;
	case SHADER_CS:
		target = "cs_"SHADER_COMPILE_TARGET;
		break;
	}

	LOG("Compiling shader %s", file.c_str());
	
	// remove techs
	struct interval {uint32_t start; uint32_t end; uint32_t str_count;};
	DArray<interval> tech_int;
	for(uint32_t i = 0; i < code_size - TECHIQUE_STR_SIZE; i++)
	{
		if(string((char*)&code_text[i], TECHIQUE_STR_SIZE) == TECHIQUE_STR)
		{
			interval tech;
			tech.start = i;
			tech.str_count = 0;

			while(i < code_size && ((char*)code_text)[i] != '}' )
			{
				if( ((char*)code_text)[i] == '\n' )
					tech.str_count++;
				i++;
			}
			
			tech.end = i;
			tech_int.push_back(tech);
		}
	}

	uint32_t pure_code_size = code_size + 1;
	for(auto& it: tech_int)
	{
		pure_code_size -= (it.end - it.start) + 1;
		pure_code_size += it.str_count;
	}

	uint8_t* pure_code = new uint8_t[pure_code_size];

	uint8_t* pure_code_cur = pure_code;
	uint8_t* code_cur = code_text;
	uint32_t last = 0;
	for(auto& it: tech_int)
	{
		auto size = it.start - last;
		memcpy(pure_code_cur, code_cur, size);
		pure_code_cur += size;
		code_cur += size;
		code_cur += (it.end - it.start) + 1;
		last = it.end + 1;

		for(uint32_t p = 0; p < it.str_count; p++)
		{
			*pure_code_cur = (uint8_t)'\n';
			pure_code_cur += 1;
		}
	}

	memcpy(pure_code_cur, code_cur, code_size - last);
	pure_code[pure_code_size-1] = 0;

	_DELETE_ARRAY(code_text);
	// remove techs

	ID3DBlob* code_compiled = nullptr;
	ID3DBlob* code_errors = nullptr;

	const D3D_SHADER_MACRO defines[] = 
    {
        //"EXAMPLE_DEFINE", "1",
        NULL, NULL
    };
	
	HRESULT hr = D3DCompile(pure_code, pure_code_size, file.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, 
		entryPoint.c_str(), target.c_str(), 0, 0, &code_compiled, &code_errors);

	if( FAILED(hr) )
	{
		if(code_errors)
		{
			ERR("Shader compilation errors:\n%s", (char*)code_errors->GetBufferPointer() );
			_RELEASE(code_errors);
		}
		_RELEASE(code_compiled);
		_DELETE_ARRAY(pure_code);
		return nullptr;
	}
	_RELEASE(code_errors);
	_DELETE_ARRAY(pure_code);
	
	uint32_t mdate = FileIO::GetDateModifRaw(file);
	if(!FileIO::WriteFileData( binFile, (uint8_t*)code_compiled->GetBufferPointer(), (uint32_t)code_compiled->GetBufferSize(), mdate ))
	{
		ERR("Cant write shader file: %s", binFile.c_str() );
		_RELEASE(code_compiled);
		return nullptr;
	}
		
	if(date)
		*date = mdate;

	LOG_GOOD("Shader %s compiled successfully", file.c_str());

	return code_compiled;
}
#endif

bool ShaderCodeMgr::GetInputData(CodeInput& HInput, uint8_t* data, uint32_t size, uint8_t type)
{
	ID3D11ShaderReflection* reflector = nullptr; 
	D3D11Reflect(data, size, &reflector);

	D3D11_SHADER_DESC shader_desc;
	HRESULT hr = reflector->GetDesc(&shader_desc);
	if(FAILED(hr))
		return false;
	
	uint8_t samplersOrder = 0;
	HInput.matTextures_StartRegister = 0;
	HInput.matTextures_Count = 0;

	for(UINT i = 0; i < shader_desc.BoundResources; i++)
	{
		D3D11_SHADER_INPUT_BIND_DESC desc;
		HRESULT hr = reflector->GetResourceBindingDesc(i, &desc);
		if(FAILED(hr))
			continue;

		string bufName = desc.Name;
		switch (desc.Type)
		{
		case D3D_SIT_CBUFFER:
			if(bufName == "materialBuffer")
			{
				HInput.matInfo_Register = desc.BindPoint;

				auto constBuf = reflector->GetConstantBufferByName(desc.Name);
				if(!constBuf)
					continue;

				D3D11_SHADER_BUFFER_DESC descBuf;
				HRESULT hr = constBuf->GetDesc(&descBuf);
				if(FAILED(hr))
					continue;

				HInput.matInfo_FloatCount = 0;
				HInput.matInfo_VectorCount = 0;

				for(UINT j = 0; j < descBuf.Variables; j++)
				{
					auto var = constBuf->GetVariableByIndex(j);
					if(!var)
						continue;

					D3D11_SHADER_VARIABLE_DESC descVar;
					HRESULT hr = var->GetDesc(&descVar);
					if(FAILED(hr))
						continue;

					switch(descVar.Size)
					{
					case 4:
						HInput.matInfo_FloatCount++;
						break;
					case 16:
						HInput.matInfo_VectorCount++;
						break;
					default:
						ERR("Wrong variable size for %s in cbuffer %s !", descVar.Name, descBuf.Name);
						return false;
					}
				}
			}
			else if(bufName == "materialId")
				HInput.matId_Register = (uint8_t)desc.BindPoint;
			else if(bufName == "matrixBuffer")
				HInput.matrixBuf_Register = (uint8_t)desc.BindPoint;
			break;
		case D3D_SIT_SAMPLER:
			{
				if(desc.BindPoint != samplersOrder)
				{
					ERR("Wrong samplers order for %s !", desc.Name);
					return false;
				}

				auto sampler = SamplerStateMgr::GetSampler(bufName);
				if(!sampler)
				{
					ERR("Wrong sampler name %s !", desc.Name);
					return false;
				}

				HInput.samplers.push_back(sampler);
				samplersOrder++;
			}
			break;
		case D3D_SIT_TEXTURE:
			{
				if(HInput.matTextures_Count == 0)
				{
					if(bufName.find("sys_") != string::npos)
						break;
					else
						HInput.matTextures_StartRegister = (uint8_t)desc.BindPoint;
				}
				
				if(bufName.find("sys_") != string::npos)
					WRN("System texture %s must be in the beginning, interpreted like user texture!", desc.Name);
				
				HInput.matTextures_Count++;
			}
			break;
		}
	}

	// LAYOUT
	if(type == SHADER_VS)
	{
		HInput.layout = GetVertexLayout(data, size);
		if(!HInput.layout)
		{
			ERR("Shader resources amount overflow!");
			return false;
		}
	}

	return true;
}

// LAYOUT ----------------------

ID3D11InputLayout* ShaderCodeMgr::GetVertexLayout(uint8_t* data, uint32_t size)
{
	ID3D11ShaderReflection* reflector = nullptr; 
	D3D11Reflect(data, size, &reflector);

	D3D11_SHADER_DESC shader_desc;
	HRESULT hr = reflector->GetDesc(&shader_desc);
	if(FAILED(hr) || shader_desc.InputParameters == 0)
		return nullptr;

	D3D11_INPUT_ELEMENT_DESC *layoutformat = new D3D11_INPUT_ELEMENT_DESC[shader_desc.InputParameters];

	string key = "";
	D3D11_SIGNATURE_PARAMETER_DESC* input_desc = new D3D11_SIGNATURE_PARAMETER_DESC[shader_desc.InputParameters];
	for(uint32_t i = 0; i < shader_desc.InputParameters; i++)
	{
		hr = reflector->GetInputParameterDesc(i, &input_desc[i]);
		if(FAILED(hr))
		{
			_DELETE_ARRAY(layoutformat);
			return nullptr;
		}

		auto format = GetInputFormat(input_desc[i].ComponentType, input_desc[i].Mask);
		
		layoutformat[i].SemanticName = input_desc[i].SemanticName;
		layoutformat[i].SemanticIndex = input_desc[i].SemanticIndex;
		layoutformat[i].Format = format;
		layoutformat[i].InputSlot = 0;
		if ( i == 0 )
			layoutformat[i].AlignedByteOffset = 0;
		else
			layoutformat[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		layoutformat[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		layoutformat[i].InstanceDataStepRate = 0;
		
		key.append(1, (char)format + '0');
		key.append(input_desc[i].SemanticName);
		key.append(1, (char)(input_desc[i].SemanticIndex) + '0');
	}

	auto it = layout_map.find(key);
	if(it != layout_map.end())
	{
		_DELETE_ARRAY(layoutformat);
		_DELETE_ARRAY(input_desc);
		return it->second;
	}

	ID3D11InputLayout* layout = nullptr;
	hr = DEVICE->CreateInputLayout(layoutformat, shader_desc.InputParameters, data, size, &layout);
	_DELETE_ARRAY(layoutformat);
	_DELETE_ARRAY(input_desc);
	if( FAILED(hr) )
	{
		ERR("Cant create layout format: %s", key.c_str());
		return nullptr;
	}

	layout_map.insert(make_pair(key, layout));
	return layout;
}

DXGI_FORMAT ShaderCodeMgr::GetInputFormat(D3D_REGISTER_COMPONENT_TYPE component, BYTE mask)
{
	if( component == D3D_REGISTER_COMPONENT_UINT32 )
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_UINT;
		case 0x03: return DXGI_FORMAT_R32G32_UINT;
		case 0x07: return DXGI_FORMAT_R32G32B32_UINT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_UINT;
		}
	}
	else if( component == D3D_REGISTER_COMPONENT_SINT32 )
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_SINT;
		case 0x03: return DXGI_FORMAT_R32G32_SINT;
		case 0x07: return DXGI_FORMAT_R32G32B32_SINT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_SINT;
		}
	}
	else if( component == D3D_REGISTER_COMPONENT_FLOAT32 )
	{
		switch(mask)
		{
		case 0x01: return DXGI_FORMAT_R32_FLOAT;
		case 0x03: return DXGI_FORMAT_R32G32_FLOAT;
		case 0x07: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 0x0f: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}