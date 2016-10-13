TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "SSR";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

StructuredBuffer<MaterialParamsStructBuffer> MAT_PARAMS : register(t0);

Texture2D <float4> gb_normal : register(t1); 
Texture2D <float4> gb_roughnessX : register(t2); 
Texture2D <float4> gb_roughnessY : register(t3); 
Texture2D <float4> reflectData : register(t4); 
Texture2D <float2> hiz_depth : register(t5); 
Texture2D hiz_vis : register(t6);  
Texture2D <uint> gb_mat_obj : register(t7); 

SamplerState samplerPointClamp : register(s0);
SamplerState samplerTrilinearClamp : register(s1);
SamplerState samplerBilinearClamp : register(s2);

cbuffer CamMove : register(b1)
{
	float4x4 viewProjInv_ViewProjPrev;
};


#define MIN_REFL_DEPTH 0.00002
#define MAX_DEPTH_OFFSET 0.001
#define RAY_ITERATOR 128

#define MAX_RAY_DIST_SQ 32000.0f
#define MAX_RAY_DIST_FADE 5000.0f

#define BORDER_FADE 0.05f


float3 GetPrevPos(float3 pos) // screen space
{
	float4 curr_pos = 1;
	
	curr_pos.xy = pos.xy * 2.0f - 1.0f;
	curr_pos.y = -curr_pos.y;
	curr_pos.z = pos.z;
	
	float4 prevHomog = mul(curr_pos, viewProjInv_ViewProjPrev);
	prevHomog.xyz = prevHomog.xyz / prevHomog.w;
	
	float3 res = float3(prevHomog.xy * float2(0.5f, -0.5f) + float2(0.5f,0.5f), prevHomog.z);
	return res; 
}

float3 intersectCellBound(float3 o, float3 d, float2 cellIndex, float2 count, float2 step, float2 offset, bool is_negative)
{
	float2 index = cellIndex + step;
	index /= count;
	index += offset;
	
	float2 delta = index - o.xy;
	delta /= d.xy;

	float t;
	[flatten]
	if(is_negative) t = max(delta.x, delta.y);
	else t = min(delta.x, delta.y);

	return o + d * t;
}

float4 traceReflections( float3 p, float3 refl, float2 screenSize, float perspW )
{
	float level = 0;
	float iterator = 0;

	if( abs(refl.x) < 0.01 && abs(refl.y) < 0.01 )
		return 0;
	
	const bool is_negative = refl.z < 0;
	
	float2 crossOffset, crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : -1.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : -1.0f;
	crossOffset = crossStep * g_PixSize * 0.5;
	crossStep = saturate(crossStep);
	
	float3 ray = p;
	const float3 d = refl.xyz / refl.z;
	const float3 o = p - d * p.z;

	float2 rayCell = trunc(ray.xy * screenSize);
	ray = intersectCellBound(o, d, rayCell, screenSize, crossStep, crossOffset, is_negative);
	//ray.xy = (trunc(ray.xy * screenSize) + float2(0.5, 0.5)) / screenSize;

	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		const float levelExp = exp2(level);
	
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(ray.xy), level).rg;
		
		const float2 cellCount = trunc(screenSize / levelExp);
		const float2 oldCellId = trunc(ray.xy * cellCount);
		
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, MAX_DEPTH_OFFSET /* g_perspParam / minmaxZ.g*/);

		const float3 tmpRay = o + d * clamp( ray.z, minmaxZ.r, minmaxZ.g );
		const float2 newCellId = trunc(tmpRay.xy * cellCount);
		
		[branch]
		if( oldCellId.x != newCellId.x || oldCellId.y != newCellId.y )
		{
			ray = intersectCellBound(o, d, oldCellId, cellCount, crossStep, crossOffset, is_negative);
			//ray.xy = (trunc(ray.xy * screenSize) + float2(0.5, 0.5)) / screenSize;
			level++;
		}
		else
			level--;
		//if(iterator > 30)
		//	return 75;
		if(ray.x <= 0 || ray.y <= 0 || ray.x >= 1 || ray.y >= 1)
		{
			iterator = 0;
			break;
		}
		
		if(level >= g_hizMipCount)
		{
			iterator = 0;
			break;
		}
		else 
			++iterator;
	}

	return float4(ray, iterator);
}

float3 intersectCellBoundParall(float3 o, float3 d, float2 cellIndex, float2 count, float2 step, float2 offset)
{
	float2 index = cellIndex + step;
	index /= count;
	index += offset;
	
	float2 delta = index - o.xy;
	delta /= d.xy;

	float t = max(delta.x, delta.y);
	return o + d * t;
}

float4 traceReflectionsParall( float3 p, float3 refl, float2 screenSize )
{
	float level = 0;
	float iterator = 0;
	
	float2 crossOffset, crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : -1.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : -1.0f;
	crossOffset = crossStep * g_PixSize * 0.5;
	crossStep = saturate(crossStep);
	
	float3 ray = p;
	
	float2 temp_p = p.xy;
	if(refl.x<0)temp_p.x = 1.0f - temp_p.x;
	if(refl.y<0)temp_p.y = 1.0f - temp_p.y;
	
	float2 absRefl = abs(refl.xy);
	float2 delta = temp_p / absRefl;
	float diff = min(delta.x, delta.y);
	float3 o = p - refl * diff;
	
	temp_p = float2(1, 1) - temp_p;
	delta = temp_p / absRefl;
	diff += min(delta.x, delta.y);
	float3 d = refl * diff;
	
	float2 rayCell = trunc(ray.xy * screenSize);
	ray = intersectCellBoundParall(o, d, rayCell, screenSize, crossStep, crossOffset);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(ray.xy), level).rg;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, MAX_DEPTH_OFFSET);

		[branch]
		if( ray.z < minmaxZ.r || ray.z > minmaxZ.g )
		{
			float2 cellCount = trunc(screenSize / exp2(level));
			ray = intersectCellBoundParall(o, d, trunc(ray.xy * cellCount), cellCount, crossStep, crossOffset);
			level++;
		}
		else	
			level--;

		if(ray.x <= 0 || ray.y <= 0 || ray.x >= 1 || ray.y >= 1)
		{
			iterator = 0;
			break;
		}

		if(level >= g_hizMipCount)
		{
			iterator = 0;
			break;
		}
		else 
			++iterator;
	}
	
	return float4(ray, iterator);
}

//#include "ssr.hlsl" 

float4 calc_ssr( float3 p, float3 N, float3 WP, float2 screenSize, float R )
{
	float3 viewPos = mul(float4(WP, 1.0f), g_view).rgb;
	float perspW = viewPos.z * g_proj[2][3] + g_proj[3][3];

	float3 V_unnorm = g_CamPos - WP;
	float3 V = normalize(V_unnorm);
	float3 reflWS = reflect(-V, N);
	
	// correct reflection pos
	float refl_d = dot(g_CamDir, V_unnorm);
	float refl_e = dot(g_CamDir, reflWS);
	bool is_parallel = abs(refl_e) < 0.1f;

	if(!is_parallel)
		reflWS *= abs(refl_d / refl_e) * 0.8; 

	float4 posReflSS = mul(float4(WP + reflWS, 1.0f), g_viewProj);  
	float3 posReflSSvect = posReflSS.xyz / posReflSS.w; 
	posReflSSvect.xy = posReflSSvect.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);	
	float3 refl = posReflSSvect - p; 
	 	  
	float4 ray;    
	[branch]         
	if(is_parallel)  
	{
		ray = traceReflectionsParall( p, refl, screenSize );
	}
	else
	{    
		ray = traceReflections( p, refl, screenSize, perspW );
		//return ray.w / 75;
	}    

	if(ray.w == 0)
		return 0; 
		 
	float3 reflRay = WP - GetWPos(ray.xy, ray.z);
	float reflFade = 1 - saturate((dot(reflRay, reflRay) - MAX_RAY_DIST_SQ) / MAX_RAY_DIST_FADE);
	if(reflFade == 0)   
		return 0;      
	 
	float2 borderFade = saturate( (float2(1, 1) - abs(2 * (ray.xy - float2(0.5, 0.5))) ) / float2(BORDER_FADE, BORDER_FADE) );
	reflFade *= 0.5 * (sin((borderFade.x * borderFade.y - 0.5) * PI) + 1);
	 
	float4 totalColor = 0; 
		    
	float3 newSamplePos = GetPrevPos(ray);  
	totalColor.rgb = reflectData.Sample(samplerPointClamp, newSamplePos.xy).rgb;
	totalColor.a = 1;

	totalColor.a *= reflFade;

	totalColor.a *= saturate(1 - float(ray.w - 100) / 27);

	if(ray.w >= 1290)  
		return float4(0,1,hiz_vis.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(ray.xy), 4).r * R,1);//totalColor.a = 0;
	
	return totalColor;  
} 

float4 SSR(PI_PosTex input) : SV_TARGET 
{   
	float2 inUV = input.tex;      
		  
	int2 pixCoords = 0; 
	pixCoords.x = int(round(input.tex.x * g_maxScreenCoords.x));
	pixCoords.y = int(round(input.tex.y * g_maxScreenCoords.y));
	 
	const uint matID_objID = gb_mat_obj.Load(int3(pixCoords, 0));  
	const uint matID = GetMatID(matID_objID); 
	MaterialParamsStructBuffer params = MAT_PARAMS[matID];   
	if(params.unlit == 1)          
		return 0;  
	
	const float4 TBN = gb_normal.Sample(samplerPointClamp, inUV);  
	                
	float3 normal;      
	float3 tangent;               
	float3 binormal;            
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);           
	                 
	float depth = hiz_depth.Sample(samplerPointClamp, UVforSamplePow2(inUV)).r;       
	const float3 wpos = GetWPos(inUV, depth);    
	           
	float R_X = gb_roughnessX.Sample(samplerPointClamp, inUV).a;     
	float R_Y = gb_roughnessY.Sample(samplerPointClamp, inUV).a;  
	   
	float3 posSS = float3(inUV, depth);   
		   
	float4 ssr = calc_ssr(posSS, normal, wpos, float2(float(g_screenW), float(g_screenH)), (R_X + R_Y) * 0.5f);
	
	return ssr;
} 