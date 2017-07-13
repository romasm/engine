#include "stdafx.h"
#include "Font.h"
#include "Common.h"
#include "Shader.h"
#include "Buffer.h"
#include "MaterialMgr.h"

using namespace EngineCore;

Font::Font(string& name)
{
	fontName = name;
	texture = 0;
	lineH = 0;
	texWidth = 0;
	texHeight = 0;

	string fullname = fontName + EXT_FONT;
	parse(fullname);
}

bool Font::parse(string& filename)
{
	ifstream fin;
	fin.open(filename);
	if(fin.fail())
		return false;

	string Line;
	string Read, Key, Value;
	size_t i;
	while( !fin.eof() )
	{
		std::stringstream LineStream;
		std::getline( fin, Line );
		LineStream << Line;
		
		LineStream >> Read;
		if( Read == "common" )
		{
			while( !LineStream.eof() )
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find( '=' );
				Key = Read.substr( 0, i );
				Value = Read.substr( i + 1 );

				Converter << Value;
				if( Key == "scaleW" )
					Converter >> texWidth;
				else if( Key == "scaleH" )
					Converter >> texHeight;
				else if( Key == "lineHeight" )
					Converter >> lineH;
			}
		}
		else if( Read == "page" )
		{
			while( !LineStream.eof() )
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find( '=' );
				Key = Read.substr( 0, i );
				Value = Read.substr( i + 1 );

				std::string str;
				Converter << Value;
				if( Key == "file" )
				{
					Converter >> str;
					str = str.substr(1, Value.length()-2);
					auto path_ptr = filename.rfind('/');
					string tex_path = filename.substr(0, path_ptr+1) + str;
				#ifdef _DEV
					texture = TEXTURE( tex_path );
				#else
					texture = RELOADABLE_TEXTURE( tex_path, true );
				#endif
				}
			}
		}
		else if( Read == "char" )
		{
			uint16_t CharID = 0;
			CharDesc chard;

			while( !LineStream.eof() )
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find( '=' );
				Key = Read.substr( 0, i );
				Value = Read.substr( i + 1 );

				Converter << Value;
				if( Key == "id" )
					Converter >> CharID;
				else if( Key == "x" )
					Converter >> chard.srcX;
				else if( Key == "y" )
					Converter >> chard.srcY;
				else if( Key == "width" )
					Converter >> chard.srcW;
				else if( Key == "height" )
					Converter >> chard.srcH;
				else if( Key == "xoffset" )
					Converter >> chard.xOff;
				else if( Key == "yoffset" )
					Converter >> chard.yOff;
				else if( Key == "xadvance" )
					Converter >> chard.xAdv;
			}
			symbols.insert(pair<uint16_t, CharDesc>(CharID, chard));
		}
	}

	fin.close();
	return true;
}

void Font::BuildVertexArray(UnlitVertex *vertex, uint16_t numvert, wstring& sentence, int16_t* symbols_pos, uint16_t* letter_count)
{
	uint16_t numLetters = (uint16_t)sentence.size();
	
	if ( numLetters * 4 > numvert )
		numLetters = numvert / 4;
			
	float drawX = 0.0f;

	uint16_t index = 0;		
	for(uint16_t i = 0; i < numLetters; i++)
	{
		auto& symData = symbols[(uint16_t)sentence[i]];

		float CharX = symData.srcX;
		float CharY = symData.srcY;
		float Width = symData.srcW;
		float Height = symData.srcH;
		float OffsetX = symData.xOff;
		float OffsetY = symData.yOff;

		float left = drawX + OffsetX;
		float right = left + Width;
		float top = - OffsetY;
		float bottom = top - Height;
		float lefttex = CharX/texWidth;
		float righttex = (CharX+Width)/texWidth;
		float toptex = CharY/texHeight;
		float bottomtex = (CharY+Height)/texHeight;
					
		if(symbols_pos)
		{
			symbols_pos[i] = (int16_t)drawX;
			symbols_pos[i+1] = (int16_t)(drawX + symData.xAdv);
		}

		vertex[index].Pos = Vector3(left, top, 0.0f);
		vertex[index].Tex = Vector2(lefttex, toptex);
		index++;		
		vertex[index].Pos = Vector3(right, bottom, 0.0f);
		vertex[index].Tex = Vector2(righttex, bottomtex);
		index++;
		vertex[index].Pos = Vector3(left, bottom, 0.0f);
		vertex[index].Tex = Vector2(lefttex, bottomtex);
		index++;
		vertex[index].Pos = Vector3(right, top, 0.0f);
		vertex[index].Tex = Vector2(righttex, toptex);
		index++;
		
		drawX += symData.xAdv;
	}

	if(letter_count)
		*letter_count = numLetters;
}