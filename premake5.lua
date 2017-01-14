-- premake5.lua

workspace "Engine"
	configurations {"Debug", "Development", "Release"}
	platforms {"x64"}
	        	
	project "Core"
		kind "WindowedApp"
		location "source"
		language "C++"
				
		includedirs { 
			"source/Common/" ,
			"source/Config/" ,
			"source/ECS/" ,
			"source/GUI/" ,
			"source/Input/" ,
			"source/Managers/" ,
			"source/Render/" ,
			"source/Render/Support/" ,
			"source/System/" ,
			"source/Utils/" ,
			"source/" ,
			
			-- third-party
			"thirdparty/Dirent" ,
			"thirdparty/Assimp/Inc" ,
			"thirdparty/Lua/LuaBridge" ,
			"thirdparty/Lua/LuaJIT/src" ,
			"thirdparty/DirectXTK/Inc" ,
			"thirdparty/DirectXTex/Inc" ,
		}
		
		libdirs {			
			"thirdparty/Assimp/lib"
		}
		
		filter "platforms:x64"
			targetdir "build/bin"
			debugdir "build/bin"
					
		filter "configurations:Debug"
			defines { "_EDITOR", "WIN64", "_DEV", "_DEBUG;" }
			flags{ "FatalWarnings" }
			symbols "On"
			optimize "Off"
			targetname "core_d"		
			libdirs {			
				"thirdparty/Lua/lib/debug" ,
				"thirdparty/DirectXTK/lib/x64/Debug" ,
				"thirdparty/DirectXTex/lib/x64/Debug" ,
			}
			
		filter "configurations:Development"
			defines { "_EDITOR", "WIN64", "_DEV", "NDEBUG;" }
			flags{ "LinkTimeOptimization", "FatalWarnings" }
			symbols "On"
			optimize "Speed"
			targetname "core_dev"	
			libdirs {			
				"thirdparty/Lua/lib/release" ,
				"thirdparty/DirectXTK/lib/x64/Release" ,
				"thirdparty/DirectXTex/lib/x64/Release" ,
			}
			
		filter "configurations:Release"
			defines { "_EDITOR", "WIN64", "NDEBUG;" }
			flags{ "LinkTimeOptimization", "FatalWarnings" }
			optimize "Speed"
			targetname "core"	
			libdirs {			
				"thirdparty/Lua/lib/release" ,
				"thirdparty/DirectXTK/lib/x64/Release" ,
				"thirdparty/DirectXTex/lib/x64/Release" ,
			}
		
		filter {}
		
		links { "assimp-vc110-mt", "lua51" }
		
		files {
			"source/Common/*.h", 
			"source/Common/*.cpp",
			"source/Config/*.h", 
			"source/Config/*.cpp",
			"source/ECS/*.h", 
			"source/ECS/*.cpp",
			"source/GUI/*.h", 
			"source/GUI/*.cpp",
			"source/Input/*.h", 
			"source/Input/*.cpp",
			"source/Managers/*.h", 
			"source/Managers/*.cpp",
			"source/Render/*.h", 
			"source/Render/*.cpp",
			"source/Render/Support/*.h" ,
			"source/Render/Support/*.cpp" ,
			"source/System/*.h", 
			"source/System/*.cpp",
			"source/Utils/*.h", 
			"source/Utils/*.cpp",
			"source/*.h",
			"source/*.cpp"
		}
		
		pchheader "stdafx.h"
		pchsource "source/stdafx.cpp"