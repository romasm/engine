
CTRL_CMDS = {
    CC_NOPE						= 0x00,
    CC_DELTA_ROT				= 0x01,		
    CC_FORWARD_START			= 0x02,
    CC_FORWARD_END				= 0x03,		
    CC_BACK_START				= 0x04,
    CC_BACK_END					= 0x05,		
    CC_LEFT_START				= 0x06,
    CC_LEFT_END					= 0x07,		
    CC_RIGHT_START				= 0x08,
    CC_RIGHT_END				= 0x09,		
    CC_UP_START					= 0x0A,
    CC_UP_END					= 0x0B,
    CC_DOWN_START				= 0x0C,
    CC_DOWN_END					= 0x0D,
    CC_MOVE_SPEED_CHANGE		= 0x0E
}

SHADERS = {
    PS = 0,
    VS = 1,
    DS = 2,
    HS = 3,
    GS = 4,
}

SELECTION_MODE = {
    NONE = 0,
    SIMPLE = 1,
    SNAKE = 2,
}

TRANSFORM_MODE = {
    NONE = 0,
    MOVE = 1,
    ROT = 2,
    SCALE = 3,
}

TABLE_ORIENT = {
    LEFT = 0,
    RIGHT = 1,
    TOP = 2,
    BOTTOM = 3,
}

TABLE_DIV = {
    HORZ = 0,
    VERT = 1,
    BOTH = 2,
}

COMMON_UPDATE_WAIT = 50

LIGHT_TYPE = {
    POINT = 1,
    SPHERE = 2,
    TUBE = 3,
    SPOT = 4,
    DISK = 5,
    RECT = 6,
}

COMPONENTS = {
    TRANSFORM = 1,
    LIGHT = 2,
    GLIGHT = 3,
    STATIC = 4,
    SCRIPT = 5,
    COLLISION = 6,
}

WORLDMODES = {
    NO_LIVE = 0,
    LIVE = 1,
}

SELECTION = {
    MAXDIST = 10000,
    DEFAULT_CAM_OFFSET = 5.0,
}

RESOURCE_TYPE = {
	TEXTURE = 0,
	MESH = 1,
	COLLISION = 2,
	SKELETON = 3,
	ANIMATION = 4,
	MATERIAL = 5,
	FONT = 6,
	SHADER = 7,
	GUI_SHADER = 8,
	COMPUTE = 9,
}