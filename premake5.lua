-- premake5.lua

workspace "Engine"
	configurations {"Debug", "Development", "Release"}
	platforms {"x64"}
	        	
	project "Core"
		kind "ConsoleApp"
		location "source"
		language "C++"
		cppdialect "C++20"
				
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
			"source/Render/RHI/" ,
			"source/Render/RHI/DX11/" ,
			"source/Render/RHI/DX12/" ,
			"source/Render/RayTracing/" ,
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
		
		systemversion "10.0.26100.0"
			
		filter "platforms:x64"
			targetdir "build/bin"
			debugdir "build/bin"
			debugargs { "-c" }
					
		filter "configurations:Debug"
			defines { "_EDITOR", "WIN64", "_DEV", "_DEBUG;" }
			flags{ "FatalWarnings" }
			disablewarnings { "4099", "4244", "5033" }
			buildoptions { "/FS" }
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
			disablewarnings { "4099", "4244", "5033" }
			buildoptions { "/FS" }
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
			disablewarnings { "4099", "4244", "5033" }
			buildoptions { "/FS" }
			optimize "Speed"
			targetname "core"	
			libdirs {			
				"thirdparty/Lua/lib/release" ,
				"thirdparty/DirectXTK/lib/x64/Release" ,
				"thirdparty/DirectXTex/lib/x64/Release" ,
			}
		
		filter {}

		-- suppress linker warning for missing PDBs from third-party libs (Bullet, etc.)
		linkoptions { "/IGNORE:4099" }

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
			"source/Managers/Loaders/*.h",
			"source/Managers/Loaders/*.cpp",
			"source/Render/*.h",
			"source/Render/*.cpp",
			"source/Render/Support/*.h",
			"source/Render/Support/*.cpp",
			"source/Render/RHI/*.h",
			"source/Render/RHI/*.cpp",
			"source/Render/RHI/DX11/*.h",
			"source/Render/RHI/DX11/*.cpp",
			"source/Render/RHI/DX12/*.h",
			"source/Render/RHI/DX12/*.cpp",
			"source/Render/RayTracing/*.h",
			"source/Render/RayTracing/*.cpp",
			"source/System/*.h",
			"source/System/*.cpp",
			"source/Utils/*.h",
			"source/Utils/*.cpp",
			"source/*.h",
			"source/*.cpp"
		}
		
		-- exclude unfinished files that don't compile yet
		removefiles {
			"source/GUI/HVisuals.h",
			"source/GUI/HVisuals.cpp",
			"source/Render/VoxelRender.h",
			"source/Render/VoxelRender.cpp",
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