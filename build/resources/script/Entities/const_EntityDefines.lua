EDITOR_VARS = {
    TYPE = '_editor_',
    INVIS = '_invis_',
}

PHYSICS_TYPES = {
    NONE = -1,
    STATIC = 0, 
    KINEMATIC = 1, 
    DYNAMIC = 2,
}

COLLISION_GROUPS
{
	None = 0,
	Default = 1,
	Static = 2,
	Kinematic = 4,
	Debris = 8,
	Trigger = 16,
	Character = 32,
	Gamelogic = 64,

	Special0 = 128,
	Special1 = 256,
	Special2 = 512,
	Special3 = 1024,
	Special4 = 2048,
	Special5 = 4096,

	Physics = Default + Static + Kinematic + Debris + Character + Trigger,
	All = -1
}