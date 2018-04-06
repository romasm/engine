#define BONE_PER_VERTEX_MAXCOUNT 8

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

struct VI_PosColor
{
    float3 pos				: POSITION;   
    float3 color			: TEXCOORD;   
};

struct PI_PosColor
{
    float4 pos				: SV_POSITION;   
    float3 color			: TEXCOORD;  
};

struct VI_Mesh
{
    float3 position			: POSITION;
    float2 tex				: TEXCOORD0;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 binormal			: BINORMAL;
};

struct VI_Skinned_Mesh
{
    float3 position			: POSITION;
    float2 tex				: TEXCOORD0;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 binormal			: BINORMAL;
	int boneID[BONE_PER_VERTEX_MAXCOUNT]	: TEXCOORD1;
	float boneWeight[BONE_PER_VERTEX_MAXCOUNT]	: TEXCOORD9;
};

struct GI_Mesh
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

struct PI_Mesh_Voxel
{
    sample float4 position			: SV_POSITION;
    sample float2 tex				: TEXCOORD0;
   	sample float3 normal			: NORMAL;
	sample float3 tangent			: TANGENT;
	sample float3 binormal			: BINORMAL;
	sample float4 voxelCoords		: POSITION;
	sample float3 worldPosition		: TEXCOORD1;
	nointerpolation int planeId	: TEXCOORD2;
	nointerpolation float trisRadiusSq	: TEXCOORD3;
	//nointerpolation uint level		: TEXCOORD3;
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

struct TripleVect
{
	float3 xdir;
	float3 ydir;
	float3 zdir;
};

TripleVect UVCoordsToCube(float2 uv)
{
	TripleVect res;
	
	float2 normUV = (uv - float2(0.5f, 0.5f)) * 2;
	
	res.xdir.z = -normUV.y * SQRT2DIV2;
	res.xdir.y = -normUV.x * SQRT2DIV2;
	res.xdir.x = SQRT2DIV2;
	res.xdir = normalize(res.xdir);
	
	res.ydir.z = -normUV.y * SQRT2DIV2;
	res.ydir.x = normUV.x * SQRT2DIV2;
	res.ydir.y = SQRT2DIV2;
	res.ydir = normalize(res.ydir);
	
	res.zdir.x = normUV.x * SQRT2DIV2;
	res.zdir.y = normUV.y * SQRT2DIV2;
	res.zdir.z = SQRT2DIV2;
	res.zdir = normalize(res.zdir);
	
	return res;
}

struct MaterialParams
{
	uint unlit;
	float ior;
	float asymmetry;
	float attenuation;
	float ssTint;

	float padding[3];
};

struct StmInstanceMatrix
{
	matrix worldMatrix;
	matrix normalMatrix;
};

struct VolumeData
{
	float3 cornerOffset;
	float worldSize;
		
	float scaleHelper;
	uint volumeRes;
	float voxelSize;
	float voxelSizeRcp;

	float voxelDiag;
	float voxelDiagRcp;
	float2 levelOffset;

	float3 volumeOffset;
	float worldSizeRcp;

	float2 levelOffsetTex;
	float _padding0;
	float _padding1;

	float3 prevFrameOffset;
	float _padding2;
};

struct VolumeTraceData
{
	uint maxLevel;
	uint levelsCount;
	float xVolumeSizeRcp;
	uint clipmapCount;
};

struct GISampleData
{
	float3 minCorner;
	float worldSizeRcp;
};

struct GBufferData
{
	float3 albedo;
	float3 reflectivity;
	float2 roughness;
	float3 emissive;
	float3 subsurf;
	float3 subsurfTint;
	float thickness;
	float ao;

	float3 normal;
	float3 tangent;
	float3 binormal;

	float3 wpos;
	float depth;
	float3 vertex_normal;
};

struct MediumData
{
	float insideRoughness;
	float opacity;
	float attenuation;
	float3 absorption;
	float thickness;
	float3 invIOR;
	float tirAmount;
	float frontDepth;
	float backDepth;
	float backDepthPersp;
	float3 backNormal;
};

struct DataForLightCompute
{
	float2 R;
	float avgR;
	float minR;
	float aGGX;
	float sqr_aGGX;
	float NoV;
	float3 reflect;
};

struct ConfigParams
{
	float dirDiff;
	float dirSpec;
	float indirDiff;
	float indirSpec;

	float isLightweight;
	float _padding0;
	float _padding1;
	float _padding2;
};