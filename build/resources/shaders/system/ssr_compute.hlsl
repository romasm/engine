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

float3 intersectCellBoundParall(float3 o, float3 d, float2 cellIndex, float2 count, float2 step, float2 pixStep, bool is_negative)
{
	float2 index = cellIndex + step;
	index /= count;
	//index += offset;
	
	float2 delta = index - o.xy;
	delta /= d.xy;

	//float t = min(delta.x, delta.y) + pixStep;
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
	
	const bool is_negative = false;//refl.z < 0;

	float2 crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : 0.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : 0.0f;
	
	float3 ray = p;
	const float3 d = refl.xyz / refl.z;
	const float3 o = p - d * p.z;

	float2 temp_p = p.xy;
	if(refl.x<0)temp_p.x = 1.0f - temp_p.x;
	if(refl.y<0)temp_p.y = 1.0f - temp_p.y;
	
	float2 absRefl = abs(refl.xy);
	float2 delta = temp_p / absRefl;
	float diff = min(delta.x, delta.y);
	float3 o_inbox = p - refl * diff;
	
	temp_p = float2(1, 1) - temp_p;
	delta = temp_p / absRefl;
	diff += min(delta.x, delta.y);
	float3 d_inbox = refl * diff;

	if(o_inbox.z < 0.0)
		o_inbox = o;

	if(d_inbox.z > 1.0)
		d_inbox = d;
		/*	
	// box intersect
	float boundParams[4];
	boundParams[0] = - o.x / d.x;
	boundParams[1] = (1 - o.x) / d.x;
	boundParams[2] = - o.y / d.y;
	boundParams[3] = (1 - o.y) / d.y;

	[unrool]
	for(int i=0; i<3; i++)
	{
		for(int i=0; i<3; i++)
		{
			if(boundParams[i] > boundParams[i+1])
			{
				float temp = boundParams[i];
				boundParams[i] = boundParams[i+1];
				boundParams[i+1] = temp;
			}
		}
	}

	float3 o_inbox = o + d * boundParams[1];
	if(o_inbox.z < 0.0)
		o_inbox = o;

	float3 d_inbox = o + d * boundParams[2];
	if(d_inbox.z > 1.0)
		d_inbox = o + d;
	d_inbox -= o_inbox;
	*/

	// box intersect
	
	float2 d_pix = abs(pixSize / d_inbox.xy);

	float d_pixStep = min(1.0, min(d_pix.x, d_pix.y));
	d_pixStep *= 0.1;

	float2 rayCell = trunc(ray.xy * screenSize);
	ray = intersectCellBoundParall(o_inbox, d_inbox, rayCell, screenSize, crossStep, d_pixStep, is_negative);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		
		const float2 cellCount = trunc(screenSize / exp2(level));
		const float2 oldCellId = trunc(ray.xy * cellCount);

		float minThickness = MAX_DEPTH_OFFSET * g_perspParam / perspW;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, minThickness);

		//minThickness *= 0.5;
		alpha = ((ray.z - minmaxZ.r) - minThickness * 0.15) / (minThickness * 0.85);

		//const float3 tmpRay = o_inbox + d_inbox * max(0.0, clamp( ray.z, minmaxZ.r, minmaxZ.g ) - o_inbox.z );
		const float3 tmpRay = o + d * clamp( ray.z, minmaxZ.r, minmaxZ.g );
		//return o.z + d.z == 1.0;//float4(, 1);
		const float2 newCellId = trunc(tmpRay.xy * cellCount);

		[branch]
		if( oldCellId.x != newCellId.x || oldCellId.y != newCellId.y)
		{
			ray = intersectCellBoundParall(o_inbox, d_inbox, oldCellId, cellCount, crossStep, d_pixStep, is_negative);
			level++;
		}
		else
		{
			level--;
			[branch]
			if(level < 0)
			{
				ray.xy = (float2(oldCellId) + float2(0.5, 0.5)) * pixSize;
				break;
			}
		}

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
	alpha *= alpha;
	alpha *= saturate(1 - float(iterator - RAY_ITERATOR * 0.8) / (RAY_ITERATOR * 0.2));
	return float4(ray, alpha);
}

float4 traceReflectionsParall( float3 p, float3 refl, float2 screenSize, float perspW )
{
	float level = 0;
	float iterator = 0;
	float alpha = 1;
	
	float2 pixSize = rcp(screenSize);

	if( abs(refl.x) <= pixSize.x && abs(refl.y) <= pixSize.y )
		return 0;
	
	float2 crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : 0.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : 0.0f;
	
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
	
	float2 d_pix = abs(pixSize / d.xy);

	float d_pixStep = min(1.0, min(d_pix.x, d_pix.y));
	d_pixStep *= 0.1;

	float2 rayCell = trunc(ray.xy * screenSize);
	ray = intersectCellBoundParall(o, d, rayCell, screenSize, crossStep, d_pixStep, false);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		
		float minThickness = MAX_DEPTH_OFFSET * g_perspParam / perspW;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, minThickness);

		//minThickness *= 0.5;
		alpha = ((ray.z - minmaxZ.r) - minThickness * 0.05) / (minThickness * 0.95);

		[branch]
		if( ray.z < minmaxZ.r || ray.z > minmaxZ.g )
		{
			float2 cellCount = trunc(screenSize / exp2(level));
			ray = intersectCellBoundParall(o, d, trunc(ray.xy * cellCount), cellCount, crossStep, d_pixStep, false);
			level++;
		}
		else	
		{
			level--;
			[branch]
			if(level < 0)
			{
				ray.xy = (trunc(ray.xy * screenSize) + float2(0.5, 0.5)) * pixSize;
				break;
			}
		}

		[branch]
		if(level >= g_hizMipCount || ray.x <= 0 || ray.y <= 0 || 
			ray.x >= g_uvCorrectionForPow2.x || ray.y >= g_uvCorrectionForPow2.y)
		{
			alpha = 1;
			break;
		}
		
		++iterator;
	}
	//return iterator / 100;
	alpha = 1 - saturate(alpha);
	alpha *= alpha;
	alpha *= alpha;
	alpha *= saturate(1 - float(iterator - RAY_ITERATOR * 0.8) / (RAY_ITERATOR * 0.2));
	return float4(ray, alpha);
}

float isoscelesTriangleInRadius(float a, float h)
{
	float a2 = a * a;
	float fh2 = 4.0f * h * h;
	return (a * (sqrt(a2 + fh2) - a)) / (4.0f * max(h, 0.00001f));
}

float4 calc_ssr( float3 p, float3 N, float3 WP, float2 screenSize, float R )
{
	//if(p.z > 0.999)
	//	return 0;

	float3 viewPos = mul(float4(WP, 1.0f), g_view).rgb;
	float perspW = viewPos.z * g_proj[2][3] + g_proj[3][3];

	if(viewPos.z > 50.0)
		return 0;

	float distFade = pow(1 - saturate((viewPos.z - 30.0) / 20.0), 2);

	float3 V_unnorm = g_CamPos - WP;
	float3 V = normalize(V_unnorm);

	if(dot(V, N) <= 0.0)
		return 0;

	float3 reflWS = reflect(-V, N);
	
	// correct reflection pos
	float refl_d = dot(g_CamDir, V_unnorm);
	float refl_e = dot(g_CamDir, reflWS);
	bool is_parallel = abs(refl_e) < 0.05f;

	if(!is_parallel)
		reflWS *= abs(refl_d / refl_e) * 0.8; 

	float4 posReflSS = mul(float4(WP + reflWS, 1.0f), g_viewProj);  
	float3 posReflSSvect = posReflSS.xyz / posReflSS.w; 
	posReflSSvect.xy = posReflSSvect.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);	
	float3 refl = posReflSSvect - p; 
	 	
	float3 p_corr = p;
	p_corr.x *= g_uvCorrectionForPow2.x;
	p_corr.y *= g_uvCorrectionForPow2.y;
	refl.x *= g_uvCorrectionForPow2.x;
	refl.y *= g_uvCorrectionForPow2.y;

	float2 correctedSceenSize = screenSize / g_uvCorrectionForPow2;

	float4 ray;    
	[branch]         
	if(is_parallel)  
	{
		ray = traceReflectionsParall( p_corr, refl, correctedSceenSize, perspW );
		//return 1;
	}
	else
	{   
		ray = traceReflections( p_corr, refl, correctedSceenSize, perspW );
		//return ray;		
	}    
	ray.xy /= g_uvCorrectionForPow2; 
	
	//if(ray.w == 0)
	//	return 0; 
		 
	float3 reflRay = WP - GetWPos(ray.xy, ray.z);

	const float4 TBN = gb_normal.Sample(samplerPointClamp, ray.xy);              
	float3 normal;      
	float3 tangent;               
	float3 binormal;            
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);  

	if(!any(normal))
		ray.a = 0;

	float NoR = dot(normal, normalize(reflRay));
	//if(NoR <= 0)
	//	ray.a = 0;

	//reflFade = saturate(reflFade * 100);
	//return reflFade;
	float reflFade = 1 - saturate((dot(reflRay, reflRay) - MAX_RAY_DIST_SQ) / MAX_RAY_DIST_FADE);
	//if(reflFade == 0)   
	//	return 0;      
	 
	float3 newSamplePos = GetPrevPos(ray);  

	float2 borderDetectionPrev = 2 * abs(newSamplePos.xy - float2(0.5, 0.5));
	float2 borderDetectionCurr = 2 * abs(ray.xy - float2(0.5, 0.5));
	borderDetectionCurr = max(borderDetectionCurr, borderDetectionPrev);

	float2 borderFade = saturate( (float2(1, 1) - borderDetectionCurr ) / float2(BORDER_FADE, BORDER_FADE) );
	reflFade *= 0.5 * (sin((borderFade.x * borderFade.y - 0.5) * PI) + 1);
	 
	//float4 totalColor = 0;
	//totalColor.rgb = reflectData.SampleLevel(samplerTrilinearClamp, newSamplePos.xy, 0).rgb;
	//totalColor.a = ray.a;
	

	// CONE TRACING
	float coneThetaHalf = acos(sqrt( (1 - 0.5) / ( 1 + (R*R*R*R - 1) * 0.5 ) ));
		
	float2 toReflPos = ray.xy - p.xy; // 2d or 3d? screen space or view space?
	
	float adjacentLength = length(toReflPos.xy);
	const float startLength = adjacentLength;
	
	float2 adjacentUnit = normalize(toReflPos.xy);
	
	// angle perspective correction
	float tanThetaHalf = tan(coneThetaHalf);
	//float incircleSize = tanThetaHalf * adjacentLength;
	float worldSize = tanThetaHalf * length(reflRay);
	float incircleSize = worldSize * g_perspParam / perspW;
	const float startSize = incircleSize;
		
	// cone-tracing using an isosceles triangle to approximate a cone in screen space
	float3 samplePos = 0;
	samplePos.z = ray.z;
	
	float4 totalColor = 0;
	
	samplePos.xy = p.xy + adjacentUnit * adjacentLength;
	float mipChannel = log2(2.0f * incircleSize * max(screenSize.x, screenSize.y)); // try this with min intead of max
	mipChannel = max(0, mipChannel);

	float3 prevSamplePos = GetPrevPos(samplePos);
	totalColor.rgb = reflectData.SampleLevel(samplerTrilinearClamp, prevSamplePos.xy, mipChannel).rgb;
	if(mipChannel >= 1)
	{
		float visCurrent = hiz_vis.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(samplePos.xy), mipChannel - 1).r;
		visCurrent = pow(visCurrent * 10, 1.1);//saturate(visCurrent * 10.0);
		visCurrent = saturate(visCurrent);
		totalColor.a = visCurrent * ray.a;
	}
	else
	{
		totalColor.a = ray.a;
		float visZero = hiz_vis.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(samplePos.xy), 0).r;
		visZero = pow(visZero * 10, 1.1);//saturate(visZero * 10);
		visZero = saturate(visZero);
		totalColor.a = lerp(totalColor.a, visZero * ray.a, mipChannel);
	}
	totalColor.a = ray.a;
	totalColor.a *= 1 - (sin(saturate((mipChannel - 3.5) / 4.0) * PI - PIDIV2) * 0.5 + 0.5);
	//totalColor.a = 1;
	//totalColor.a = ray.a;
	
	/*[unroll]
	for(int i = 0; i < 7; ++i)
	{
		adjacentLength = adjacentLength - incircleSize * 1.5;
		if(adjacentLength <= 0)
			break;

		incircleSize = lerp(startSize, 0.0, adjacentLength / startLength);
		
		samplePos.z = lerp(p.z, ray.z, adjacentLength / startLength);
		samplePos.xy = p.xy + adjacentUnit * adjacentLength;

		float mipChannel = log2(2.0f * incircleSize * max(screenSize.x, screenSize.y));
		
		float2 minmaxZ = hiz_depth.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(samplePos.xy), mipChannel).rg;
		float minThickness = MAX_DEPTH_OFFSET * g_perspParam / perspW;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, minThickness);

		[branch]
		if( ray.z >= minmaxZ.r && ray.z <= minmaxZ.g )
		{
			float4 color = 0;
			float3 prevSamplePos = GetPrevPos(samplePos);
			color.rgb = reflectData.SampleLevel(samplerTrilinearClamp, prevSamplePos.xy, mipChannel).rgb;
			//if(ray.x != 0)
				//return mipChannel;

			[branch]
			if(mipChannel > 0)
				color.a = hiz_vis.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(samplePos.xy), mipChannel - 1).r;
			else
				color.a = 1.0;
		
			totalColor.rgb = lerp(totalColor.rgb, color.rgb, color.a);
			totalColor.a = lerp(totalColor.a, 1.0, color.a);
		}
		
		[branch]
		if(mipChannel < 0.5f)
			break;
	}*/
	
	///////////
	//totalColor.a = 1;

	totalColor.a *= reflFade;

	//if(ray.w == 0)  
	//	return float4(0,1,hiz_vis.SampleLevel(samplerTrilinearClamp, UVforSamplePow2(ray.xy), 4).r * R,0);//totalColor.a = 0;
	
	return totalColor * distFade;  
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
		   
	float avgR = (R_X + R_Y) * 0.5f;
	[branch]
	if(avgR > 0.4)
		return 0;

	float4 ssr = calc_ssr(posSS, normal, wpos, float2(float(g_screenW), float(g_screenH)), avgR);

	float FadeR = 1 - saturate((avgR - 0.3) / 0.1);
	
	return ssr * FadeR;
} 