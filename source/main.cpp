#include "stdafx.h"

#include "MainLoop.h"
#include "EngineSettings.h"
#include "Pathes.h"

MainLoop* MainLoop::m_instance = nullptr;
EngineSettings* EngineSettings::instance = nullptr;

struct CommandLineParams
{
	bool console;
	string scriptMain;

	CommandLineParams()
	{
		console = false;
		scriptMain = PATH_MAIN_SCRIPT;
	}
};

inline void ApplyCommandLineParams(int argc, char** argv, CommandLineParams& params) // TODO: remove to config system
{
	if(argc <= 1)
		return;

	for(int i = 1; i < argc; i++)
	{
		if( argv[i][0] != '-' || strlen(argv[i]) <= 1 )
			continue;

		switch( argv[i][1] )
		{
		case 'c':
			params.console = true;
			break;

		case 's':
			{
				int path_i = i + 1;
				if( path_i >= argc )
					return;

				if( argv[path_i][0] == '-' )
					break;

				params.scriptMain = argv[path_i];
				i++;
			}
			break;

		// add params here
		}
	}
}

void configLocale()
{
	setlocale(LC_ALL,""); 
	setlocale(LC_NUMERIC,"C"); 

	locale loc("", locale::all & (~locale::numeric));
	locale::global(loc);
}

int main(int argc, char* argv[])
{
	auto hr = SetCurrentProcessExplicitAppUserModelID(L"MLE.Renderer"); // TEMP
	if(hr != S_OK)
		return 0;
	
	CommandLineParams commandLineParams;
	ApplyCommandLineParams(argc, argv, commandLineParams); // TODO: remove to config system

	configLocale();

	Log* log = new Log(commandLineParams.console);

	MainLoop* loop = new MainLoop(commandLineParams.scriptMain);
	if(!loop->Succeeded())
		return 0;

	loop->Run();

	_DELETE(loop);
	_DELETE(log);
	return 0;
}