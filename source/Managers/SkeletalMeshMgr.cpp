#include "stdafx.h"
#include "SkeletalMeshMgr.h"
#include "Common.h"
#include "Render.h"
#include "Buffer.h"
#include "Material.h"
#include "WorldMgr.h"

using namespace EngineCore;

SkeletalMeshMgr* SkeletalMeshMgr::instance = nullptr;
SkeletalMeshData* SkeletalMeshMgr::null_mesh = nullptr;
string SkeletalMeshMgr::null_name = "";

SkeletalMeshMgr::SkeletalMeshMgr()
{
	if(!instance)
	{
		instance = this;

		mesh_array.resize(SKELETAL_MESH_MAX_COUNT);
		mesh_free.resize(SKELETAL_MESH_MAX_COUNT);
		for(uint32_t i=0; i<SKELETAL_MESH_MAX_COUNT; i++)
			mesh_free[i] = i;
		mesh_map.reserve(SKELETAL_MESH_INIT_COUNT);
		mesh_reloaded.resize(SKELETAL_MESH_MAX_COUNT);
		mesh_reloaded.assign(0);
		
		null_mesh = loadSTMFile(string(PATH_SKELETAL_MESH_NULL));
	}
	else
		ERR("Only one instance of SkeletalMeshMgr is allowed!");
}

SkeletalMeshMgr::~SkeletalMeshMgr()
{
	for(uint32_t i=0; i<SKELETAL_MESH_MAX_COUNT; i++)
	{
		_CLOSE(mesh_array[i].mesh);
		mesh_array[i].name.clear();
	}
	_CLOSE(null_mesh);
	null_name.clear();

	instance = nullptr;
}

void SkeletalMeshMgr::PreloadSkeletalMeshes()
{
	bool reload = false;
#ifdef _DEV
	reload = true;
#endif

	GetSkeletalMesh(string(ENV_MESH), reload);
	GetSkeletalMesh(string(MAT_MESH), reload);
}

uint32_t SkeletalMeshMgr::GetSkeletalMesh(string& name, bool reload)
{
	uint32_t res = SKELETAL_MESH_NULL;
	if(name.length() == 0)
		return res;

	res = FindSkeletalMeshInList(name);
	if(res != SKELETAL_MESH_NULL)
		return res;

	res = AddSkeletalMeshToList(name, reload);
	if(res != SKELETAL_MESH_NULL)
		return res;

	ERR("Cant load static mesh %s", name.c_str());

	return res;
}

uint32_t SkeletalMeshMgr::AddSkeletalMeshToList(string& name, bool reload)
{
	if(mesh_free.size() == 0)
	{
		ERR("Static meshes resources amount overflow!");
		return SKELETAL_MESH_NULL;
	}

	uint32_t idx = mesh_free.front();
	auto& handle = mesh_array[idx];
	
	if(!FileIO::IsExist(name))
		return SKELETAL_MESH_NULL;

	if(name.find(EXT_STATIC) != string::npos)
		handle.mesh = loadSTMFile( name );
	else
		handle.mesh = loadNoNativeFile( name );
	if(handle.mesh->matCount == 0)
		return SKELETAL_MESH_NULL;
	//handle.mesh = null_mesh;
	
	handle.name = name;
	handle.refcount = 1;

	if(reload)
		handle.filedate = FileIO::GetDateModifRaw(name);
	else
		handle.filedate = NOT_RELOAD;
	/*if(reload)
		handle.filedate = NEED_RELOADING;
	else
		handle.filedate = NEED_LOADING_ONCE;*/

	mesh_map.insert(make_pair(name, idx));
	mesh_free.pop_front();

	return idx;
}

uint32_t SkeletalMeshMgr::FindSkeletalMeshInList(string& name)
{
	auto it = mesh_map.find(name);
	if(it == mesh_map.end())
		return SKELETAL_MESH_NULL;

	auto& handle = mesh_array[it->second];
	handle.refcount++;
	return it->second;
}

void SkeletalMeshMgr::DeleteSkeletalMesh(uint32_t id)
{
	if(id == SKELETAL_MESH_NULL)
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

void SkeletalMeshMgr::DeleteSkeletalMeshByName(string& name)
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

void SkeletalMeshMgr::UpdateSkeletalMeshes()
{
	bool something_reloaded = false;
	for(auto& it: mesh_map)
	{
		auto& handle = mesh_array[it.second];

		if(handle.filedate == NOT_RELOAD)
			continue;

		if(handle.filedate == NEED_LOADING_ONCE)
			handle.filedate = NOT_RELOAD;
		else
		{
			uint32_t last_date = FileIO::GetDateModifRaw(handle.name);
			if(last_date == handle.filedate || handle.filedate == NOT_RELOAD)
				continue;
			handle.filedate = last_date;
		}

		SkeletalMeshData* newMesh;
		if(handle.name.find(EXT_STATIC) != string::npos)
			newMesh = loadSTMFile( handle.name );
		else
			newMesh = loadNoNativeFile( handle.name );
		if(!newMesh)
			continue;

		auto oldMesh = handle.mesh;
		handle.mesh = newMesh;
		if(oldMesh != null_mesh)
			_CLOSE(oldMesh);

		something_reloaded = true;
		mesh_reloaded[it.second] = handle.refcount;
	}

	if(something_reloaded)
		WorldMgr::Get()->PostSkeletalMeshesReload();
}

SkeletalMeshData* SkeletalMeshMgr::loadSTMFile(string& name)
{
	ifstream fin;
	LitVertex **vertices;
	int **indices;

	SkeletalMeshData* SkeletalMesh = new SkeletalMeshData();

	fin.open(name, std::ios::binary);
	if(!fin.is_open())
	{
		ERR("Cant load static mesh STM file %s", name.c_str());
		_CLOSE(SkeletalMesh);
		return nullptr;
	}

	fin.read( (char*)&(SkeletalMesh->matCount), sizeof(int32_t) );
	vertices = new LitVertex*[SkeletalMesh->matCount];
	indices = new int*[SkeletalMesh->matCount];

	SkeletalMesh->indexCount = new uint32_t[SkeletalMesh->matCount];
	SkeletalMesh->vertexCount = new uint32_t[SkeletalMesh->matCount];
	SkeletalMesh->vertexBuffer = new ID3D11Buffer*[SkeletalMesh->matCount];
	SkeletalMesh->indexBuffer = new ID3D11Buffer*[SkeletalMesh->matCount];

	for(uint16_t i=0; i<SkeletalMesh->matCount; i++)
	{
		fin.read( (char*)&(SkeletalMesh->vertexCount[i]), sizeof(uint32_t) ); //vertex count
		vertices[i] = new LitVertex[SkeletalMesh->vertexCount[i]];
		for(uint32_t j=0; j<SkeletalMesh->vertexCount[i]; j++)
		{
			fin.read( (char*)&(vertices[i][j]), sizeof(LitVertex) );
		}

		fin.read( (char*)&(SkeletalMesh->indexCount[i]), sizeof(uint32_t) ); //index count
		indices[i] = new int[SkeletalMesh->indexCount[i]];
		for(uint32_t j=0; j<SkeletalMesh->indexCount[i]; j++)
		{
			fin.read( (char*)&(indices[i][j]), sizeof(uint32_t) );
		}
	}

	Vector3 center;
	//							temp: REMOVE
	float radius;
	fin.read( (char*)&(center), sizeof(Vector3) ); 
	fin.read( (char*)&(radius), sizeof(float) ); 
	//SkeletalMesh->sphere = BoundingSphere(center, radius);
	
	Vector3 extents;
	fin.read( (char*)&(center), sizeof(Vector3) ); 
	fin.read( (char*)&(extents), sizeof(Vector3) );
	SkeletalMesh->box = BoundingBox(center, extents);

	fin.close();

	for(int i=0; i<SkeletalMesh->matCount; i++)
	{
		SkeletalMesh->vertexBuffer[i] = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LitVertex)*SkeletalMesh->vertexCount[i], false, vertices[i]);
		if (!SkeletalMesh->vertexBuffer[i])
		{
			ERR("Cant init static mesh vertex buffer for %s", name.c_str());
			_CLOSE(SkeletalMesh);
			return nullptr;
		}

		SkeletalMesh->indexBuffer[i] = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t)*SkeletalMesh->indexCount[i], false, indices[i]);
		if (!SkeletalMesh->indexBuffer[i])
		{
			ERR("Cant init static mesh index buffer for %s", name.c_str());
			_CLOSE(SkeletalMesh);
			return nullptr;
		}
	}
	
	for(int i = SkeletalMesh->matCount - 1; i >= 0; i--)
	{
		_DELETE_ARRAY(vertices[i]);
		_DELETE_ARRAY(indices[i]);
	}
	_DELETE_ARRAY(vertices);
	_DELETE_ARRAY(indices);

	LOG("Static mesh(.stm) loaded %s", name.c_str());

	return SkeletalMesh;
}

SkeletalMeshData* SkeletalMeshMgr::loadNoNativeFile(string& filename, bool onlyConvert)
{
	string extension = filename.substr(filename.rfind('.'));

	if( !m_importer.IsExtensionSupported(extension) )
	{
		ERR("Extension %s is not supported for skeletals", extension.data());
		return nullptr;
	}

	const aiScene* scene = m_importer.ReadFile( filename, 
        aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded | aiProcess_LimitBoneWeights);

	if(!scene)
	{
		ERR("Import failed for skeletal mesh %s", filename.c_str());
		return nullptr;
	}

	SkeletalMeshData* SkeletalMesh = loadAIScene(filename, scene, onlyConvert, onlyConvert);
	m_importer.FreeScene();

	if(onlyConvert)
	{
		LOG("Skeletal mesh %s converted to STM", filename.c_str());
		_CLOSE(SkeletalMesh);
		return nullptr;
	}
	else
	{
		LOG("Skeletal mesh %s loaded", filename.c_str());
		return SkeletalMesh;
	}
	return nullptr;
}

SkeletalMeshData* SkeletalMeshMgr::loadAIScene(string& filename, const aiScene* scene, bool convert, bool noInit)
{
	SkeletalMeshData* SkeletalMesh = new SkeletalMeshData;

	aiMesh** mesh = scene->mMeshes;
	SkeletalMesh->matCount = scene->mNumMeshes;

	SkeletalMesh->indexCount = new uint32_t[SkeletalMesh->matCount];
	SkeletalMesh->vertexCount = new uint32_t[SkeletalMesh->matCount];
	if(!noInit)
	{
		SkeletalMesh->vertexBuffer = new ID3D11Buffer*[SkeletalMesh->matCount];
		SkeletalMesh->indexBuffer = new ID3D11Buffer*[SkeletalMesh->matCount];
	}
	uint32_t** indices = new uint32_t*[SkeletalMesh->matCount];
	LitVertex** vertices = new LitVertex*[SkeletalMesh->matCount];

	Vector3 posMin = Vector3(9999999.0f,9999999.0f,9999999.0f);
	Vector3 posMax = Vector3(-9999999.0f,-9999999.0f,-9999999.0f);

	for(uint32_t i=0; i<SkeletalMesh->matCount; i++)
	{
		SkeletalMesh->indexCount[i] = 0;
		SkeletalMesh->vertexCount[i] = 0;
		if(!noInit)
		{
			SkeletalMesh->indexBuffer[i] = nullptr;
			SkeletalMesh->vertexBuffer[i] = nullptr;
		}
		indices[i] = nullptr;
		vertices[i] = nullptr;

		if(mesh[i]->mPrimitiveTypes != aiPrimitiveType_TRIANGLE )
		{
			ERR("Unsupported primitive type on %s", mesh[i]->mName.C_Str());
			continue;
		}

		SkeletalMesh->indexCount[i] = mesh[i]->mNumFaces * 3;

		indices[i] = new uint32_t[SkeletalMesh->indexCount[i]];
		uint32_t k = 0;
		for(uint32_t j=0; j<mesh[i]->mNumFaces; j++)
		{
			for(uint32_t q=0; q<3; q++)
				indices[i][k+q] = mesh[i]->mFaces[j].mIndices[q];
			k += 3;
		}

		if(!noInit)
		{
			SkeletalMesh->indexBuffer[i] = Buffer::CreateIndexBuffer(Render::Device(), sizeof(uint32_t) * SkeletalMesh->indexCount[i], false, indices[i]);
			
			if (!SkeletalMesh->indexBuffer[i])
			{
				ERR("Cant init static mesh index buffer for %s", mesh[i]->mName.C_Str());
				SkeletalMesh->indexCount[i] = 0;
				continue;
			}
		}

		if(mesh[i]->mNumBones > 0)
		{
			for(uint32_t j = 0; j < mesh[i]->mNumBones; j++)
			{
				mesh[i]->mBones[j]
			}
		}

		SkeletalMesh->vertexCount[i] = mesh[i]->mNumVertices;
		vertices[i] = new LitVertex[SkeletalMesh->vertexCount[i]];

		for(uint32_t j=0; j<SkeletalMesh->vertexCount[i]; j++)
		{
			vertices[i][j].Pos = Vector3(mesh[i]->mVertices[j].x, mesh[i]->mVertices[j].y, mesh[i]->mVertices[j].z);

			posMin.x = min(vertices[i][j].Pos.x, posMin.x);
			posMin.y = min(vertices[i][j].Pos.y, posMin.y);
			posMin.z = min(vertices[i][j].Pos.z, posMin.z);

			posMax.x = max(vertices[i][j].Pos.x, posMax.x);
			posMax.y = max(vertices[i][j].Pos.y, posMax.y);
			posMax.z = max(vertices[i][j].Pos.z, posMax.z);

			if(mesh[i]->mNormals)
				vertices[i][j].Norm = Vector3(mesh[i]->mNormals[j].x, mesh[i]->mNormals[j].y, mesh[i]->mNormals[j].z);
			else
				vertices[i][j].Norm = Vector3(0,0,0);

			if(mesh[i]->mTangents)
			{
				vertices[i][j].Tang = Vector3(mesh[i]->mTangents[j].x, mesh[i]->mTangents[j].y, mesh[i]->mTangents[j].z);
				vertices[i][j].Binorm = Vector3(mesh[i]->mBitangents[j].x, mesh[i]->mBitangents[j].y, mesh[i]->mBitangents[j].z);
			}
			else
			{
				vertices[i][j].Tang = Vector3(0,0,0);
				vertices[i][j].Binorm = Vector3(0,0,0);
			}

			if(mesh[i]->HasTextureCoords(0))
				vertices[i][j].Tex = Vector2(mesh[i]->mTextureCoords[0][j].x, mesh[i]->mTextureCoords[0][j].y);
			else
				vertices[i][j].Tex = Vector2(0.5f, 0.5f);
		}

		if(noInit)
			continue;
			
		SkeletalMesh->vertexBuffer[i] = Buffer::CreateVertexBuffer(Render::Device(), sizeof(LitVertex) * SkeletalMesh->vertexCount[i], false, vertices[i]);
		
		if (!SkeletalMesh->vertexBuffer[i])
		{
			ERR("Cant init static mesh vertex buffer for %s", mesh[i]->mName.C_Str());
			SkeletalMesh->indexCount[i] = 0;
			SkeletalMesh->vertexCount[i] = 0;
			_RELEASE(SkeletalMesh->indexBuffer[i]);
			continue;
		}
	}

	Vector3 center = Vector3(0.5f * (posMin.x + posMax.x), 0.5f * (posMin.y + posMax.y), 0.5f * (posMin.z + posMax.z));
	Vector3 extents = Vector3(posMax.x - center.x, posMax.y - center.y, posMax.z - center.z);
	extents.x = max(extents.x, 0);
	extents.y = max(extents.y, 0);
	extents.z = max(extents.z, 0);
	SkeletalMesh->box = BoundingBox(center, extents);

	if(convert)
		convertToSTM(filename, SkeletalMesh, indices, vertices);

	for(uint32_t i = 0; i < SkeletalMesh->matCount; i++)
	{
		if(indices)
			_DELETE_ARRAY(indices[i]);
		if(vertices)
			_DELETE_ARRAY(vertices[i]);
	}
	_DELETE_ARRAY(indices);
	_DELETE_ARRAY(vertices);

	return SkeletalMesh;
}

void SkeletalMeshMgr::convertToSTM(string& filename, SkeletalMeshData* mesh, uint32_t** indices, LitVertex** vertices)
{
	string stm_file = filename.substr(0, filename.rfind('.')) + EXT_STATIC;

	uint32_t file_size = sizeof(int32_t) + sizeof(Vector3) + sizeof(float) + sizeof(Vector3) + sizeof(Vector3);
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
	*(Vector3*)t_data = Vector3(0,0,0);
	t_data += sizeof(Vector3);
	*(float*)t_data = 0;
	t_data += sizeof(float);
	
	*(Vector3*)t_data = mesh->box.Center;
	t_data += sizeof(Vector3);
	*(Vector3*)t_data = mesh->box.Extents;
	t_data += sizeof(Vector3);

	if(!FileIO::WriteFileData( stm_file, data.get(), file_size ))
	{
		ERR("Cant write STM file: %s", stm_file.c_str() );
	}
}