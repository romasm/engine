#pragma once

#include "Log.h"

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

		inline static GlobalColor* Get(){return m_instance;}

		void AddColor(string& id, XMFLOAT4 color);
		XMFLOAT4* GetColorPtr(string& id);

		bool LoadColorsFromFile(string& filename);
		void ClearColors();

	private:
		static GlobalColor *m_instance;
		
		map<string, XMFLOAT4> colors;
	};

//------------------------------------------------------------------
}