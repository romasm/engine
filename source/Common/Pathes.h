#pragma once

#define PATH_BIN "bin/"

#define PATH_ROOT "../"

#define EXT_MATERIAL ".mtb"
#define EXT_MESH ".msh"
#define EXT_SKELETON ".skl"
#define EXT_ANIMATION ".anm"
#define EXT_COLLISION ".cls"
#define EXT_TEXTURE ".dds"
#define EXT_RESOURCE ".rsc"
#define EXT_CONFIG ".cfg"
#define EXT_SCRIPT ".lua"
#define EXT_SCRIPT_COMPILED ".luac"
#define EXT_FONT ".fnt"
#define EXT_ASSET ".ast"
#define EXT_PACKAGE ".pkg"

#define EXT_IMPORT ".imp"

#define EXT_SHADER_SOURCE ".hlsl"
#define EXT_SHADER_BYTECODE ".bc"
#define EXT_SHADER_TECHS ".tq"

#define PATH_MATERIALS PATH_ROOT "content/materials/"
#define PATH_TEXTURES PATH_ROOT "content/textures/"
#define PATH_SYS_TEXTURES PATH_ROOT "resources/textures/"
#define PATH_SYS_MESHES PATH_ROOT "resources/meshes/"
#define PATH_SYS_MATS PATH_ROOT "resources/materials/"
#define PATH_STATICS PATH_ROOT "content/statics/"
#define PATH_EDITOR_MESHES PATH_STATICS "editor/"
#define PATH_ENVS PATH_ROOT "content/environments/"
#define PATH_RSC PATH_ROOT "resources/"
#define PATH_SHADERS PATH_RSC "shaders/"
#define PATH_SCRIPTS PATH_ROOT "resources/script/"
#define PATH_GUI PATH_ROOT "resources/gui/"
#define PATH_CONFIG PATH_ROOT "config/"
#define PATH_FONTS PATH_RSC "fonts/"

#define PATH_KEYMAPS PATH_CONFIG "keymaps/"

#define PATH_MATERIAL_TECH PATH_MATERIALS "tech/"
#define PATH_EDITOR_MATERIAL PATH_MATERIALS "editor/"
#define PATH_MATERIALS_GUI PATH_MATERIALS "hud/"

#define PATH_MATERIAL_NULL PATH_SYS_MATS "null_material" EXT_MATERIAL
#define PATH_TEXTURE_NULL PATH_SYS_TEXTURES "null_tex" EXT_TEXTURE
#define PATH_SHADER_NULL PATH_SHADERS "system/null_shader"

#define PATH_STMESH_NULL PATH_SYS_MESHES "null_mesh" EXT_MESH
#define PATH_SKELETON_NULL PATH_SYS_MESHES "null_mesh" EXT_SKELETON
#define PATH_COLLISION_NULL PATH_SYS_MESHES "null_collision" EXT_COLLISION

#define PATH_RU_LOCAL PATH_RSC "localization/ru" EXT_RESOURCE
#define PATH_EN_LOCAL PATH_RSC "localization/en" EXT_RESOURCE
#define PATH_DEFAULT_LOCAL PATH_RSC "localization/en" EXT_RESOURCE

#define PATH_ENGINE_CONFIG PATH_CONFIG "engine_settings" EXT_CONFIG
#define PATH_COLORS_CONFIG PATH_CONFIG "colors_default" EXT_CONFIG

#define PATH_MAIN_SCRIPT PATH_SCRIPTS "main"

#define PATH_RUNTIME_STATS PATH_ROOT "stats/"