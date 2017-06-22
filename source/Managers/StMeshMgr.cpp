#include "stdafx.h"
#include "StMeshMgr.h"
#include "Common.h"
#include "Render.h"
#include "Buffer.h"
#include "Material.h"
#include "WorldMgr.h"

using namespace EngineCore;

StMeshMgr* StMeshMgr::instance = nullptr;
StMeshData* StMeshMgr::null_mesh = nullptr;
string StMeshMgr::null_name = "";

StMeshMgr::StMeshMgr()
{
	if(!instance)
	{
		instance = this;

		mesh_array.resize(STMESH_MAX_COUNT);
		mesh_free.resize(STMESH_MAX_COUNT);
		for(uint32_t i=0; i<STMESH_MAX_COUNT; i++)
			mesh_free[i] = i;
		mesh_map.reserve(STMESH_INIT_COUNT);
		mesh_reloaded.resize(STMESH_MAX_COUNT);
		mesh_reloaded.assign(0);
		
		null_mesh = LoadFromFile(string(PATH_STMESH_NULL));
	}
	else
		ERR("Only one instance of StMeshMgr is allowed!");
}

StMeshMgr::~StMeshMgr()
{
	for(uint32_t i=0; i<STMESH_MAX_COUNT; i++)
	{
		_CLOSE(mesh_array[i].mesh);
		mesh_array[i].name.clear();
	}
	_CLOSE(null_mesh);
	null_name.clear();

	instance = nullptr;
}

void StMeshMgr::PreloadStMeshes()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetStMesh(string(ENV_MESH), reload);
	GetStMesh(string(MAT_MESH), reload);
}

uint32_t StMeshMgr::GetStMesh(string& name, bool reload)
{
	uint32_t res = STMESH_NULL;
	if(name.length() == 0)
		return res;

	res = FindStMeshInList(name);
	if(res != STMESH_NULL)
		return res;

	res = AddStMeshToList(name, reload);
	if(res != STMESH_NULL)
		return res;

	ERR("Cant load static mesh %s", name.c_str());

	return res;
}

uint32_t StMeshMgr::AddStMeshToList(string& name, bool reload)
{
	if(mesh_free.size() == 0)
	{
		ERR("Static meshes resources amount overflow!");
		return STMESH_NULL;
	}

	uint32_t idx = mesh_free.front();
	auto& handle = mesh_array[idx];
	
	if(!FileIO::IsExist(name))
		return STMESH_NULL;

	handle.mesh = LoadFromFile( name ); // TODO: loading in background
	if(!handle.mesh || handle.mesh->matCount == 0)
	{
		_CLOSE(handle.mesh);
		handle.mesh = null_mesh;
	}
	
	handle.name = name;
	handle.refcount = 1;

	if(reload)
		handle.filedate = FileIO::GetDateModifRaw(name);
	else
		handle.filedate = ReloadingType::RELOAD_NOT;

	mesh_map.insert(make_pair(name, idx));
	mesh_free.pop_front();

	return idx;
}

uint32_t StMeshMgr::FindStMeshInList(string& name)
{
	auto it = mesh_map.find(name);
	if(it == mesh_map.end())
		return STMESH_NULL;

	auto& handle = mesh_array[it->second];
	handle.refcount++;
	return it->second;
}

void StMeshMgr::DeleteStMesh(uint32_t id)
{
	if(id == STMESH_NULL)
		return;
	
	auto& handle = mesh_array[id];

	if(handle.refcount == 1)
	{
		if(handle.mesh != null_mesh)
		{
			_CLOSE(handle.mesh);
			LOG("Static mesh droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		mesh_free.push_back(id);

		mesh_map.erase(handle.name);

		handle.name.clear();
	}
	else if(handle.refcount == 0)
	{
		WRN("Static mesh %s has already deleted!", handle.name.c_str());
	}
	else
		handle.refcount--;
}

void StMeshMgr::DeleteStMeshByName(string& name)
{
	if(name.length() == 0)
		return;
	
	auto it = mesh_map.find(name);
	if(it == mesh_map.end())
		return;

	auto& handle = mesh_array[it->second];

	if(handle.refcount == 1)
	{
		if(handle.mesh != null_mesh)
		{
			_CLOSE(handle.mesh);
			LOG("Static mesh droped %s", handle.name.c_str());
		}
		
		handle.refcount = 0;
		handle.filedate = 0;

		mesh_free.push_back(it->second);

		mesh_map.erase(name);

		handle.name.clear();
	}
	else
		handle.refcount--;
}

void StMeshMgr::UpdateStMeshes()
{
	bool something_reloaded = false;
	auto it = mesh_map.begin();
	while(it != mesh_map.end())
	{
		auto& handle = mesh_array[it->second];

		if(handle.filedate == ReloadingType::RELOAD_NOT)
		{
			it++;
			continue;
		}

		if( handle.filedate == ReloadingType::RELOAD_ONCE )
			handle.filedate = ReloadingType::RELOAD_NOT;
		else
		{
			uint32_t last_date = FileIO::GetDateModifRaw(handle.name);
			if(last_date == handle.filedate || handle.filedate == ReloadingType::RELOAD_NOT)
			{
				it++;
				continue;
			}
			handle.filedate = last_date;
		}

		ResourceProcessor::Get()->QueueLoad(it->second, ResourceType::MESH);
		
		something_reloaded = true;
		mesh_reloaded[it->second] = handle.refcount;
		it++;
	}

	if(something_reloaded) // TODO: valid bbox update
		WorldMgr::Get()->PostStMeshesReload();
}

void StMeshMgr::SaveSTMFile(string& file) 
{
	StMeshHandle nullHandle;
	nullHandle.name = file;
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(nullHandle.name, &size);
	if(!data)
		return;

	loadNoNativeFileFromMemory(nullHandle.name, data, size, true);
	_DELETE_ARRAY(data);
}

StMeshData* StMeshMgr::LoadFromFile(string& filename)
{
	StMeshHandle nullHandle;
	nullHandle.name = filename;
	uint32_t size = 0;
	uint8_t* data = FileIO::ReadFileData(nullHandle.name, &size);
	if(!data)
		return nullptr;

	LoadFromMemory(nullHandle, data, size);
	_DELETE_ARRAY(data);
	return nullHandle.mesh;
}

void StMeshMgr::LoadFromMemory(StMeshHandle& handle, uint8_t* data, uint32_t size)
{
	StMeshData* newMesh;
	if(handle.name.find(EXT_STATIC) != string::npos)
		newMesh = loadSTMFileFromMemory( handle.name, data, size );
	else
		newMesh = loadNoNativeFileFromMemory( handle.name, data, size );

	auto oldMesh = handle.mesh;
	handle.mesh = newMesh;
	if(oldMesh != null_mesh)
		_CLOSE(oldMesh);
}

StMeshData* StMeshMgr::loadSTMFileFromMemory(string& name, uint8_t* data, uint32_t size)
{
	LitVertex **vertices;
	int **indices;
		
	uint8_t* t_data = data;
	
	StMeshData* mesh = new StMeshData;

	mesh->matCount = (uint16_t)(*(int32_t*)t_data);
	t_data += sizeof(int32_t);

	vertices = new LitVertex*[mesh->matCount];
	indices = new int*[mesh->matCount];

	mesh->indexCount = new uint32_t[mesh->matCount];
	mesh->vertexCount = new uint32_t[mesh->matCount];
	mesh->vertexBuffer = new ID3D11Buffer*[mesh->matCount];
	mesh->indexBuffer = new ID3D11Buffer*[mesh->matCount];

	for(uint16_t i=0; i<mesh->matCount; i++)
	{
		mesh->vertexCount[i] = *(uint32_t*)t_data; 
		t_data += sizeof(uint32_t); //vertex count

		vertices[i] = new LitVertex[mesh->vertexCount[i]];
		for(uint32_t j=0; j<mesh->vertexCount[i]; j++)
		{
			vertices[i][j] = *(LitVertex*)t_data;
			t_data += sizeof(LitVertex);
		}

		mesh->indexCount[i] = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t); //index count

		indices[i] = new int[mesh->indexCount[i]];
		for(uint32_t j=0; j<mesh->indexCount[i]; j++)
		{
			indices[i][j] = *(uint32_t*)t_data;
			t_data += sizeof(uint32_t);
		}
	}

	XMFLOAT3 center;
	//							temp: REMOVE
	float radius;
	center = *(XMFLOAT3*)t_data;
	t_data += sizeof(XMFLOAT3); 

	radius = *(float*)t_data;
	t_data += sizeof(float);
	//mesh->sphere = BoundingSphere(center, radius);
	
	XMFLOAT3 extents;
	center = *(XMFLOAT3*)t_data;
	t_data += sizeof(XMFLOAT3); 

	extents = *(XMFLOAT3*)t_data;
	t_data += sizeof(XMFLOAT3); 

	mesh->box = BoundingBox(center, extents);
	
	for(int i=0; i<mesh->matCount; i++)
	{
		mesh->vertexBuffer[i] = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LitVertex)*mesh->vertexCount[i], false, vertices[i]);
		if (!mesh->vertexBuffer[i])
		{
			ERR("Cant init static mesh vertex buffer for %s", name.c_str());
			_CLOSE(mesh);
			return nullptr;
		}

		mesh->indexBuffer[i] = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t)*mesh->indexCount[i], false, indices[i]);
		if (!mesh->indexBuffer[i])
		{
			ERR("Cant init static mesh index buffer for %s", name.c_str());
			_CLOSE(mesh);
			return nullptr;
		}
	}
	
	for(int i = mesh->matCount - 1; i >= 0; i--)
	{
		_DELETE_ARRAY(vertices[i]);
		_DELETE_ARRAY(indices[i]);
	}
	_DELETE_ARRAY(vertices);
	_DELETE_ARRAY(indices);

	LOG("Static mesh(.stm) loaded %s", name.c_str());
	return mesh;
}

bool StMeshMgr::IsSupported(string filename)
{
	string extension = filename.substr(filename.rfind('.'));
	return instance->m_importer.IsExtensionSupported(extension);
}

StMeshData* StMeshMgr::loadNoNativeFileFromMemory(string& name, uint8_t* data, uint32_t size, bool onlyConvert)
{
	string extension = name.substr(name.rfind('.'));

	if( !m_importer.IsExtensionSupported(extension) )
	{
		ERR("Extension %s is not supported for statics", extension.data());
		return nullptr;
	}

	const aiScene* scene = m_importer.ReadFileFromMemory( data, size, 
        aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices);

	if(!scene)
	{
		ERR("Import failed for static model %s", name.c_str());
		return nullptr;
	}

	StMeshData* mesh = loadAIScene(name, scene, onlyConvert, onlyConvert);
	m_importer.FreeScene();

	if(onlyConvert)
	{
		LOG("Static mesh %s converted to STM", name.c_str());
		_CLOSE(mesh);
		return nullptr;
	}
	
	LOG("Static mesh %s loaded", name.c_str());
	return mesh;
}

StMeshData* StMeshMgr::loadAIScene(string& filename, const aiScene* scene, bool convert, bool noInit)
{
	StMeshData* stmesh = new StMeshData;

	aiMesh** mesh = scene->mMeshes;
	stmesh->matCount = scene->mNumMeshes;

	stmesh->indexCount = new uint32_t[stmesh->matCount];
	stmesh->vertexCount = new uint32_t[stmesh->matCount];
	if(!noInit)
	{
		stmesh->vertexBuffer = new ID3D11Buffer*[stmesh->matCount];
		stmesh->indexBuffer = new ID3D11Buffer*[stmesh->matCount];
	}
	uint32_t** indices = new uint32_t*[stmesh->matCount];
	LitVertex** vertices = new LitVertex*[stmesh->matCount];

	XMFLOAT3 posMin = XMFLOAT3(9999999.0f,9999999.0f,9999999.0f);
	XMFLOAT3 posMax = XMFLOAT3(-9999999.0f,-9999999.0f,-9999999.0f);

	for(uint32_t i=0; i<stmesh->matCount; i++)
	{
		stmesh->indexCount[i] = 0;
		stmesh->vertexCount[i] = 0;
		if(!noInit)
		{
			stmesh->indexBuffer[i] = nullptr;
			stmesh->vertexBuffer[i] = nullptr;
		}
		indices[i] = nullptr;
		vertices[i] = nullptr;

		if(mesh[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		stmesh->indexCount[i] = mesh[i]->mNumFaces * 3;

		indices[i] = new uint32_t[stmesh->indexCount[i]];
		uint32_t k = 0;
		for(uint32_t j=0; j<mesh[i]->mNumFaces; j++)
		{
			for(uint32_t q=0; q<3; q++)
				indices[i][k+q] = mesh[i]->mFaces[j].mIndices[q];
			k += 3;
		}

		if(!noInit)
		{
			stmesh->indexBuffer[i] = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * stmesh->indexCount[i], false, indices[i]);
			
			if (!stmesh->indexBuffer[i])
			{
				ERR("Cant init static mesh index buffer for %s", mesh[i]->mName.C_Str());
				stmesh->indexCount[i] = 0;
				continue;
			}
		}

		stmesh->vertexCount[i] = mesh[i]->mNumVertices;
		vertices[i] = new LitVertex[stmesh->vertexCount[i]];

		for(uint32_t j=0; j<stmesh->vertexCount[i]; j++)
		{
			vertices[i][j].Pos = XMFLOAT3(mesh[i]->mVertices[j].x, mesh[i]->mVertices[j].y, mesh[i]->mVertices[j].z);

			posMin.x = min(vertices[i][j].Pos.x, posMin.x);
			posMin.y = min(vertices[i][j].Pos.y, posMin.y);
			posMin.z = min(vertices[i][j].Pos.z, posMin.z);

			posMax.x = max(vertices[i][j].Pos.x, posMax.x);
			posMax.y = max(vertices[i][j].Pos.y, posMax.y);
			posMax.z = max(vertices[i][j].Pos.z, posMax.z);

			if(mesh[i]->mNormals)
				vertices[i][j].Norm = XMFLOAT3(mesh[i]->mNormals[j].x, mesh[i]->mNormals[j].y, mesh[i]->mNormals[j].z);
			else
				vertices[i][j].Norm = XMFLOAT3(0,0,0);

			if(mesh[i]->mTangents)
			{
				vertices[i][j].Tang = XMFLOAT3(mesh[i]->mTangents[j].x, mesh[i]->mTangents[j].y, mesh[i]->mTangents[j].z);
				vertices[i][j].Binorm = XMFLOAT3(mesh[i]->mBitangents[j].x, mesh[i]->mBitangents[j].y, mesh[i]->mBitangents[j].z);
			}
			else
			{
				vertices[i][j].Tang = XMFLOAT3(0,0,0);
				vertices[i][j].Binorm = XMFLOAT3(0,0,0);
			}

			if(mesh[i]->HasTextureCoords(0))
				vertices[i][j].Tex = XMFLOAT2(mesh[i]->mTextureCoords[0][j].x, mesh[i]->mTextureCoords[0][j].y);
			else
				vertices[i][j].Tex = XMFLOAT2(0.5f, 0.5f);
		}

		if(noInit)
			continue;
			
		stmesh->vertexBuffer[i] = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LitVertex) * stmesh->vertexCount[i], false, vertices[i]);
		
		if (!stmesh->vertexBuffer[i])
		{
			ERR("Cant init static mesh vertex buffer for %s", mesh[i]->mName.C_Str());
			stmesh->indexCount[i] = 0;
			stmesh->vertexCount[i] = 0;
			_RELEASE(stmesh->indexBuffer[i]);
			continue;
		}
	}

	XMFLOAT3 center = XMFLOAT3(0.5f * (posMin.x + posMax.x), 0.5f * (posMin.y + posMax.y), 0.5f * (posMin.z + posMax.z));
	XMFLOAT3 extents = XMFLOAT3(posMax.x - center.x, posMax.y - center.y, posMax.z - center.z);
	extents.x = max(extents.x, 0);
	extents.y = max(extents.y, 0);
	extents.z = max(extents.z, 0);
	stmesh->box = BoundingBox(center, extents);

	if(convert)
		convertToSTM(filename, stmesh, indices, vertices);

	for(uint32_t i = 0; i < stmesh->matCount; i++)
	{
		if(indices)
			_DELETE_ARRAY(indices[i]);
		if(vertices)
			_DELETE_ARRAY(vertices[i]);
	}
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(vertices);

	return stmesh;
}

void StMeshMgr::convertToSTM(string& filename, StMeshData* mesh, uint32_t** indices, LitVertex** vertices)
{
	string stm_file = filename.substr(0, filename.rfind('.')) + EXT_STATIC;

	uint32_t file_size = sizeof(int32_t) + sizeof(XMFLOAT3) + sizeof(float) + sizeof(XMFLOAT3) + sizeof(XMFLOAT3);
	for(uint16_t i = 0; i < mesh->matCount; i++)
	{
		file_size += sizeof(uint32_t) + sizeof(uint32_t);
		file_size += mesh->vertexCount[i] * sizeof(LitVertex);
		file_size += mesh->indexCount[i] * sizeof(uint32_t);
	}

	unique_ptr<uint8_t> data(new uint8_t[file_size]);
	uint8_t* t_data = data.get();

	*(int32_t*)t_data = (int32_t)mesh->matCount;
	t_data += sizeof(int32_t);

	for(uint16_t i = 0; i < mesh->matCount; i++)
	{
		*(uint32_t*)t_data = mesh->vertexCount[i];
		t_data += sizeof(uint32_t);

		if(vertices[i])
		{
			uint32_t size = mesh->vertexCount[i] * sizeof(LitVertex);
			memcpy(t_data, vertices[i], size);
			t_data += size;
		}

		*(uint32_t*)t_data = mesh->indexCount[i];
		t_data += sizeof(uint32_t);

		if(indices[i])
		{
			uint32_t size = mesh->indexCount[i] * sizeof(uint32_t);
			memcpy(t_data, indices[i], size);
			t_data += size;
		}
	}

	// temp
	*(XMFLOAT3*)t_data = XMFLOAT3(0,0,0);
	t_data += sizeof(XMFLOAT3);
	*(float*)t_data = 0;
	t_data += sizeof(float);
	
	*(XMFLOAT3*)t_data = mesh->box.Center;
	t_data += sizeof(XMFLOAT3);
	*(XMFLOAT3*)t_data = mesh->box.Extents;
	t_data += sizeof(XMFLOAT3);

	if(!FileIO::WriteFileData( stm_file, data.get(), file_size ))
	{
		ERR("Cant write STM file: %s", stm_file.c_str() );
	}
}