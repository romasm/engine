#pragma once

#include "Render.h"
#include "Material.h"

#define FONT_SHADER_DEFAULT PATH_SHADERS "gui/font_default"

namespace EngineCore
{
	class Font
	{
	private:
		struct CharDesc
		{
			CharDesc() : srcX(0), srcY(0), srcW(0), srcH(0), xOff(0), yOff(0), xAdv(0) {}

			short srcX;
			short srcY;
			short srcW;
			short srcH;
			short xOff;
			short yOff;
			short xAdv;
		};	

	public:
		Font(string& filename);
		~Font()	{TEXTURE_DROP(texture);}

		void BuildVertexArray(UnlitVertex *vert, uint16_t numvert, wstring& sentence, int16_t* symbols_pos = nullptr, uint16_t* letter_count = nullptr);
		
		inline void SetTexture(uint16_t i = 0){Render::PSSetShaderTexture(i, texture);}
		inline uint16_t GetLineH() const {return lineH;}
		inline string& GetName() {return fontName;}

	private:
		string fontName;

		bool parse(string& filename);
					
		uint32_t texture;

		uint16_t texWidth;
		uint16_t texHeight;
		
		unordered_map<uint16_t, CharDesc> symbols;

		uint16_t lineH;
	};
}