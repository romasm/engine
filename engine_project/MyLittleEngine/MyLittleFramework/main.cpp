#include "stdafx.h"

#include "MainLoop.h"
#include "EngineSettings.h"

MainLoop *MainLoop::m_instance = nullptr;
EngineSettings::ESet EngineSettings::EngSets;

inline char* GetCommand(int i, int argc, char** argv)
{
	if(i >= argc)
		return nullptr;
	return argv[i];
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

	bool console = false;
#ifdef _DEV
	console = true;
#else
	auto cmd = GetCommand(1, argc, argv); // TEMP
	if(cmd && strlen(cmd) > 1 && cmd[1] == 'c')
		console = true;
#endif

	configLocale();

	Log* log = new Log(console);

	MainLoop* loop = new MainLoop();
	
	if(!loop->Succeeded())
		return 0;

	loop->Run();

	_DELETE(loop);
	_DELETE(log);
	return 0;
}