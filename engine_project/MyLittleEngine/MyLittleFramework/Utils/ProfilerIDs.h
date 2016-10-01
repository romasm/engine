#pragma once

#define PERF_CPU_FRAME_ID		0
#define PERF_CPU_FRAME_STR		"Frame"

	#define PERF_CPU_GUIUPDATE_ID			1
	#define PERF_CPU_GUIUPDATE_STR			"Frame|GuiUpdate"

	#define PERF_CPU_SCENE_ID		2
	#define PERF_CPU_SCENE_STR		"Frame|Scene"

		#define PERF_CPU_SCENE_UPDATE_ID		3
		#define PERF_CPU_SCENE_UPDATE_STR	"Frame|Scene|Update"

		#define PERF_CPU_SCENE_DRAW_ID		4
		#define PERF_CPU_SCENE_DRAW_STR		"Frame|Scene|Draw"

#define PERF_CPU_FINISH_ID		5
#define PERF_CPU_FINISH_STR		"Finish"

#define PERF_IDS_COUNT			6
