#include "stdafx.h"
#include "Log.h"

#include <io.h>
#include <fcntl.h>

namespace EngineCore
{
//------------------------------------------------------------------
	
	Log *Log::instance = nullptr;

	Log::Log(bool has_console)
	{	
		if(instance)
		{
			Err("Only one instance of Log is allowed!");
			return;
		}
		
		instance = this;

		//setlocale(LC_ALL, "rus");
		setlocale(LC_ALL,"Russian"); // force lua string.format() separate floats with comma, only in release
		//setlocale( LC_ALL,".1251" );

		// CONSOLE ----------------
		if(has_console)
		{
			AllocConsole();
			SetConsoleTitle( L"Log console" );
			EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
			DrawMenuBar(GetConsoleWindow());

			//Redirect unbuffered STDIN to the console
			HANDLE stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
			if(stdInHandle != INVALID_HANDLE_VALUE)
			{
				int fileDescriptor = _open_osfhandle((intptr_t)stdInHandle, _O_TEXT);
				if(fileDescriptor != -1)
				{
					FILE* file = _fdopen(fileDescriptor, "r");
					if(file != NULL)
					{
						*stdin = *file;
						setvbuf(stdin, NULL, _IONBF, 0);
					}
				}
			}

			//Redirect unbuffered STDOUT to the console
			consoleOutH = GetStdHandle(STD_OUTPUT_HANDLE);
			if(consoleOutH != INVALID_HANDLE_VALUE)
			{
				int fileDescriptor = _open_osfhandle((intptr_t)consoleOutH, _O_TEXT);
				if(fileDescriptor != -1)
				{
					FILE* file = _fdopen(fileDescriptor, "w");
					if(file != NULL)
					{
						*stdout = *file;
						setvbuf(stdout, NULL, _IONBF, 0);
					}
				}
			}

			//Redirect unbuffered STDERR to the console
			HANDLE stdErrHandle = GetStdHandle(STD_ERROR_HANDLE);
			if(stdErrHandle != INVALID_HANDLE_VALUE)
			{
				int fileDescriptor = _open_osfhandle((intptr_t)stdErrHandle, _O_TEXT);
				if(fileDescriptor != -1)
				{
					FILE* file = _fdopen(fileDescriptor, "w");
					if(file != NULL)
					{
						*stderr = *file;
						setvbuf(stderr, NULL, _IONBF, 0);
					}
				}
			}
		}

		// FILE ----------------
		m_file = nullptr;
		
		GET_DATETIME

		string filename = PATH_RUNTIME_STATS "log_" + datetime + ".log";

		if( fopen_s(&m_file, filename.c_str(), "w") == 0 )
		{
			char timer[9];
			_strtime_s(timer,9);
			char date[9];
			_strdate_s(date,9);
			fprintf(m_file, "Log started: %s %s.\n", date, timer);
			fprintf(m_file, "===========================\n");
		}		
		else
		{
			printf("Error creating log file!\n");
			m_file = nullptr;
		}	
	}

	Log::~Log()
	{
		FreeConsole();

		if (m_file)
		{
			char timer[9];
			_strtime_s(timer,9);
			char date[9];
			_strdate_s(date,9);
			fprintf(m_file, "===========================\n");
			fprintf(m_file, "End of log: %s %s.", date, timer);
			fclose(m_file);
		}

		instance = nullptr;
	}

	void Log::m_print(const char *levtext, const char *text, bool infile)
	{
		char timer[9];
		_strtime_s(timer,9);
		clock_t cl = clock();

		printf("%s:%d|%s%s\n", timer, cl, levtext, text); 
		if(m_file && infile)
		{
			fprintf(m_file, "%s:%d|%s%s\n", timer, cl, levtext, text);
			fflush(m_file);
		}
	}

#define LOG_BODY(prefix, infile) \
		va_list args; \
		va_start(args, message); \
		int len = _vscprintf( message, args ) + 1; \
		char *buffer = static_cast<char*>( malloc(len*sizeof(char)) ); \
		vsprintf_s( buffer, len, message, args ); \
		m_print(#prefix " ", buffer, infile); \
		va_end(args); \
		free(buffer);

#define W_LOG_BODY(prefix, infile) \
		va_list args; \
		va_start(args, message); \
		int len = _vscwprintf( message, args ) + 1; \
		wchar_t *buffer = static_cast<wchar_t*>( malloc(len*sizeof(wchar_t)) ); \
		vswprintf_s( buffer, len, message, args ); \
		m_wprint(L#prefix L" ", buffer, infile); \
		va_end(args); \
		free(buffer);

#define SQ_LOG_BODY(prefix, infile) \
		int len = _vscprintf( message, args ) + 1; \
		char *buffer = static_cast<char*>( malloc(len*sizeof(char)) ); \
		vsprintf_s( buffer, len, message, args ); \
		m_print(#prefix " ", buffer, infile); \
		free(buffer);

#define W_SQ_LOG_BODY(prefix, infile) \
		int len = _vscwprintf( message, args ) + 1; \
		wchar_t *buffer = static_cast<wchar_t*>( malloc(len*sizeof(wchar_t)) ); \
		vswprintf_s( buffer, len, message, args ); \
		m_wprint(L#prefix L" ", buffer, infile); \
		free(buffer);

	void Log::Print(const char *message, ...)
	{
		PRINT_COLOR
		LOG_BODY(:, true)
	}

	void Log::PrintGood(const char *message, ...)
	{
		GOOD_COLOR
		LOG_BODY(:, true)
	}

	void Log::Debug(const char *message, ...)
	{
		DEBUG_COLOR
		LOG_BODY(DEBUG:, false)
	}

	void Log::Err(const char *message, ...)
	{
		ERROR_COLOR
		LOG_BODY(ERROR:, true)
	}

	void Log::War(const char *message, ...)
	{
		WARNING_COLOR
		LOG_BODY(WARNING:, true)
	}

	void Log::Lua(const char *message, ...)
	{
		LUA_COLOR
		LOG_BODY(_LUA:, true)
	}

	void Log::LuaError(const char *message, ...)
	{
		ERROR_COLOR
		LOG_BODY(_LUA_ERROR:, true)
	}
}