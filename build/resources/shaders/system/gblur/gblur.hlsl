#include "../../common/math.hlsl"
#include "../../common/structs.hlsl"
#include "../../common/shared.hlsl"

Texture2D inputTex : register(t0);
Texture2D depth : register(t1);
SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float pix;
	float _padding0;
	float _padding1;
	float _padding2;
};

#if G_SAMPLES == 1
	#define GWIEGTH {0.25, 0.5, 0.25}
#else 
	#if G_SAMPLES == 2
		#define GWIEGTH {0.015258, 0.21878, 0.531924, 0.21878, 0.015258}
	#else 
		#if G_SAMPLES == 3
			#define GWIEGTH {0.00291, 0.045807, 0.241313, 0.41994, 0.241313, 0.045807, 0.00291}
		#else
			#if G_SAMPLES == 4
				#define GWIEGTH {0.00129, 0.01463, 0.082921, 0.234932, 0.332454, 0.234932, 0.082921, 0.01463, 0.00129}
			#else
				#if G_SAMPLES == 7
					#define GWIEGTH {0.000446, 0.00223, 0.008779, 0.027009, 0.064769, 0.120979, 0.176049, 0.199478, 0.176049, 0.120979, 0.064769, 0.027009, 0.008779, 0.00223, 0.000446}
				#else
					#if G_SAMPLES == 10
						#define GWIEGTH {0.000514, 0.001477, 0.003799, 0.008741, 0.017997, 0.033159, 0.054670, 0.080657, 0.106483, 0.125794, 0.132981, 0.125794, 0.106483, 0.080657, 0.054670, 0.033159, 0.017997, 0.008741, 0.003799, 0.001477, 0.000514}
					#else
						#if G_SAMPLES == 15
							#define GWIEGTH {0.000343, 0.000701, 0.001366, 0.002532, 0.004469, 0.007505, 0.011998, 0.018256, 0.026440, 0.036447, 0.047821, 0.059721, 0.070988, 0.080316, 0.086492, 0.088654, 0.086492, 0.080316, 0.070988, 0.059721, 0.047821, 0.036447, 0.026440, 0.018256, 0.011998, 0.007505, 0.004469, 0.002532, 0.001366, 0.000701, 0.000343}
						#endif
					#endif
				#endif
			#endif
		#endif
	#endif
#endif

#define DEPTH_DIFF_FOR_GBLUR 0.01

#define G_COUNT G_SAMPLES * 2 + 1

#ifndef DEPTH_DEPEND 

float4 BlurH(PI_PosTex input) : SV_TARGET
{
	static const float wiegth[G_COUNT] = GWIEGTH;
	
	float4 color = 0;

	[unroll]
	for(int i = -G_SAMPLES; i <= G_SAMPLES; i++)
#if G_SAMPLES > 7
		color += inputTex.SampleLevel(samplerPointClamp, input.tex + float2(pix * i, 0), 0) * wiegth[i+G_SAMPLES];
#else
		color += inputTex.SampleLevel(samplerPointClamp, input.tex, 0, int2(i, 0)) * wiegth[i+G_SAMPLES];
#endif
	
	return color;
}

float4 BlurV(PI_PosTex input) : SV_TARGET
{
	static const float wiegth[G_COUNT] = GWIEGTH;
	
	float4 color = 0;

	[unroll]
	for(int i = -G_SAMPLES; i <= G_SAMPLES; i++)
#if G_SAMPLES > 7
		color += inputTex.SampleLevel(samplerPointClamp, input.tex + float2(0, pix * i), 0) * wiegth[i+G_SAMPLES];
#else
		color += inputTex.SampleLevel(samplerPointClamp, input.tex, 0, int2(0, i)) * wiegth[i+G_SAMPLES];
#endif
	
	return color;
}

#else

float4 BlurH(PI_PosTex input) : SV_TARGET
{
	static const float wiegth[G_COUNT] = GWIEGTH;
	
	float4 color = 0;
	float wiegth_accum = 0;

	float d[G_COUNT];
	[unroll]
	for(int j = -G_SAMPLES; j <= G_SAMPLES; j++)
#if G_SAMPLES > 7
		d[j+G_SAMPLES] = depth.SampleLevel( samplerPointClamp, UVforSamplePow2(input.tex + float2(pix * j, 0)), 0 ).r;
#else
		d[j+G_SAMPLES] = depth.SampleLevel( samplerPointClamp, UVforSamplePow2(input.tex), 0, int2(j, 0) ).r;
#endif

	const float target_depth = d[G_SAMPLES];
	const float comp_depth = max(1 - target_depth, 0.0000001) * DEPTH_DIFF_FOR_GBLUR;

	[unroll]
	for(int i = -G_SAMPLES; i <= G_SAMPLES; i++)
	{
		const int id = i+G_SAMPLES;

		const float cur_depth = d[id];
		if( abs(cur_depth - target_depth) < comp_depth )
		{
			const float w = wiegth[id];
#if G_SAMPLES > 7
			color += inputTex.SampleLevel( samplerPointClamp, input.tex + float2(pix * i, 0), 0) * w;
#else
			color += inputTex.SampleLevel( samplerPointClamp, input.tex, 0, int2(i, 0) ) * w;
#endif
			wiegth_accum += w;
		}
	}
	
	return color / wiegth_accum;
}

float4 BlurV(PI_PosTex input) : SV_TARGET
{
	static const float wiegth[G_COUNT] = GWIEGTH;
	
	float4 color = 0;
	float wiegth_accum = 0;

	float d[G_COUNT];
	[unroll]
	for(int j = -G_SAMPLES; j <= G_SAMPLES; j++)
#if G_SAMPLES > 7
		d[j+G_SAMPLES] = depth.SampleLevel( samplerPointClamp, UVforSamplePow2(input.tex + float2(0, pix * j)), 0 ).r;
#else
		d[j+G_SAMPLES] = depth.SampleLevel( samplerPointClamp, UVforSamplePow2(input.tex), 0, int2(0, j) ).r;
#endif

	const float target_depth = d[G_SAMPLES];
	const float comp_depth = max(1 - target_depth, 0.0000001) * DEPTH_DIFF_FOR_GBLUR;

	[unroll]
	for(int i = -G_SAMPLES; i <= G_SAMPLES; i++)
	{
		const int id = i+G_SAMPLES;

		const float cur_depth = d[id];
		if( abs(cur_depth - target_depth) < comp_depth )
		{
			const float w = wiegth[id];
#if G_SAMPLES > 7
			color += inputTex.SampleLevel(samplerPointClamp, input.tex + float2(0, pix * i), 0) * w;
#else
			color += inputTex.SampleLevel(samplerPointClamp, input.tex, 0, int2(0, i)) * w;
#endif
			wiegth_accum += w;
		}
	}
	
	return color / wiegth_accum;
}

#endif