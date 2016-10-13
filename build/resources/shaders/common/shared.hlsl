shared cbuffer SharedBuffer : register(b0)
{
	matrix g_viewProj;
	matrix g_invViewProj;
	matrix g_view;
	matrix g_proj;
	
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
	float2 padding1;
};

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

// snaps a uv coord to the nearest texel centre
float2 SnapToScreenTexel(float2 screen_uv)
{
	return round(screen_uv * g_maxScreenCoords.xy) * g_maxScreenCoords.zw;
}

float2 UVforSamplePow2(float2 uv)
{
	return uv * g_uvCorrectionForPow2;
}