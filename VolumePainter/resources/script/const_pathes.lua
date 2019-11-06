EXT = {
    MATERIAL = ".mtb",
    MESH = ".msh",
    SKELETON = ".skl",
    ANIMATION = ".ani",
    COLLISION = ".cls",
    TEXTURE = ".dds",
    FONT = ".fnt",
    ASSET = ".ast",
    PACKAGE = ".pkg",

    IMPORT = ".imp",
        
    CONFIG = ".cfg",
    RESOURCE = ".rsc",
    
    SCRIPT = ".lua",
    SCRIPT_COMPILED = ".luac",

    SHADER_SOURCE = ".hlsl",
    SHADER_BYTECODE = ".bc",
    SHADER_TECHS = ".tq",
}

PATH = {
    BIN = "bin/",
    ROOT = "../",
}

PATH.SYS_TEXTURES = PATH.ROOT.."resources/textures/"
PATH.SYS_MESHES = PATH.ROOT.."resources/meshes/"
PATH.SYS_MATS = PATH.ROOT.."resources/materials/"
PATH.EDITOR_MESHES = PATH.SYS_MESHES.."editor/"
PATH.ENVS = PATH.ROOT.."content/environments/"
PATH.RSC = PATH.ROOT.."resources/"
PATH.SHADERS = PATH.RSC.."shaders/"
PATH.SCRIPTS = PATH.ROOT.."resources/script/"
PATH.GUI = PATH.ROOT.."resources/gui/"
PATH.CONFIG = PATH.ROOT.."config/"
PATH.FONTS = PATH.RSC.. "fonts/"

PATH.KEYMAPS = PATH.CONFIG.."keymaps/"

--PATH.MATERIAL_TECH = PATH.MATERIALS.."tech/"
--PATH.EDITOR_MATERIAL = PATH.MATERIALS.."editor/"
--PATH.MATERIALS_GUI = PATH.MATERIALS.."hud/"

PATH.RUNTIME_STATS = PATH.ROOT.."stats/"