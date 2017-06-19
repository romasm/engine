#pragma once

#include "Pathes.h"
#include "FileIO.h"

namespace EngineCore
{
#define CONFIG(type, name) EngineSettings::get_##type(#name)

	class EngineSettings
	{
	public:
		EngineSettings()
		{
			if(instance)
			{
				ERR("Only one instance of EngineSettings is allowed!");
				return;
			}
			
			instance = this;

			cfgFile = nullptr;
			cfgMap = nullptr;
			ReadConfigFile(PATH_ENGINE_CONFIG);
		}

		~EngineSettings()
		{
			_DELETE(cfgFile);
			instance = nullptr;
		}

		static EngineSettings* Get() {return instance;}
		
		static bool get_bool(string name)
		{return instance->cfgFile->ReadBool(name, instance->cfgMap);}
		static int32_t get_int(string name)
		{return instance->cfgFile->ReadInt(name, instance->cfgMap);}
		static float get_float(string name)
		{return instance->cfgFile->ReadFloat(name, instance->cfgMap);}
		static string get_string(string name)
		{return instance->cfgFile->ReadString(name, instance->cfgMap);}

		static void set_bool(string name, bool value)
		{instance->cfgFile->WriteBool(name, value, instance->cfgMap);}
		static void set_int(string name, int32_t value)
		{instance->cfgFile->WriteInt(name, value, instance->cfgMap);}
		static void set_float(string name, float value)
		{instance->cfgFile->WriteFloat(name, value, instance->cfgMap);}
		static void set_string(string name, string value)
		{instance->cfgFile->WriteString(name, value, instance->cfgMap);}

		static void Save()
		{instance->cfgFile->Save();}

		static void RegLuaFunctions()
		{
			getGlobalNamespace(LSTATE)
				.beginNamespace("Config")
				.addFunction("GetBool", &EngineSettings::get_bool)
				.addFunction("GetInt", &EngineSettings::get_int)
				.addFunction("GetFloat", &EngineSettings::get_float)
				.addFunction("GetString", &EngineSettings::get_string)
				.addFunction("SetBool", &EngineSettings::set_bool)
				.addFunction("SetInt", &EngineSettings::set_int)
				.addFunction("SetFloat", &EngineSettings::set_float)
				.addFunction("SetString", &EngineSettings::set_string)
				.addFunction("SaveConfigs", &EngineSettings::Save)
				.endNamespace();
		}

	private:
		static EngineSettings* instance;
		FileIO* cfgFile;
		FileIO::file_map* cfgMap;

		void ReadConfigFile(string filename)
		{
			cfgFile = new FileIO(filename);
			auto root = cfgFile->Root();
			if(!root)
				return;
			cfgMap = cfgFile->Node("config", root);
		}
	};
}