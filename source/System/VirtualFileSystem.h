#pragma once
#include "Log.h"
#include "DataTypes.h"
#include "macros.h"
#include "Util.h"
#include "LuaVM.h"
#include "Arrays.h"

#define PACKAGE_FILE_VERSION 101

namespace EngineCore
{

class Package
{
public:
	Package();
	Package(string& path);
	~Package();

	void WrapUp();

	bool ReadFromDisk(string& path);
	bool WriteToDisk();

	bool AddFile(string& path, uint8_t* data, uint32_t size); // TODO
	bool AddDir(string& path); // TODO
	bool DeleteContent(string& path); // TODO

	bool IsEmpty() const {return packedSize == 0;};

private:

	struct FileLink
	{
		uint32_t date;	// if 0, then deleted
		uint32_t offset;
		uint32_t size;	// if 0, then directory
		uint8_t* dataPointer;

		FileLink() : date(0), offset(0), size(0), dataPointer(nullptr) {}
	};

	struct HNode
	{
		unordered_map<string, HNode>* content;
		FileLink file;

		HNode() : content(nullptr) {}
	};

	void Clear();
	void ClearNode(HNode* node);

	void ReadNode(uint8_t** t_data, unordered_map<string, HNode>* content);
	void CalcNodeSize(uint32_t* size, unordered_map<string, HNode>* content);
	void WriteNode(uint8_t** t_data, unordered_map<string, HNode>* content);

	void CalcDataSize(uint32_t* size, unordered_map<string, HNode>* content);
	void MoveData(uint8_t* newData, uint32_t newSize, uint32_t* dataOffset, unordered_map<string, HNode>* content);
	void ClearDeleted(unordered_map<string, HNode>* content);

	string pkgName;
	string pkgPath;

	unordered_map<string, HNode> pkgMap;

	uint32_t fileDataOffset; // if 0, then no file written
	uint32_t packedSize;
	uint8_t* packedData;

	bool unwrapped;
	bool fullyLoaded;

public:

	static void RegLuaFunctions()
	{
		getGlobalNamespace(LSTATE)
			.beginNamespace("FileIO")
				.addFunction("IsExist", &FileIO::IsExist_lua)
				.addFunction("IsFile", &FileIO::IsFile)

				.addFunction("CreateDir", &FileIO::CreateDir)
				.addFunction("GetDirList", &FileIO::GetDirList)

				.addFunction("Rename", &FileIO::Rename)
				.addFunction("Copy", &FileIO::Copy)
				.addFunction("CopyByExt", &FileIO::CopyByExt)
				.addFunction("Delete", &FileIO::Delete)

				.addFunction("GetFileDateModifRaw", &FileIO::GetDateModifRaw_lua)
				.addFunction("GetFileDateModif", &FileIO::GetDateModif)
				.addFunction("GetFileDateCreate", &FileIO::GetDateCreate)
				.addFunction("GetFileSize", &FileIO::GetSize)

				.beginClass<DirList>("DirList") // TODO: not safe, rework
					.addFunction("getnext", &DirList::next)
					.addFunction("reset", &DirList::reset)
					.addFunction("size", &DirList::size)
					.addFunction("destruct", &DirList::destruct)
				.endClass()
			.endNamespace();
	}
};

}