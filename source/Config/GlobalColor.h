#pragma once

#include "Log.h"
#include "LuaVM.h"

namespace EngineCore
{
//------------------------------------------------------------------

static XMFLOAT4 null_color = XMFLOAT4(0,0,0,0);
static XMFLOAT4 black_color = XMFLOAT4(0,0,0,1);
static XMFLOAT4 white_color = XMFLOAT4(1,1,1,1);

#define GCOLOR(name) GlobalColor::Get()->GetColorPtr(name)

	class GlobalColor
	{
	public:
		GlobalColor();
		~GlobalColor();

		inline static GlobalColor* Get(){return instance;}

		void AddColor(string& id, XMFLOAT4 color);
		XMFLOAT4* GetColorPtr(string& id);
		
		static XMFLOAT4 GetColor(string id) {return *(instance->GetColorPtr(id));}
		static void SetColor(string id, XMFLOAT4 color);

		static bool Load(string filename);
		static bool Save(string filename);

		static void RegLuaFunctions()
		{
			getGlobalNamespace(LSTATE)
				.beginNamespace("Config")
				.addFunction("GetColor", &GlobalColor::GetColor)
				.addFunction("SetColor", &GlobalColor::SetColor)
				.addFunction("SaveColors", &GlobalColor::Save)
				.addFunction("LoadColors", &GlobalColor::Load)
				.endNamespace();
		}

	private:
		static GlobalColor *instance;
		bool init;
		
		map<string, XMFLOAT4> colorsMap;
	};

//------------------------------------------------------------------
}