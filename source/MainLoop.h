#pragma once

#include "Hud.h"
#include "WindowsMgr.h"
#include "Render.h"
#include "Log.h"
#include "Timer.h"
#include "Pathes.h"
#include "LuaVM.h"
#include "macros.h"
#include "JobSystem.h"
#include "WorldMgr.h"

#include "Utils\Profiler.h"

class MainLoop
{
public:
	MainLoop(string& luaScript)
	{
		init = false;

		jobSystem = new JobSystem(0);
		
	#ifdef _DEV
		profiler = new Profiler;
		profiler->RegThread(this_thread::get_id().hash());
		jobSystem->RegThreadsForProfiler();
	#endif
		Profiler::RegLuaFunctions();

		if(m_instance)
		{
			ERR("Only one instance of MainLoop is allowed!");
			return;
		}
		m_instance = this;

		m_render = new Render;
		if (!m_render || !m_render->Init())
		{
			ERR("Unable to create Render");
			return;
		}

		m_resourceProc = new ResourceProcessor;

		m_wins = new WindowsMgr;

		m_hud = new Hud;

		if(!m_hud || !m_hud->Init())
		{
			ERR("Unable to initialize Gui");
			return;
		}

		// Lua
		if(!LuaVM::Get()->LoadScript(luaScript))
		{
			ERR("Unable to initialize Lua Main");
			return;
		}

		main_table = new LuaRef(getGlobal(LSTATE, "Main"));
		if(!main_table->isTable())
		{
			ERR("Lua Main is incorrect!");
			return;
		}

		tick_func = new LuaRef((*main_table)["onTick"]);
		if(!tick_func->isFunction())
			_DELETE(tick_func);
	
		LuaRef start_func = (*main_table)["Start"];
		if(!start_func.isFunction())
		{
			ERR("Require Main:Start() in %ls", PATH_MAIN_SCRIPT);
			return;
		}
		start_func((*main_table));
		// Lua

		m_timer.Frame();

		fpslock = 1000.0f / EngineSettings::EngSets.fpslock;

		init = true;
	}

	~MainLoop()
	{
		_DELETE(tick_func);
		_DELETE(main_table);
		_DELETE(m_wins);
		_DELETE(m_resourceProc);

		_CLOSE(m_hud);
		_CLOSE(m_render);
		_CLOSE(jobSystem);
		init = false;
		m_instance = nullptr;

	#ifdef _DEV
		_DELETE(profiler);
	#endif
	}

	void Run()
	{
		double rendertime = 0;
		while(Frame(rendertime, false, false))
		{
			//Sleep(uint32_t( max(0, (fpslock - waittime) * 0.25f) )); // todo
			Sleep(0); // todo
		}
	}

	inline bool Frame(double& rendertime, bool force_update_gui, bool no_gui_gc)
	{
		double cur_time = Timer::ForcedGetCurrentTime();
		float waittime = float(cur_time - rendertime);
		
		if( waittime >= fpslock )
		{
			PERF_CPU_FRAME_BEGIN;
			PERF_GPU_FRAME_BEGIN;

			rendertime = cur_time;
			m_timer.Frame();

			// job update
			jobSystem->Tick(m_timer.dt_ms);
			m_resourceProc->Tick(m_timer.dt_ms);
			
			PERF_CPU_BEGIN(_LUA_TICK);
			// Lua
			if(tick_func)
			{
				auto isExit = (*tick_func)((*main_table), m_timer.dt_ms);
				if( !isExit.cast<bool>() )
					return false;	
			}
			PERF_CPU_END(_LUA_TICK);
			
			PERF_CPU_BEGIN(_WIN_MSG);
			if(!force_update_gui)
			{
				// Events
				if(!m_wins->Tick())
					return false;
			}
			PERF_CPU_END(_WIN_MSG);
			
			PERF_CPU_BEGIN(_GUI_UPDATE);
			m_hud->Update(force_update_gui, no_gui_gc);
			PERF_CPU_END(_GUI_UPDATE);	

			// Render
			PERF_CPU_BEGIN(_SCENE);	
			PERF_GPU_TIMESTAMP(_SCENE);	

			WorldMgr::Get()->UpdateWorlds();

			PERF_CPU_END(_SCENE);

			PERF_CPU_BEGIN(_GUI_DRAW);
			PERF_GPU_TIMESTAMP(_GUI);		
			for(auto& winId : *m_wins->GetMap())
			{
				auto window = m_wins->GetWindowByID(winId.second);
				window->ClearRenderTarget();

				m_hud->Draw(winId.first);
			}
			PERF_CPU_END(_GUI_DRAW);	
			
			PERF_CPU_BEGIN(_PRESENT);
			//PERF_GPU_TIMESTAMP(_PRESENT);	
			PERF_GPU_FRAME_END;

			for(auto& winId : *WindowsMgr::Get()->GetMap())
			{
				auto window = m_wins->GetWindowByID(winId.second);
				window->Swap();
			}
			PERF_CPU_END(_PRESENT);	

			PERF_GPU_GRABDATA;
			//PERF_GPU_FRAME_END;

			PERF_CPU_FRAME_END;
		}
		return true;
	}

	bool Succeeded() {return init;}

	inline static MainLoop* Get(){return m_instance;}

private:
	static MainLoop *m_instance;

	Timer m_timer;
	EngineSettings m_esets;
	LuaVM Lua_VM;

	JobSystem* jobSystem;
	
	WindowsMgr* m_wins;
	ResourceProcessor* m_resourceProc;
	Render* m_render;
	Hud* m_hud;

	LuaRef* main_table;
	LuaRef* tick_func;

	float fpslock;

	bool init;

#ifdef _DEV
	Profiler* profiler;
#endif
};