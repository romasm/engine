//~ exemple
sampler_exemple
{
	ComparisonFunc = ALWAYS;
	/*
	  NEVER
	  LESS
	  EQUAL
	  LESS_EQUAL
	  GREATER
	  NOT_EQUAL
	  GREATER_EQUAL
	  ALWAYS
	*/

    Filter = MIN_MAG_MIP_POINT;
	/*
	MIN_MAG_MIP_POINT
	MIN_MAG_POINT_MIP_LINEAR
	MIN_POINT_MAG_LINEAR_MIP_POINT
	MIN_POINT_MAG_MIP_LINEAR
	MIN_LINEAR_MAG_MIP_POINT
	MIN_LINEAR_MAG_POINT_MIP_LINEAR
	MIN_MAG_LINEAR_MIP_POINT
	MIN_MAG_MIP_LINEAR
	ANISOTROPIC
	COMPARISON_MIN_MAG_MIP_POINT
	COMPARISON_MIN_MAG_POINT_MIP_LINEAR
	COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
	COMPARISON_MIN_POINT_MAG_MIP_LINEAR
	COMPARISON_MIN_LINEAR_MAG_MIP_POINT
	COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
	COMPARISON_MIN_MAG_LINEAR_MIP_POINT
	COMPARISON_MIN_MAG_MIP_LINEAR
	COMPARISON_ANISOTROPIC
	*/

    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
	/*
	WRAP
	MIRROR
	MIRROR_ONCE
	CLAMP
	BORDER
	*/

	BorderColor = "0.0 0.0 0.0 0.0";

	MaxAnisotropy = 0;

	MaxLOD = 999999.0;
	MinLOD = 0;
	MipLODBias = 0;
};
//~

samplerPointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

samplerBilinearClamp
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

samplerTrilinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

samplerPointWrap
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = WRAP;
    AddressV = WRAP;
};

samplerBilinearWrap
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = WRAP;
    AddressV = WRAP;
};

samplerTrilinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

// configurable
samplerAnisotropicClamp
{
    Filter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP;
	MaxAnisotropy = 16; //todo
};

samplerAnisotropicWrap
{
    Filter = ANISOTROPIC;
    AddressU = WRAP;
    AddressV = WRAP;
	MaxAnisotropy = 16; //todo
};

samplerAnisotropic16Wrap
{
    Filter = ANISOTROPIC;
    AddressU = WRAP;
    AddressV = WRAP;
	MaxAnisotropy = 16;
};

samplerBilinearVolumeClamp
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};