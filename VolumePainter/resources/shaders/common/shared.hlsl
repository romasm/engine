shared cbuffer SharedBuffer : register(b0)
{
	matrix g_viewProj;
	matrix g_invViewProj;
	matrix g_view;
	matrix g_proj;

	matrix g_viewProjInv_ViewProjPrev;
	
	float3 g_CamPos;
	int g_screenW;
	
	int g_screenH;
	float3 g_CamDir;
	
	float g_time;
	float g_dt;
	float2 g_PixSize;

	float4 g_maxScreenCoords;
	
	float3 g_CamTangent;
	float g_hizMipCount;

	float3 g_CamBinormal;
	float g_perspParam;

	float2 g_uvCorrectionForPow2;
	float2 padding0;

	// 0 ---- 1
	// |      |
	// 2 ---- 3
	float3 g_CamFrust0;
	float g_nearMulFar;

	float3 g_CamFrust1;
	float g_far;

	float3 g_CamFrust2;
	float g_farMinusNear;

	float3 g_CamFrust3;
	float padding4;
};

float DepthToLinear(float d)
{
	return g_nearMulFar / (g_far - d * g_farMinusNear);
}

float4 WorldToClip(float4 worldCoords)
{
	return mul(worldCoords, g_viewProj);
}

float2 WorldToScreen(float4 worldCoords)
{
	return ClipToScreen( WorldToClip(worldCoords) );
}

float3 GetWPos(float2 uv, float depth)
{
	float4 position = 1.0f; 
	position.x = uv.x * 2.0f - 1.0f; 
	position.y = -(uv.y * 2.0f - 1.0f);
	position.z = depth; 
 
	position = mul(position, g_invViewProj); 
	position /= position.w;

	return position.xyz;
}

float3 GetCameraVector(float2 screen_uv)
{
	float3 horz0 = lerp(g_CamFrust0, g_CamFrust1, screen_uv.x);
	float3 horz1 = lerp(g_CamFrust2, g_CamFrust3, screen_uv.x);
	float3 res = lerp(horz0, horz1, screen_uv.y);
	return normalize(res);
}

// snaps a uv coord to the nearest texel centre
float2 SnapToScreenTexel(float2 screen_uv)
{
	return round(screen_uv * g_maxScreenCoords.xy) * g_maxScreenCoords.zw;
}

float2 UVforSamplePow2(float2 uv)
{
	return uv * g_uvCorrectionForPow2;
}

float2 PixelCoordsFromThreadID(uint2 threadID)
{
	return g_PixSize.xy * (float2(threadID) + 0.5f);
}