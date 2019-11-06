-- premake5.lua

workspace "VolumePainter"
	configurations {"Release"}
	platforms {"x64"}
			
	project "Shaders"
		kind "Utility"
		location "resources/shaders"
		language "C++"
		
		files {
			"resources/shaders/**.hlsl"
		}
		
		filter { "files:**" }
			flags { "ExcludeFromBuild" }
			
	project "Scripts"
		kind "Utility"
		location "resources"
		language "C++"
		
		files {
			"**.lua",			
			"**.cfg"
		}
		
		filter { "files:**" }
			flags { "ExcludeFromBuild" }