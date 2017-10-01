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

	bool ReadFromDisk(string& path, bool fullLoad);
	bool WriteToDisk();

	bool AddContent(string& path, uint8_t* data, uint32_t size);
	bool DeleteContent(string& path);

	uint8_t* GetFile(string& path, uint32_t* size);
	bool IsExist(string& path);
	uint32_t GetDateModifRaw(string& path);

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

	HNode* GetNode(string& path);

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
	bool onlyTableOfContent;

public:

	static void RegLuaFunctions()
	{
		
	}
};

}