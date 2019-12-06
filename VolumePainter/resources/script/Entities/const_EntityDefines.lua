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

COLLISION_GROUPS = {
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

	NoCollide = 65536,

	Physics = 63,--Default | Static | Kinematic | Debris | Character | Trigger,

	RayCastPhysicsNoChar = 7,--Default | Static | Kinematic,
	RayCastPhysics = 39,--Default | Static | Kinematic | Character,

	All = -1,
    AllNoSpecial = 127,--All & ~(Special0 | Special1 | Special2 | Special3 | Special4 | Special5),
}

ENVPROB_TYPE = {
    None = 0, 
    Sphere = 1,
    Box = 2
}

ENVPROB_QUALITY = {
    High = 0, 
    Standart = 1,
    Low = 2
}

ENVPROB_PRIORITY_ALWAYS = 0