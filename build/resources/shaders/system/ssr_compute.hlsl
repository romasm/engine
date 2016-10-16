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
#define MAX_DEPTH_OFFSET 0.0025
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

float3 intersectCellBound(float3 o, float3 d, float2 cellIndex, float2 count, float2 step, float pixStep, bool is_negative)
{
	float2 index = cellIndex + step;
	index /= count;
	//index += offset;
	
	float2 delta = index - o.xy;
	delta /= d.xy;

	float t;
	[flatten]
	if(is_negative) t = (max(delta.x, delta.y) - /*0.000001*/pixStep);
	else t = (min(delta.x, delta.y) + pixStep);

	return o + d * t;
}

float4 traceReflections( float3 p, float3 refl, float2 screenSize, float perspW )
{
	float level = 0;
	float iterator = 0;
	float alpha = 1;

	float2 pixSize = rcp(screenSize);

	if( abs(refl.x) <= pixSize.x && abs(refl.y) <= pixSize.y )
		return 0;
	
	const bool is_negative = refl.z < 0;
	
	float2 crossOffset, crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : -1.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : -1.0f;
	//crossOffset = crossStep * g_PixSize * 0.5;
	//crossOffset = crossStep * 0.5 * pixSize;

	crossStep = saturate(crossStep);
	
	float3 ray = p;
	const float3 d = refl.xyz / refl.z;
	const float3 o = p - d * p.z;
			
	float2 d_pix = abs(pixSize / d.xy);

	float d_pixStep = min(1.0, min(d_pix.x, d_pix.y));
	d_pixStep *= 0.1;
	//return min(d.x, d.y) > 0.00000001;

	float2 rayCell = trunc(ray.xy * screenSize);
	ray = intersectCellBound(o, d, rayCell, screenSize, crossStep, d_pixStep, is_negative);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{	
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		
		const float2 cellCount = trunc(screenSize / exp2(level));
		const float2 oldCellId = trunc(ray.xy * cellCount);
		
		float minThickness = MAX_DEPTH_OFFSET * g_perspParam / perspW;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, minThickness);

		minThickness *= 0.5;
		alpha = ((ray.z - minmaxZ.r) - minThickness) / minThickness;

		const float3 tmpRay = o + d * clamp( ray.z, minmaxZ.r, minmaxZ.g );
		const float2 newCellId = trunc(tmpRay.xy * cellCount);
		
		[branch]
		if( oldCellId.x != newCellId.x || oldCellId.y != newCellId.y )
		{
			ray = intersectCellBound(o, d, oldCellId, cellCount, crossStep, d_pixStep, is_negative);
			level++;
		}
		else
			level--;
		
		[branch]
		if(level >= g_hizMipCount || ray.x <= 0 || ray.y <= 0 || 
			ray.x >= g_uvCorrectionForPow2.x || ray.y >= g_uvCorrectionForPow2.y)
		{
			alpha = 1;
			break;
		}
		
		++iterator;
	}
	
	alpha = 1 - saturate(alpha);
	alpha *= alpha;
	alpha *= saturate(1 - float(iterator - RAY_ITERATOR * 0.8) / (RAY_ITERATOR * 0.2));
	return float4(ray, alpha);
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
	crossOffset = crossStep * 0.5 / screenSize;
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
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
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

		if(ray.x <= 0 || ray.y <= 0 || ray.x >= g_uvCorrectionForPow2.x || ray.y >= g_uvCorrectionForPow2.y)
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
	bool is_parallel = abs(refl_e) < 0.3f;

	if(!is_parallel)
		reflWS *= abs(refl_d / refl_e) * 0.8; 

	float4 posReflSS = mul(float4(WP + reflWS, 1.0f), g_viewProj);  
	float3 posReflSSvect = posReflSS.xyz / posReflSS.w; 
	posReflSSvect.xy = posReflSSvect.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);	
	float3 refl = posReflSSvect - p; 
	 	  
	p.x *= g_uvCorrectionForPow2.x;
	p.y *= g_uvCorrectionForPow2.y;
	refl.x *= g_uvCorrectionForPow2.x;
	refl.y *= g_uvCorrectionForPow2.y;

	float2 correctedSceenSize = screenSize / g_uvCorrectionForPow2;

	float4 ray;    
	[branch]         
	if(is_parallel)  
	{
		ray = traceReflectionsParall( p, refl, correctedSceenSize );
		return 1;
	}
	else
	{    
		ray = traceReflections( p, refl, correctedSceenSize, perspW );
		//return ray;
	}    
	ray.xy /= g_uvCorrectionForPow2; 
	
	if(ray.w == 0)
		return 0; 
		 
	float3 reflRay = WP - GetWPos(ray.xy, ray.z);
	float reflFade = 1 - saturate((dot(reflRay, reflRay) - MAX_RAY_DIST_SQ) / MAX_RAY_DIST_FADE);
	if(reflFade == 0)   
		return 0;      
	 
	float3 newSamplePos = GetPrevPos(ray);  

	float2 borderDetectionPrev = 2 * abs(newSamplePos.xy - float2(0.5, 0.5));
	float2 borderDetectionCurr = 2 * abs(ray.xy - float2(0.5, 0.5));
	borderDetectionCurr = max(borderDetectionCurr, borderDetectionPrev);

	float2 borderFade = saturate( (float2(1, 1) - borderDetectionCurr ) / float2(BORDER_FADE, BORDER_FADE) );
	reflFade *= 0.5 * (sin((borderFade.x * borderFade.y - 0.5) * PI) + 1);
	 
	float4 totalColor = 0;
	totalColor.rgb = reflectData.Sample(samplerTrilinearClamp, newSamplePos.xy).rgb;
	totalColor.a = ray.a;

	totalColor.a *= reflFade;

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