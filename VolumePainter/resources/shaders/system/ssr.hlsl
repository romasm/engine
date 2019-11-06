/*
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
}*/

// ray tracing

float3 intersectDepthPlane(float3 o, float3 d, float t)
{
	return o + d * t;
}

float2 getCell(float2 r, float2 count)
{
	return trunc(r * count);
}
/*
float3 intersectCellBound(float3 o, float3 d, float2 cellIndex, float2 count, float2 step, float2 offset)
{
	float2 index = cellIndex + step;
	index /= count;
	index += offset;
	
	float2 delta = index - o.xy;
	delta /= d.xy;
	float t = min(delta.x, delta.y);
	return o + d * t;
}

float4 traceReflections( float3 p, float3 refl, float2 screenSize, float mipCount )
{
	float level = 0;
	float iterator = 0;
	
	const float2 cellCount_0 = screenSize;
	
	float2 crossOffset, crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : -1.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : -1.0f;
	crossOffset = crossStep * g_PixSize * 0.5;
	crossStep = saturate(crossStep);
	
	float3 ray = p;
	float3 d = refl.xyz / refl.z;
	float3 o = p - d * p.z;

	float2 rayCell = trunc(ray.xy * cellCount_0);
	ray = intersectCellBound(o, d, rayCell, cellCount_0, crossStep, crossOffset);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		const float levelExp = exp2(level);
	
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		
		const float2 cellCount = trunc(screenSize / levelExp);
		const float2 oldCellId = trunc(ray.xy * cellCount);
		
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, 0.006);

		float3 tmpRay = o + d * clamp( ray.z, minmaxZ.r, minmaxZ.g );
		
		const float2 newCellId = trunc(tmpRay.xy * cellCount);
		
		[branch]
		if( oldCellId.x != newCellId.x || oldCellId.y != newCellId.y )
		{
			ray = intersectCellBound(o, d, oldCellId, cellCount, crossStep, crossOffset);
			level += 2.0f;
		}
		
		//ray = tmpRay;

		if(ray.x <= 0 || ray.y <= 0 || ray.x >= 1 || ray.y >= 1)
			break;

		--level;
		
		if(level >= mipCount)
			break;
		else ++iterator;
	}

	return float4(ray, iterator);
}

float4 traceReflectionsInverse( float3 p, float3 refl, float2 screenSize, float mipCount )
{
	float level = 0;
	float iterator = 0;
	
	float2 crossOffset, crossStep;
	crossStep.x = (refl.x>=0) ? 1.0f : -1.0f;
	crossStep.y = (refl.y>=0) ? 1.0f : -1.0f;
	crossOffset = crossStep * g_PixSize * 0.5;
	crossStep = saturate(crossStep);
	
	float3 ray = p;
	float3 d = refl.xyz / (-refl.z);
	float3 o = intersectDepthPlane(p, d, p.z - 1.0f);
	
	float2 rayCell = getCell(ray.xy, screenSize);
	ray = intersectCellBound(o, d, rayCell, screenSize, crossStep, crossOffset);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		const float levelExp = exp2(level);
	
		float2 minmaxZ = HIZBUFFER.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		
		const float2 cellCount = trunc(screenSize / levelExp);
		const float2 oldCellId = getCell(ray.xy, cellCount);
		
		float3 tmpRay = intersectDepthPlane(o, d, 1.0f - clamp( ray.z, minmaxZ.r, minmaxZ.g + MAX_DEPTH_OFFSET_INVERSE ));
		
		const float2 newCellId = getCell(tmpRay.xy, cellCount);
		
		[branch]
		if( oldCellId.x != newCellId.x || oldCellId.y != newCellId.y )
		{
			tmpRay = intersectCellBound(o, d, oldCellId, cellCount, crossStep, crossOffset);
			level += 2.0f;
		}
		
		ray = tmpRay;
		--level;
		if(level >= mipCount)iterator = RAY_ITERATOR;
		else ++iterator;
	}
	
	return float4(ray, iterator);
}*/

float4 traceReflectionsParall( float3 p, float3 refl, float2 screenSize, float mipCount )
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
	float3 o = intersectDepthPlane(p, refl, -diff);
	
	temp_p = float2(1, 1) - temp_p;
	delta = temp_p / absRefl;
	diff += min(delta.x, delta.y);
	float3 d = refl * diff;
	
	float2 rayCell = getCell(ray.xy, screenSize);
	ray = intersectCellBound(o, d, rayCell, screenSize, crossStep, crossOffset, false);
	
	[loop]
	while( level >= 0 && iterator < RAY_ITERATOR )
	{
		float2 minmaxZ = hiz_depth.SampleLevel(samplerPointClamp, ray.xy, level).rg;
		minmaxZ.g = minmaxZ.r + max(minmaxZ.g - minmaxZ.r, MAX_DEPTH_OFFSET);

		[branch]
		if( ray.z < minmaxZ.r || ray.z > minmaxZ.g )
		{
			float2 cellCount = trunc(screenSize / exp2(level));
			ray = intersectCellBound(o, d, getCell(ray.xy, cellCount), cellCount, crossStep, crossOffset, false);
			level += 2.0f;
		}
		
		--level;
		if(level >= mipCount)break;
		else ++iterator;
	}
	
	return float4(ray, iterator);
}

// cone tracing

#define coneCosParam 0.5
#define coneCosParamInv 1 - coneCosParam
float roughnessToConeHalfAngleCos(float R)
{
	return sqrt( coneCosParamInv / ( 1 + (R*R*R*R - 1) * coneCosParam ) );
}

float isoscelesTriangleInRadius(float a, float h)
{
	float a2 = a * a;
	float fh2 = 4.0f * h * h;
	return (a * (sqrt(a2 + fh2) - a)) / (4.0f * max(h, 0.00001f));
}

float4 coneSampleWeightedColorFirst(float3 samplePos, float mipChannel, float radius)
{
	float3 newSamplePos = GetPrevPos(samplePos);
	float4 sampleColor;
	sampleColor.rgb = reflectData.SampleLevel(samplerTrilinearClamp, newSamplePos.xy, mipChannel).rgb;
	
	/*float pointMip = round(mipChannel);
	float visValue1 = hiz_vis.SampleLevel(samplerPointClamp, coords[0], pointMip - 1).r;
	float visValue2 = hiz_vis.SampleLevel(samplerPointClamp, coords[1], pointMip - 1).r;
	float visValue3 = hiz_vis.SampleLevel(samplerPointClamp, coords[2], pointMip - 1).r;
	float visValue4 = hiz_vis.SampleLevel(samplerPointClamp, coords[3], pointMip - 1).r;
	sampleColor.a = 0.25f * (visValue1 + visValue2 + visValue3 + visValue4);*/
	sampleColor.a = hiz_vis.SampleLevel(samplerTrilinearClamp, samplePos.xy, mipChannel).r;

	return sampleColor;
}

float4 coneSampleWeightedColor(float3 samplePos, float mipChannel, float radius)
{
	float pointMip = round(mipChannel);
	const float2 coords[4] = 
	{
		samplePos.xy + float2(radius, 0),
		samplePos.xy + float2(0, radius),
		samplePos.xy - float2(radius, 0),
		samplePos.xy - float2(0, radius)
	};
	
	float2 hizMinMax1 = hiz_depth.SampleLevel(samplerPointClamp, coords[0], pointMip).rg;
	float2 hizMinMax2 = hiz_depth.SampleLevel(samplerPointClamp, coords[1], pointMip).rg;
	float2 hizMinMax3 = hiz_depth.SampleLevel(samplerPointClamp, coords[2], pointMip).rg;
	float2 hizMinMax4 = hiz_depth.SampleLevel(samplerPointClamp, coords[3], pointMip).rg;
		
	float minz = min(min(hizMinMax1.r, hizMinMax2.r), min(hizMinMax3.r, hizMinMax4.r));
	float maxz = max(max(hizMinMax1.g, hizMinMax2.g), max(hizMinMax3.g, hizMinMax4.g)) + MAX_DEPTH_OFFSET;

	float3 sampleColor = 0;
	float sampleVis = 0;
	
	if(samplePos.z <= maxz && samplePos.z >= minz)
	{
		if(pointMip == 0)
			sampleVis = 1;
		else
		{
			/*float visValue1 = hiz_vis.SampleLevel(samplerPointClamp, coords[0], pointMip - 1).r;
			float visValue2 = hiz_vis.SampleLevel(samplerPointClamp, coords[1], pointMip - 1).r;
			float visValue3 = hiz_vis.SampleLevel(samplerPointClamp, coords[2], pointMip - 1).r;
			float visValue4 = hiz_vis.SampleLevel(samplerPointClamp, coords[3], pointMip - 1).r;
			sampleVis = 0.25f * (visValue1 + visValue2 + visValue3 + visValue4);*/
			sampleVis = hiz_vis.SampleLevel(samplerTrilinearClamp, samplePos.xy, mipChannel).r;
		}
	
		float3 newSamplePos = GetPrevPos(samplePos);
		sampleColor = reflectData.SampleLevel(samplerTrilinearClamp, newSamplePos.xy, mipChannel).rgb;
	}
	
	return float4(sampleColor, sampleVis);

	/*
	
	// hemisphere visibility: 2/pi * ( arccos(L/r) - L/r * sin(arccos(L/r)) )

		float2 alpha = 1 - saturate((newSamplePos.xy - float2(0.95, 0.95)) * 20.0);
		alpha *= saturate(newSamplePos.xy * 20.0);
*/
}

// main

float4 calcSSR( float3 p, float3 N, float3 WP, float2 screenSize, float mipCount, float R )
{
	float3 V_unnorm = g_CamPos - WP;
	float3 V = normalize(V_unnorm);
	float3 reflWS = reflect(-V, N);
	
	// correct reflection pos
	float refl_d = dot(g_CamDir, V_unnorm);
	float refl_e = dot(g_CamDir, reflWS);

	if(abs(refl_e) >= 0.001f)
		reflWS *= abs(refl_d / refl_e) * 0.8;

	float4 posReflSS = mul(float4(WP + reflWS, 1.0f), g_viewProj);
	float3 posReflSSvect = posReflSS.xyz / posReflSS.w;
	posReflSSvect.xy = posReflSSvect.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);	
	float3 refl = posReflSSvect - p;
	
	//float3 d = refl.xyz / refl.z;
	//return posReflSS.w < 0;

	float4 reflVS = normalize(mul(float4(reflWS, 0.0f), g_view));
	float3 reflProjSrceen = float3(reflVS.xy, 0);
	float cosReflScreen = length(reflProjSrceen);

	float3 tmul = 1;

	float4 ray;
	[branch]
	if(abs(refl.z) < 0.0001)
		return 1;//ray = traceReflectionsParall( p, refl, screenSize, mipCount );
	else
		ray = traceReflections( p, refl, screenSize, mipCount );
	
	//if(ray.w < 0)return float4(ray.xyz,1);
	
	float3 reflRay = WP - GetWPos(ray.xy, ray.z);
	float reflFade = 1 - saturate((dot(reflRay, reflRay) - MAX_RAY_DIST_SQ) / MAX_RAY_DIST_FADE);
	if(reflFade == 0)
		return 0;

	float2 borderFade = saturate( (float2(1, 1) - abs(2 * (ray.xy - float2(0.5, 0.5))) ) / float2(BORDER_FADE, BORDER_FADE) );
	reflFade *= 0.5 * (sin((borderFade.x * borderFade.y - 0.5) * PI) + 1);
	
	//float coneThetaHalf = acos(roughnessToConeHalfAngleCos(R));
	float coneThetaHalf = acos(sqrt( (1 - 0.5) / ( 1 + (R*R*R*R - 1) * 0.5 ) ));
		
	float2 toReflPos = ray.xy - p.xy; // 2d or 3d? screen space or view space?
	
	float adjacentLength = length(toReflPos.xy);
	const float startLength = adjacentLength;
	
	float2 adjacentUnit = normalize(toReflPos.xy);
	
	// angle perspective correction
	float incircleSize = tan(coneThetaHalf) * adjacentLength / max(cosReflScreen, 0.01);
	//float incircleSize = tan(coneThetaHalf) * adjacentLength;
	float tanThetaHalf = incircleSize / adjacentLength;
	
	float centerOffset = 0;
		
	// cone-tracing using an isosceles triangle to approximate a cone in screen space
	float3 samplePos = 0;
	samplePos.z = ray.z;
	
	float4 totalColor = 0;
	
	samplePos.xy = p.xy + adjacentUnit * adjacentLength;
	float mipChannel = log2(2.0f * incircleSize * max(screenSize.x, screenSize.y)); // try this with min intead of max
	totalColor = coneSampleWeightedColorFirst(samplePos, mipChannel, incircleSize);
	totalColor.a = pow(totalColor.a, 0.25);

	/*[unroll]
	for(int i = 0; i < 7; ++i)
	{
		adjacentLength = adjacentLength - centerOffset - incircleSize;
		float oppositeLength = 2.0f * tanThetaHalf * adjacentLength;
		
		incircleSize = isoscelesTriangleInRadius(oppositeLength, adjacentLength);
		centerOffset = incircleSize;
		adjacentLength -= centerOffset;
		
		samplePos.z = lerp(p.z, ray.z, adjacentLength / startLength);
		samplePos.xy = p.xy + adjacentUnit * adjacentLength;

		float mipChannel = log2(2.0f * incircleSize * max(screenSize.x, screenSize.y));

		float4 color = coneSampleWeightedColor(samplePos, mipChannel, incircleSize);
		
		totalColor.rgb = lerp(totalColor.rgb, color.rgb, color.a);
		
		[branch]
		if(mipChannel < 0.5f)
			break;
	}*/
	float3 newSamplePos = GetPrevPos(ray);
	totalColor.rgb = reflectData.SampleLevel(samplerTrilinearClamp, newSamplePos.xy, 0).rgb;
	totalColor.a = 1;

	totalColor.a *= reflFade;

	if(ray.w >= 127)totalColor.a = 0;
	
	return totalColor;
}
