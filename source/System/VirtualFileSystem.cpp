#include "stdafx.h"
#include "VirtualFileSystem.h"
#include "FileIO.h"
#include "Util.h"

using namespace EngineCore;

Package::Package()
{
	packedData = nullptr;
	packedSize = 0;
	fileDataOffset = 0;
	unwrapped = true;
	onlyTableOfContent = true;
}

Package::Package(string& path)
{
	packedData = nullptr;
	packedSize = 0;
	fileDataOffset = 0;
	unwrapped = true;
	onlyTableOfContent = true;

	ReadFromDisk(path, true);
}

Package::~Package()
{
	Clear();
}

void Package::CalcDataSize(uint32_t* size, unordered_map<string, HNode>* content)
{
	for(auto& it: *content)
	{
		if(it.second.file.date == 0)
			continue;

		if(it.second.content)
			CalcDataSize(size, it.second.content);
		else
			size += it.second.file.size;
	}
}

void Package::MoveData(uint8_t* newData, uint32_t newSize, uint32_t* dataOffset, unordered_map<string, HNode>* content)
{
	for(auto& it: *content)
	{
		if(it.second.file.date == 0)
			continue;

		if(it.second.content)
		{
			MoveData(newData, newSize, dataOffset, it.second.content);
			continue;
		}

		if( *dataOffset + it.second.file.size > newSize )
		{
			ERR("Package %s buffer overflow!", pkgName.data());
			return;
		}

		if(it.second.file.dataPointer)
		{
			memcpy(newData + *dataOffset, it.second.file.dataPointer, it.second.file.size);
			_DELETE_ARRAY(it.second.file.dataPointer);
		}
		else
		{
			memcpy(newData + *dataOffset, packedData + it.second.file.offset, it.second.file.size);
		}

		it.second.file.offset = *dataOffset;
		*dataOffset += it.second.file.size;
	}
}

void Package::ClearDeleted(unordered_map<string, HNode>* content)
{
	for(auto& it: *content)
	{
		if(it.second.file.date == 0)
		{
			ClearNode(&it.second);
		}
		else
		{
			if(it.second.content)
				ClearDeleted(it.second.content);
		}
	}
}

void Package::WrapUp()
{
	if(!unwrapped)
		return;

	if(onlyTableOfContent)
	{
		ERR("Only fully loaded packages can be modified");
		return;
	}

	uint32_t newSize = packedSize;
	CalcDataSize(&newSize, &pkgMap);
	if( newSize == 0 )
	{
		LOG("No content in package %s", pkgName.data());
		Clear();
		return;
	}

	uint8_t* newData = new uint8_t[newSize];
	uint32_t dataOffset = 0;
	MoveData(newData, newSize, &dataOffset, &pkgMap);

	if( dataOffset != newSize )
		WRN("Something went wrong during data wraping up for %s", pkgName.data());

	_DELETE_ARRAY(packedData);
	packedData = newData;
	packedSize = newSize;

	ClearDeleted(&pkgMap);

	unwrapped = false;
}

void Package::ClearNode(HNode* node)
{
	_DELETE_ARRAY(node->file.dataPointer);
	if(!node->content)
		return;

	for(auto& it: *(node->content))
	{
		ClearNode(&it.second);
	}
	_DELETE(node->content);
}

void Package::Clear()
{
	for(auto& it: pkgMap)
		ClearNode(&it.second);

	_DELETE_ARRAY(packedData);
	packedSize = 0;

	pkgMap.clear();
	unwrapped = true;
}

void Package::ReadNode(uint8_t** t_data, unordered_map<string, HNode>* content)
{
	uint32_t nodesCount = *(uint32_t*)(*t_data);
	*t_data += sizeof(uint32_t);

	for(uint32_t i = 0; i < nodesCount; i++)
	{
		uint32_t stringSize = *(uint32_t*)(*t_data);
		*t_data += sizeof(uint32_t);
		string nodeName((char*)(*t_data), stringSize);
		*t_data += sizeof(char) * stringSize;

		HNode node;
		node.file.date = *(uint32_t*)(*t_data);
		*t_data += sizeof(uint32_t);
		node.file.offset = *(uint32_t*)(*t_data);
		*t_data += sizeof(uint32_t);
		node.file.size = *(uint32_t*)(*t_data);
		*t_data += sizeof(uint32_t);

		if(node.file.size == 0)
		{
			node.content = new unordered_map<string, HNode>;
			ReadNode(t_data, node.content);
		}

		content->insert(make_pair(nodeName, node));
	}
}

bool Package::ReadFromDisk(string& path, bool fullLoad) // TODO: only table read from file
{
	if( GetExtension(path) != EXT_PACKAGE )
	{
		ERR("Package file %s has wrong extension", path.c_str());
		return false;
	}

	Clear();

	uint32_t size;
	uint8_t* data = FileIO::ReadFileData(path, &size);
	if(!data)
		return false;

	uint8_t* t_data = data;

	if( (*(uint32_t*)t_data) != PACKAGE_FILE_VERSION )
	{
		ERR("Package file %s has wrong version!", path.c_str());
		_DELETE_ARRAY(data);
		return false;
	}
	t_data += sizeof(uint32_t);

	ReadNode(&t_data, &pkgMap);

	fileDataOffset = *(uint32_t*)t_data;
	t_data += sizeof(uint32_t);

	if(fullLoad)
	{
		packedSize = *(uint32_t*)t_data;
		t_data += sizeof(uint32_t);

		packedData = new uint8_t[packedSize];
		memcpy(packedData, t_data, packedSize);
	}

	_DELETE_ARRAY(data);

	pkgPath = path;
	pkgName = RemoveExtension(GetFilename(path));
	unwrapped = false;
	onlyTableOfContent = !fullLoad;

	LOG_GOOD("Package %s loaded from disk", path.data());
	return true;
}

void Package::CalcNodeSize(uint32_t* size, unordered_map<string, HNode>* content)
{
	*size += sizeof(uint32_t);
	for(auto& it: *content)
	{
		if( it.second.file.size == 0 && !it.second.content )
		{
			ERR("Zero size files is not supported for package %s", pkgName.data());
			continue;
		}

		if( it.second.file.size != 0 && it.second.content )
		{
			WRN("Directory in %s cant be file at same time, file part is ignoring", pkgName.data());
		}

		size += sizeof(uint32_t) + (uint32_t)it.first.size() * sizeof(char);
		size += sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
		
		if(it.second.content)
		{
			CalcNodeSize(size, it.second.content);
		}
	}
}

void Package::WriteNode(uint8_t** t_data, unordered_map<string, HNode>* content)
{
	*(uint32_t*)(*t_data) = (uint32_t)content->size();
	*t_data += sizeof(uint32_t);

	for(auto& it: *content)
	{
		if( it.second.file.size == 0 && !it.second.content )
			continue;

		*(uint32_t*)(*t_data) = (uint32_t)it.first.size();
		*t_data += sizeof(uint32_t);

		memcpy(*t_data, it.first.data(), it.first.size());
		*t_data += it.first.size() * sizeof(char);

		if( !it.second.content )
		{
			*(uint32_t*)(*t_data) = it.second.file.date;
			*t_data += sizeof(uint32_t);
			*(uint32_t*)(*t_data) = it.second.file.offset;
			*t_data += sizeof(uint32_t);
			*(uint32_t*)(*t_data) = it.second.file.size;
			*t_data += sizeof(uint32_t);
		}
		else
		{
			*(uint32_t*)(*t_data) = 0;
			*t_data += sizeof(uint32_t);
			*(uint32_t*)(*t_data) = 0;
			*t_data += sizeof(uint32_t);
			*(uint32_t*)(*t_data) = 0;
			*t_data += sizeof(uint32_t);

			WriteNode(t_data, it.second.content);
		}
	}
}

bool Package::WriteToDisk()
{
	if(pkgPath.empty())
	{
		ERR("Path must be specified for %s package before writing on disk", pkgName.data());
		return false;
	}

	WrapUp();

	if(IsEmpty())
		return false;

	uint32_t fileSize = sizeof(uint32_t);
	CalcNodeSize(&fileSize, &pkgMap);
	fileSize += sizeof(uint32_t) + sizeof(uint32_t);
	
	fileDataOffset = fileSize;
	fileSize += packedSize;

	uint8_t* data = new uint8_t[fileSize];
	uint8_t* t_data = data;

	*(uint32_t*)t_data = PACKAGE_FILE_VERSION;
	t_data += sizeof(uint32_t);
	
	WriteNode(&t_data, &pkgMap);

	*(uint32_t*)t_data = fileDataOffset;
	t_data += sizeof(uint32_t);

	*(uint32_t*)t_data = packedSize;
	t_data += sizeof(uint32_t);
	
	memcpy(t_data, packedData, packedSize);
	t_data += packedSize;

	bool status;
	if( !(status = FileIO::WriteFileData(pkgPath, data, fileSize)) )
	{
		ERR("Cant write package file %s", pkgPath.c_str() );
	}
	_DELETE_ARRAY(data);
	return status;
}

Package::HNode* Package::GetNode(string& path)
{
	string dir;
	string subpath = path;
	unordered_map<string, HNode>* content = &pkgMap;

	while( DivideString(subpath, dir, subpath) )
	{
		auto node = content->find(dir);
		if(node == content->end())
			return nullptr;

		content = node->second.content;
		if(!content)
			return nullptr;
	};

	auto node = content->find(dir);
	if(node == content->end())
		return nullptr;

	return &node->second;
}

bool Package::AddContent(string& path, uint8_t* data, uint32_t size)
{
	string dir;
	string subpath = path;
	unordered_map<string, HNode>* content = &pkgMap;

	__time32_t timer;
	_time32(&timer);

	while( DivideString(subpath, dir, subpath) )
	{
		auto node = content->find(dir);
		if(node != content->end())
			continue;

		HNode newNode;
		newNode.file.date = uint32_t(timer);
		newNode.content = new unordered_map<string, HNode>;
		content->insert(make_pair(dir, newNode));
	};

	auto node = content->find(dir);
	if(node != content->end())
	{
		if(!data)
			return true;

		// TODO: overwrite deleted content
		return false;
	}

	HNode newNode;
	newNode.file.date = uint32_t(timer);
	if(!data)
	{
		newNode.content = new unordered_map<string, HNode>;
	}
	else
	{
		newNode.file.size = size;
		newNode.file.dataPointer = new uint8_t[size];
		memcpy(newNode.file.dataPointer, data, size);
	}
	content->insert(make_pair(dir, newNode));

	unwrapped = true;
	return true;
}

bool Package::DeleteContent(string& path)
{
	auto node = GetNode(path);
	if(!node)
		return false;

	node->file.date = 0;
	unwrapped = true;
}

uint8_t* Package::GetFile(string& path, uint32_t* size)
{
	auto node = GetNode(path);
	if(!node)
		return nullptr;

	*size = node->file.size;

	if(node->file.dataPointer)
		return node->file.dataPointer;

	if(packedData)
		return packedData + node->file.offset;

	// TODO: read from disk
	return nullptr;
}

bool Package::IsExist(string& path)
{
	auto node = GetNode(path);
	if(!node)
		return false;
	return true;
}

uint32_t Package::GetDateModifRaw(string& path)
{
	auto node = GetNode(path);
	if(!node)
		return 0;
	return node->file.date;
}