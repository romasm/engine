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
			"source/Managers/Loaders/" ,
			"source/Render/" ,
			"source/Render/Support/" ,
			"source/System/" ,
			"source/Utils/" ,
			"source/" ,
			
			-- third-party
			"thirdparty/Dirent" ,
			"thirdparty/Assimp/include" ,
			"thirdparty/Lua/LuaBridge" ,
			"thirdparty/Lua/LuaJIT/src" ,
			"thirdparty/DirectXTK/Inc" ,
			"thirdparty/DirectXTex/Inc" ,
			"thirdparty/BulletPhysics/src" ,
			"thirdparty/DirectX12/Inc" ,
			
			-- optix
			--"C:/ProgramData/NVIDIA Corporation/OptiX SDK 6.0.0/include",
			--"C:/ProgramData/NVIDIA Corporation/OptiX SDK 6.0.0/SDK/sutil",
			--"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/include",
			
			-- dx12
			"$(WindowsSdkDir_10)Include/$(WindowsTargetPlatformVersion)/shared",
			"$(WindowsSdkDir_10)Include/$(WindowsTargetPlatformVersion)/um",
		}
		
		libdirs {			
			"thirdparty/Assimp/lib/Release",
			"thirdparty/BulletPhysics/lib",
			
			-- optix
			"C:/ProgramData/NVIDIA Corporation/OptiX SDK 6.0.0/lib64",
			--"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/lib/x64",
			
			--dx12
			"$(WindowsSdkDir_10)Lib/$(WindowsTargetPlatformVersion)/um/x64",
		}
		
		systemversion "10.0.20348.0"
			
		filter "platforms:x64"
			targetdir "build/bin"
			debugdir "build/bin"
			debugargs { "-c" }
					
		filter "configurations:Debug"
			defines { "_EDITOR", "WIN64", "_DEV", "_DEBUG;" }
			flags{ "FatalWarnings" }
			disablewarnings { "4099", "4244" }
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
			disablewarnings { "4099", "4244" }
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
			disablewarnings { "4099", "4244" }
			optimize "Speed"
			targetname "core"	
			libdirs {			
				"thirdparty/Lua/lib/release" ,
				"thirdparty/DirectXTK/lib/x64/Release" ,
				"thirdparty/DirectXTex/lib/x64/Release" ,
			}
		
		filter {}
		
		links { 
			"assimp-vc140-mt", 
			"lua51", 
			
			-- optix
			--"optix.6.0.0",
			--"nvrtc"
		}
		
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
			"source/Managers/Loaders/*.h" ,
			"source/Managers/Loaders/*.cpp" ,
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
		
	project "Shaders"
		kind "Utility"
		location "build/resources/shaders"
		language "C++"
		
		files {
			"build/resources/shaders/**.hlsl"
		}
		
		filter { "files:**" }
			flags { "ExcludeFromBuild" }
			
	project "Scripts"
		kind "Utility"
		location "build/resources"
		language "C++"
		
		files {
			"build/**.lua",			
			"build/**.cfg"
		}
		
		filter { "files:**" }
			flags { "ExcludeFromBuild" }