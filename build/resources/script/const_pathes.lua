EXT = {
    MATERIAL = ".mtb",
    MESH = ".stm",
    TEXTURE = ".dds",
    RESOURCE = ".rsc",
    CONFIG = ".cfg",
    SCRIPT = ".lua",
    SCRIPT_COMPILED = ".luac",
    FONT = ".fnt",

    SHADER_SOURCE = ".hlsl",
    SHADER_BYTECODE = ".bc",
    SHADER_TECHS = ".tq",
}

PATH = {
    BIN = "bin/",
    ROOT = "../",

    SYS_TEXTURES = ROOT.."resources/textures/",
    SYS_MESHES = ROOT.."resources/meshes/",
    SYS_MATS = ROOT.."resources/materials/",
    EDITOR_MESHES = SYS_MESHES.."editor/",
    ENVS = ROOT.."content/environments/",
    RSC = ROOT.."resources/",
    SHADERS = RSC.."shaders/",
    SCRIPTS = ROOT.."resources/script/",
    GUI = ROOT.."resources/gui/",
    CONFIG = ROOT.."config/",
    FONTS = RSC"fonts/",

    KEYMAPS = CONFIG.."keymaps/",

    MATERIAL_TECH = MATERIALS.."tech/",
    EDITOR_MATERIAL = MATERIALS.."editor/",
    MATERIALS_GUI = MATERIALS.."hud/",

    RUNTIME_STATS = ROOT.."stats/",
}