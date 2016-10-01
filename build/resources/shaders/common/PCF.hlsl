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