#pragma once

namespace EngineCore
{
	enum PERF_CPU
	{
		PERF_CPU_FRAME,
		PERF_CPU_GUIUPDATE,
		PERF_CPU_SCENE,
		PERF_CPU_SCENE_UPDATE,
		PERF_CPU_SCENE_DRAW,
		PERF_CPU_GUIDRAW,

		PERF_CPU_COUNT,
	};

	enum PERF_GPU
	{
		PERF_GPU_FRAME,

		PERF_GPU_SCENE,
		PERF_GPU_GUI,

		PERF_GPU_COUNT,
	};
}
