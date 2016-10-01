
struct LightCalcOutput
{
    float3 diffuse;
	float3 specular;
};

///
struct PI_Pos
{
    float4 position			: SV_POSITION;   
};

struct VI_Pos
{
    float3 position			: POSITION;   
};

struct PI_PosTex
{
    float4 pos				: SV_POSITION;   
    float2 tex				: TEXCOORD;  
};

struct VI_PosTex
{
    float3 pos				: POSITION;
    float2 tex				: TEXCOORD;
};

struct VI_Mesh
{
    float3 position			: POSITION;
    float2 tex				: TEXCOORD0;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 binormal			: BINORMAL;
};

struct PI_Mesh
{
    float4 position			: SV_POSITION;
    float2 tex				: TEXCOORD0;
   	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 binormal			: BINORMAL;
	float4 worldPos			: POSITION;
};

struct PO_Gbuffer
{
    float4 albedo_roughY 	: SV_TARGET0;
	float4 tbn 				: SV_TARGET1;
	float2 vnormXY 			: SV_TARGET2;
	float4 spec_roughX 		: SV_TARGET3;
	float4 emiss_vnormZ 	: SV_TARGET4;
	uint id 				: SV_TARGET5;
	float4 subs_thick 		: SV_TARGET6;
	float ao 				: SV_TARGET7;
};

struct PI_ToolMesh
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
   	float3 normal : NORMAL;
	float4 worldPos : POSITION;
};

struct PO_faces
{
	float4 face0 : SV_TARGET0;
	float4 face1 : SV_TARGET1;
	float4 face2 : SV_TARGET2;
	float4 face3 : SV_TARGET3;
	float4 face4 : SV_TARGET4;
	float4 face5 : SV_TARGET5;
};

struct triple_vect
{
	float3 xdir;
	float3 ydir;
	float3 zdir;
};

struct MaterialParamsStructBuffer
{
	uint unlit;
	uint subscattering;
	float ss_distortion;
	float ss_direct_translucency;
	float ss_direct_pow;
	float ss_indirect_translucency;

	float padding[2];
};