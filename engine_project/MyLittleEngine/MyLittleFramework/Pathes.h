#pragma once

#define PATH_BIN "bin/"

#define PATH_ROOT "../"

#define EXT_MATERIAL ".mtb"
#define EXT_STATIC ".stm"
#define EXT_TEXTURE ".dds"
#define EXT_RESOURCE ".rsc"
#define EXT_CONFIG ".cfg"
#define EXT_SCRIPT ".lua"
#define EXT_FONT ".fnt"

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

#define PATH_STMESH_NULL PATH_SYS_MESHES "null_mesh" EXT_STATIC

#define PATH_RU_LOCAL PATH_RSC "localization/ru" EXT_RESOURCE
#define PATH_EN_LOCAL PATH_RSC "localization/en" EXT_RESOURCE
#define PATH_DEFAULT_LOCAL PATH_RSC "localization/en" EXT_RESOURCE

#define PATH_ENGINE_CONFIG PATH_CONFIG "engine_settings" EXT_CONFIG

#define PATH_GUI_COLORS PATH_RSC "colors" EXT_RESOURCE

#define PATH_MAIN_SCRIPT PATH_SCRIPTS "main" EXT_SCRIPT

#define PATH_RUNTIME_STATS PATH_ROOT "stats/"