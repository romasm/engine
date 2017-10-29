#pragma once
#include "stdafx.h"
#include "Common.h"

namespace EngineCore
{
#define FUNC_CODE_MAP_EXT ".fcm"
#define FUNC_CODE_MAX_SIZE 1024

	class FunctionCodeMgr
	{
	public:
		FunctionCodeMgr() 
		{}

		void Add(string& func, string& code)
		{
			auto it = codeMap.find(func);
			if( it == codeMap.end() )
				codeMap.insert(make_pair(func, FuncCode(code)));
			else
				it->second.refcount++;
		}

		void Remove(string& func)
		{
			auto it = codeMap.find(func);
			if( it == codeMap.end() )
				return;
			it->second.refcount--;
			if(it->second.refcount < 1)
				codeMap.erase(it);
		}

		string Get(string& func)
		{
			auto it = codeMap.find(func);
			if( it == codeMap.end() )
				return "";
			else
				return it->second.code;
		}

		void DumpToFile(string& name)
		{
			uint32_t mapSize = (uint32_t)codeMap.size();
			uint32_t fileSize = mapSize * (FUNC_CODE_MAX_SIZE + FUNC_CODE_MAX_SIZE + sizeof(int32_t));

			unique_ptr<uint8_t> data(new uint8_t[fileSize]);
			uint8_t* t_data = data.get();
			
			*(uint32_t*)t_data = mapSize;
			t_data += sizeof(uint32_t);

			uint32_t dummySize = 0;

			for(auto& it: codeMap)
			{
				StringSerialize(it.first, &t_data, &dummySize);
				StringSerialize(it.second.code, &t_data, &dummySize);
				*(int32_t*)t_data = it.second.refcount;
				t_data += sizeof(int32_t);

				if( uint32_t(t_data - data.get()) > fileSize )
				{
					ERR("Wrong code map size estimation for %s", name.data());
					return;
				}
			}

			uint32_t totalSize = uint32_t(t_data - data.get());
			string filename = name + FUNC_CODE_MAP_EXT;

			if(!FileIO::WriteFileData( filename, data.get(), totalSize ))
				ERR("Cant write code map file %s", filename.c_str() );
		}

		void LoadFromFile(string& name)
		{
			string filename = name + FUNC_CODE_MAP_EXT;
			uint32_t size = 0;
			unique_ptr<uint8_t> data(FileIO::ReadFileData(filename, &size));
			if(!data)
			{
				ERR("Cant read code map file %s", filename.c_str() );
				return;
			}
			uint8_t* t_data = data.get();

			uint32_t codesCount = *(uint32_t*)t_data;
			t_data += sizeof(uint32_t);

			codeMap.clear();
			codeMap.reserve(codesCount);

			for(uint32_t i = 0; i < codesCount; i++)
			{
				string func = StringDeserialize(&t_data);
				string code = StringDeserialize(&t_data);

				int32_t refs = *(int32_t*)t_data;
				t_data += sizeof(int32_t);

				codeMap.insert(make_pair(func, FuncCode(code, refs)));
			}
		}

	private:
		struct FuncCode
		{
			string code;
			int32_t refcount;
			FuncCode() : refcount(0) {}
			FuncCode(string& s) : refcount(1) { code = s; }
			FuncCode(string& s, int32_t r) : refcount(r) { code = s; }
		};
		unordered_map<string, FuncCode> codeMap;
	};
}