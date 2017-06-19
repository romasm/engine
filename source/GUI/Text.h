#pragma once

#include "Font.h"

namespace EngineCore
{
//------------------------------------------------------------------

	class Text
	{
	public:
		Text();

		bool Init(string &font, wstring &text, string& shaderName, bool statictext = true, uint16_t length = 0, bool needSymPos = false);
		void Draw();
		void Close();

		inline void ForceUpdate() {shaderInst->ForceUpdate();}

		bool SetText(wstring &text);

		inline void SetColor(XMFLOAT4& color)
		{shaderInst->SetVector(color, 0);}
		inline void SetPos(int16_t x, int16_t y)
		{textPos = XMFLOAT2(float(x),float(y)); dirty = true;}
		
		inline void SetClip(RECT& r)
		{shaderInst->SetVector(XMFLOAT4( float(r.left), float(r.top), float(r.right), float(r.bottom) ), 1);}

		inline uint16_t GetSymbolsCount() const 
		{return symbolsCount;}
		inline uint16_t GetWidth() const 
		{
			if(!symbolsPos)return 0;
			else return symbolsPos[symbolsCount];
		}
		inline uint16_t GetHeight() const {return textFont->GetLineH();}

		inline int16_t GetLeft() const {return (int16_t)textPos.x;}
		inline int16_t GetTop() const {return (int16_t)textPos.y;}

		int16_t GetSymPos(uint16_t s) const;
		uint16_t GetClosestSym(uint16_t x);

		inline SimpleShaderInst* GetShaderInst(){return shaderInst;}
		inline void SetFloat(float f, uint8_t i){shaderInst->SetFloat(f, i);}
		inline void SetVector(XMFLOAT4 v, uint8_t i){shaderInst->SetVector(v, i);}

		inline void SetTexture(ID3D11ShaderResourceView *tex, uint8_t id){shaderInst->SetTexture(tex, id);}
		inline void SetTextureByName(string& name, uint8_t id){shaderInst->SetTextureByName(name, id);}
		inline void SetTextureByNameS(char* name, uint8_t id){shaderInst->SetTextureByName(string(name), id);}
		inline void ClearTex(){shaderInst->ClearTextures();}

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
			.beginClass<Text>("Text")
				.addProperty("letter_count", &Text::GetSymbolsCount)
				.addProperty("line_h", &Text::GetHeight)
				.addProperty("line_w", &Text::GetWidth)
				.addFunction("GetLetterPos", &Text::GetSymPos)
				.addFunction("GetClosestLetter", &Text::GetClosestSym)
			.endClass();
		}

	private:
		bool initBuffers(wstring &text);
		bool updateBuffer(wstring &text);
		bool updateMatrix();

		SimpleShaderInst* shaderInst;

		Font *textFont;
		ID3D11Buffer *vertexBuffer; 
		ID3D11Buffer *indexBuffer;	
		ID3D11Buffer *constantBuffer;
		uint16_t numIndex;
		uint16_t numDrawIndex;
		uint16_t numVertex;

		int16_t* symbolsPos;
		uint16_t symbolsCount;

		XMFLOAT2 textPos;

		bool is_static;
		uint16_t maxLength;

		bool dirty;
	};

//------------------------------------------------------------------
}