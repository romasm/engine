#pragma once

#include "Log.h"
#include "LuaVM.h"

namespace EngineCore
{
//------------------------------------------------------------------

static Vector4 null_color = Vector4(0,0,0,0);
static Vector4 black_color = Vector4(0,0,0,1);
static Vector4 white_color = Vector4(1,1,1,1);

#define GCOLOR(name) GlobalColor::Get()->GetColorPtr(name)

	class GlobalColor
	{
	public:
		GlobalColor();
		~GlobalColor();

		inline static GlobalColor* Get(){return instance;}

		void AddColor(string& id, Vector4 color);
		Vector4* GetColorPtr(string& id);
		
		static Vector4 GetColor(string id) {return *(instance->GetColorPtr(id));}
		static void SetColor(string id, Vector4 color);

		static bool Load(string filename);
		static bool Save(string filename);

		static string GetName() {return instance->configName;};

		static void RegLuaFunctions()
		{
			getGlobalNamespace(LSTATE)
				.beginNamespace("Config")
				.addFunction("GetColor", &GlobalColor::GetColor)
				.addFunction("SetColor", &GlobalColor::SetColor)
				.addFunction("SaveColors", &GlobalColor::Save)
				.addFunction("LoadColors", &GlobalColor::Load)
				.addFunction("GetColorsPresetName", &GlobalColor::GetName)
				.endNamespace();
		}

	private:
		static GlobalColor *instance;
		bool init;
		
		string configName;

		map<string, Vector4> colorsMap;
	};

//------------------------------------------------------------------
}