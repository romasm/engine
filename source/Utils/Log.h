#pragma once

#include "Pathes.h"

namespace EngineCore
{
//------------------------------------------------------------------

#define GET_DATETIME \
	char time[9];_strtime_s(time, 9);\
	time[2] = '_';time[5] = '_';\
	char date[9];_strdate_s(date, 9);\
	date[2] = '_';date[5] = '_';\
	string datetime = date;datetime += '_';	datetime += time;

#define PRINT_COLOR SetConsoleTextAttribute(consoleOutH, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#define DEBUG_COLOR SetConsoleTextAttribute(consoleOutH, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
#define WARNING_COLOR SetConsoleTextAttribute(consoleOutH, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#define ERROR_COLOR SetConsoleTextAttribute(consoleOutH, FOREGROUND_RED | FOREGROUND_INTENSITY);
#define GOOD_COLOR SetConsoleTextAttribute(consoleOutH, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#define LUA_COLOR SetConsoleTextAttribute(consoleOutH, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

#define LOG(message, ...) Log::Get()->Print(message, __VA_ARGS__)
#define LOG_GOOD(message, ...) Log::Get()->PrintGood(message, __VA_ARGS__)
#define WRN(message, ...) Log::Get()->War(message" [%ls|%ls|%i]", __VA_ARGS__, __FILEW__, __FUNCTIONW__, __LINE__)
#define ERR(message, ...) Log::Get()->Err(message" [%ls|%ls|%i]", __VA_ARGS__, __FILEW__, __FUNCTIONW__, __LINE__)
#define DBG(message, ...) Log::Get()->Debug(message" [%ls|%ls|%i]", __VA_ARGS__, __FILEW__, __FUNCTIONW__, __LINE__)
#define DBG_SHORT(message, ...) Log::Get()->Debug(message, __VA_ARGS__)
#define LUA(message, ...) Log::Get()->Lua(message, __VA_ARGS__)
#define LUA_ERROR(message, ...) Log::Get()->LuaError(message, __VA_ARGS__)

	class Log
	{
	public:
		Log(bool has_console = false);
		~Log();

		inline static Log* Get(){return instance;}

		void Print(const char *message, ...);
		void PrintGood(const char *message, ...);
		void Debug(const char *message, ...);
		void Err(const char *message, ...);
		void War(const char *message, ...);

		void Lua(const char *message, ...);
		void LuaError(const char *message, ...);

	private:
		static Log *instance;

		void m_print(const char *levtext, const char *text, bool infile = true);

		FILE *m_file;	
		HANDLE consoleOutH;
	};

//------------------------------------------------------------------
}