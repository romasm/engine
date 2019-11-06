
#ifndef ORTHO_SHADOW

float ShadowToLinear(float d, float4 farNear)
{
	return farNear.z / (farNear.y - d * farNear.w);
}

float4 ShadowToLinear(float4 d, float4 farNear)
{
	return farNear.z / (farNear.y - d * farNear.w);
}

float2 horzPCF(float4 horzSamples[3], float uvFracX)
{
	float uvFravXInv = 1.0 - uvFracX;
	
	float2 res;

	res.x = horzSamples[0].w * uvFravXInv;
	res.y = horzSamples[0].x * uvFravXInv;
	res.x += horzSamples[0].z;
	res.y += horzSamples[0].y;

	res.x += horzSamples[1].w;
	res.y += horzSamples[1].x;
	res.x += horzSamples[1].z;
	res.y += horzSamples[1].y;

	res.x += horzSamples[2].w;
	res.y += horzSamples[2].x;
	res.x += horzSamples[2].z * uvFracX;
	res.y += horzSamples[2].y * uvFracX;

	return res;
}

#endif	

float2 
#ifdef ORTHO_SHADOW
	PCF_Filter_Ortho
#else
	PCF_Filter
#endif
(sampler samp, Texture2DArray <float> shadowmap, float3 UV, float depth, in GBufferData gbuffer, 
#ifdef ORTHO_SHADOW
	float farClip, 
#else
	float4 farNear, 
#endif			 
bool scatter)
{
	float2 uvInTexels = UV.xy * float2(SHADOWS_BUFFER_RES, SHADOWS_BUFFER_RES) - 0.5f;
	float2 uvFrac = frac(uvInTexels);
	float2 texelPos = floor(uvInTexels);

	float3 shadowCoords = float3( (texelPos + 0.5f) * PCF_PIXEL, UV.z );
	
	float2 vertSamples[2][3];

	[unroll]
	for(int i = -1; i <= 1; i++)
	{
		float4 horzSamples[2][3];

		[unroll]
		for(int j = -1; j <= 1; j++)
		{
			float4 shadowSample = shadowmap.Gather( samp, shadowCoords, int2(j, i) * 2 );
			float4 diff = depth - shadowSample;

			horzSamples[0][j + 1] = clamp( -diff * PCF_DEPTH_TEST_SENCE + 1.0f, 0.0f, 2.0f/*acne fading*/ );

			[branch]
			if(scatter)// TODO: bbox scale occluding
			{
			#ifdef ORTHO_SHADOW
				// 0.01 m bias, TODO?
				horzSamples[1][j + 1] = saturate( exp(-max(diff * farClip - 0.01, 0)) );
			#else
				// 0.01 m bias, TODO?
				float linDepth = ShadowToLinear(depth, farNear) - 0.01;
				float4 linShadow = ShadowToLinear(shadowSample, farNear);
				horzSamples[1][j + 1] = saturate( exp(-max(linDepth - linShadow, 0)) );
			#endif

			}
		}
		vertSamples[0][i + 1] = horzPCF(horzSamples[0], uvFrac.x);

		[branch]
		if(scatter)
		{
			vertSamples[1][i + 1] = horzPCF(horzSamples[1], uvFrac.x);
		}
	}

	float shadow = vertSamples[0][0].x * (1 - uvFrac.y) + vertSamples[0][0].y;
	shadow += vertSamples[0][1].x + vertSamples[0][1].y;
	shadow += vertSamples[0][2].x + vertSamples[0][2].y * uvFrac.y;

	float shadowExp = 0;
	[branch]
	if(scatter)
	{
		shadowExp = vertSamples[1][0].x * (1 - uvFrac.y) + vertSamples[1][0].y;
		shadowExp += vertSamples[1][1].x + vertSamples[1][1].y;
		shadowExp += vertSamples[1][2].x + vertSamples[1][2].y * uvFrac.y;
	}

	return saturate(float2(shadow, shadowExp) * 0.04f);
}

// -------------------------------------------------------------------------------
/*
float GatherFilter(sampler samp, Texture2DArray <float> shadowmap, float3 UV, float2 reprojUV, float halfPix, float depth)  
{  
	float4 shadMapDepth = shadowmap.GatherRed(samp, UV);
	float4 compare = float4(shadMapDepth.x < depth,
							shadMapDepth.y < depth,
							shadMapDepth.z < depth,
							shadMapDepth.w < depth);

	if(!any(compare))
		return 1;
	
	float2 fracUV = abs( frac(reprojUV / (halfPix * 0.5) + float2(0.002, 0.002)) );
	fracUV.x = fracUV.x > 0.5 ? 1.5 - fracUV.x : 0.5 - fracUV.x;
	fracUV.y = fracUV.y > 0.5 ? 1.5 - fracUV.y : 0.5 - fracUV.y;

	float l1 = lerp(compare.y, compare.x, fracUV.x);
	float l2 = lerp(compare.z, compare.w, fracUV.x);
	return 1 - lerp(l1, l2, fracUV.y); 
}

// -------------------------------------
// Search for potential blockers
// -------------------------------------
float findBlocker(float3 uv,
		float current_depth,
		float searchWidth,
		float numSamples)
{
		float stepSize = 2 * searchWidth / numSamples;
		uv.xy = uv.xy - float2(searchWidth, searchWidth);
		
		float blockerSum = 0;
		float blockerCount = 0;
		
		for (int i=0; i<numSamples; i++)
		{
			for (int j=0; j<numSamples; j++)
			{			
				float shadMapDepth = shadows.SampleLevel(samplerPointClamp, uv + float3(i*stepSize, j*stepSize, 0), 0).r;
				if (shadMapDepth < current_depth)
				{
					blockerSum += shadMapDepth;
					blockerCount++;
				}
			}
		}
		
		if(blockerCount == 0)
			return -999;
		return blockerSum / blockerCount;
}

// ------------------------------------------------
// Estimate penumbra based on
// blocker estimate, receiver depth, and light size
// ------------------------------------------------
float estimatePenumbra(float current_depth,
			float Blocker,
			float LightSize)
{
       // estimate penumbra using parallel planes approximation
       float penumbra = (current_depth - Blocker) * LightSize / Blocker;
       return penumbra;
}

// Blur filter
float Blurfilter(float3 uv,
		float current_depth,
		float searchWidth,
		float numSamples,
		float halfPix)
{
        // divide filter width by number of samples to use
        float stepSize = 2 * searchWidth / numSamples;
        // compute starting point uv coordinates for search
        uv.xy = uv.xy - float2(searchWidth, searchWidth);
		
        float blockerCount = 0;
        for (int i=0; i<numSamples; i++)
		{
			for (int j=0; j<numSamples; j++)
			{
				float3 cur_uv = uv;
				cur_uv.xy += float2(i*stepSize, j*stepSize);
				float4 shadMapDepth = shadows.GatherRed(samplerPointClamp, cur_uv);
				float4 compare = float4(shadMapDepth.x < current_depth,
										shadMapDepth.y < current_depth,
										shadMapDepth.z < current_depth,
										shadMapDepth.w < current_depth);
					
				if(any(compare))
				{
					float2 fracUV = abs(frac((cur_uv.xy - halfPix) / (halfPix * 2)));
					fracUV.x = fracUV.x > 0.5 ? 1 - fracUV.x : fracUV.x;
					fracUV.y = fracUV.y > 0.5 ? 1 - fracUV.y : fracUV.y;

					float l1 = lerp(compare.x, compare.y, fracUV.x);
					float l2 = lerp(compare.z, compare.w, fracUV.x);

					blockerCount += lerp(l1, l2, fracUV.y);
				}
            }
        }
	
		return blockerCount / (numSamples*numSamples);
}

// Random Blur filter
float RandomBlurfilter(float2 uv,
		float current_depth,
		Texture2D ShadowMap,
		float searchWidth,
		float numSamples,
		float2 noise_coord,
		bool noise)
{
        // divide filter width by number of samples to use
        float stepSize = 2 * searchWidth / numSamples;
        // compute starting point uv coordinates for search
        uv = uv - float2(searchWidth, searchWidth);
		
		float2 rnd_vect = 0;
		
        float blockerCount = 0;
        for (int i=0; i<numSamples; i++) {
               for (int j=0; j<numSamples; j++) {
						if(noise)
						{
							rnd_vect = noiseTex.SampleLevel(samplerBilinearWrap, noise_coord, 0).rg;
							rnd_vect = (rnd_vect * 2 - 1) * stepSize; 
						}
						
                       float shadMapDepth = ShadowMap.SampleLevel(samplerPointClamp, uv + float2(i*stepSize,j*stepSize) + rnd_vect, 0).r;
                       // found a blocker
                       if (shadMapDepth < current_depth) 
					   {
                            blockerCount++;
                       }
               }
        }
	
		return blockerCount / (numSamples*numSamples);
}

// Gather filter
float GatherFilter(float3 UV, float2 reprojUV, float halfPix, float depth)  
{  
	float4 shadMapDepth = shadows.GatherRed(samplerPointClamp, UV);

	float4 compare = float4(shadMapDepth.x < depth,
							shadMapDepth.y < depth,
							shadMapDepth.z < depth,
							shadMapDepth.w < depth);
				
	if(!any(compare))
		return 1;
	
	float2 fracUV = abs( frac(reprojUV / (halfPix * 0.5) + float2(0.002, 0.002)) );
	fracUV.x = fracUV.x > 0.5 ? 1.5 - fracUV.x : 0.5 - fracUV.x;
	fracUV.y = fracUV.y > 0.5 ? 1.5 - fracUV.y : 0.5 - fracUV.y;

	float l1 = lerp(compare.y, compare.x, fracUV.x);
	float l2 = lerp(compare.z, compare.w, fracUV.x);
	return 1 - lerp(l1, l2, fracUV.y); 
}

// VSM
float linstep(float min, float max, float v)
{
	return clamp((v-min) / (max - min), 0, 1);
}
 
float ReduceLightBleeding(float p_max, float Amount)  
{  
  // Remove the [0, Amount] tail and linearly rescale (Amount, 1].  
   //return linstep(Amount, 1, p_max);  
   return smoothstep(Amount, 1, p_max);  
}

float ChebyshevUpperBound(float3 Moments, float t, float bias, float bias_maxz, float lbr)  
{  
	if(Moments.z < t - bias_maxz)
	{
		return 0.0;
	}
	// One-tailed inequality valid if t > Moments.x  
	float p = (t <= Moments.x);  
	// Compute variance.  
	float Variance = Moments.y - (Moments.x*Moments.x);
	Variance = max(Variance, bias);
	
	float d = t - Moments.x;
	float p_max = Variance / (Variance + d*d);
	p_max = max(p, p_max);
	p_max = ReduceLightBleeding(p_max, lbr);
	return p_max;
}

// ----------------------------------------------------
// Percentage-closer filter implementation with
// variable filter width and number of samples.
// This assumes a square filter with the same number of
// horizontal and vertical samples.
// ----------------------------------------------------

float PCF_Filter(float2 uv, float current_depth, Texture2D ShadowMap, float filterWidth, float numSamples, float2 noise_coord, bool noise)
{
       // compute step size for iterating through the kernel
       float stepSize = 2 * filterWidth / numSamples;

       // compute uv coordinates for upper-left corner of the kernel
       uv = uv - float2(filterWidth,filterWidth);

       float sum = 0;  // sum of successful depth tests

	   float2 rnd_vect = 0;
	   
       // now iterate through the kernel and filter
       for (int i=0; i<numSamples; i++) {
               for (int j=0; j<numSamples; j++) {
						if(noise)
						{
							rnd_vect = noiseTex.SampleLevel(samplerBilinearWrap, noise_coord, 0).rg;
							rnd_vect = (rnd_vect * 2 - 1) * stepSize; 
						}
			   
                       // get depth at current texel of the shadow map
                       float shadMapDepth = 0;
                       
                       shadMapDepth = ShadowMap.SampleLevel(samplerPointClamp, uv + float2(i*stepSize,j*stepSize) + rnd_vect, 0).r;

                       // test if the depth in the shadow map is closer than
                       // the eye-view point
                       float shad = current_depth < shadMapDepth;

                       // accumulate result
                       sum += shad;
               }
       }
       
       // return average of the samples
       return sum / (numSamples*numSamples);

}
*/