#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/light_structs.hlsl"
#include "../common/common_helpers.hlsl"

Texture2D sys_depthGB : register(t0);
SamplerState samplerPointClamp;

Texture3D textureVolume : register(t1);
SamplerState samplerBilinearVolumeClamp;

Texture2D textureNoise : register(t2);
SamplerState samplerPointWrap;

cbuffer matrixBuffer : register(b1)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

cbuffer materialBuffer : register(b2)
{
	float4 cutPlaneOriginL;
	float4 cutPlaneNormal;
	float4 volumeScale;
	float4 volumeScaleInv;

	float cutPlaneFade;
	float _padding1;
	float _padding2;
	float _padding3;
};

// Schlick phase function
float SchlickPhase(float3 viewDir, float3 lightDir, float asymmetry)
{
	float cosCL = dot(viewDir, -lightDir);
	float denominator = 1.0f - asymmetry * cosCL;
	float phase = (1.0f - asymmetry * asymmetry) / (4.0f * PI * denominator * denominator);
	return phase;
}

float CalcFadeFactor(float3 samplePos)
{
	float3 samplePlanePos = (samplePos - cutPlaneOriginL.xyz) * volumeScale.xyz;
	float fadeFactor = 1.0f - saturate(dot(samplePlanePos, cutPlaneNormal.xyz) * cutPlaneFade);
	return fadeFactor;
}

#define DISCARD_LIGHT_LEVEL 0.001f
#define SHADOW_STEPS_COUNT 32

bool DoVolumeStep(inout float transmittanceAcc, inout float3 lightAcc, float stepDensity, float3 currentPos, float shadowSearchDist, 
	float3 lightDir, float3 absorptionShadow, float3 lightColor, float3 skyColor, float3 absorptionSky)
{
	float lightDensitySample = 0;
	float4 colorDensitySample = 0;

	bool res = true;

	float fadeFactor = CalcFadeFactor(currentPos);
	if (fadeFactor > 0)
	{
		colorDensitySample = textureVolume.SampleLevel(samplerBilinearVolumeClamp, currentPos, 0);
		float finalDensity = saturate(colorDensitySample.a * stepDensity);
		
		// determine shadow
		if (colorDensitySample.a > DISCARD_LIGHT_LEVEL)
		{
#ifdef VOLUME_SHADED
		
			float3 volumePos = currentPos;

			// shadow search
			float shadowAcc = 0;
			for (int s = 0; s < SHADOW_STEPS_COUNT; s++)
			{
				volumePos += lightDir;
				lightDensitySample = textureVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0).a;

				// early termination
				float3 boxTest = floor((abs(volumePos - 0.5f)) + 0.5f);
				if ((boxTest.x + boxTest.y + boxTest.z) >= 1.0f || shadowAcc > shadowSearchDist)
					break;

				shadowAcc += lightDensitySample;
			}

			// beer-lambert law & fake ambient shadowing
			float3 finalLight = lightColor * colorDensitySample.rgb * exp(-shadowAcc * absorptionShadow) * finalDensity * transmittanceAcc;
#else
			float3 finalLight = colorDensitySample.rgb * finalDensity * transmittanceAcc;
#endif

#ifdef VOLUME_SHADED
			//Sky Lighting
			shadowAcc = 0;

			volumePos = currentPos + float3(0, 0.05, 0);
			shadowAcc += textureVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0).r;
			volumePos = currentPos + float3(0, 0.1, 0);
			shadowAcc += textureVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0).r;
			volumePos = currentPos + float3(0, 0.2, 0);
			shadowAcc += textureVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0).r;

			finalLight += skyColor * exp(-shadowAcc * absorptionSky) * finalDensity * transmittanceAcc;
#endif
			
#ifdef VOLUME_SOLID
			if (finalDensity * fadeFactor > 0.5f)
			{
				finalDensity = 1.0f;
				fadeFactor = 1.0f;
			}
			else
			{
				finalDensity = 0.0f;
				fadeFactor = 0.0f;
			}
#endif

			lightAcc += finalLight * fadeFactor;
			transmittanceAcc *= 1.0f - finalDensity * fadeFactor;

			if (transmittanceAcc < DISCARD_LIGHT_LEVEL)
				res = false;
		}
	}

	return res;
}

float4 VolumeTrace(int stepsCount, float density, float3 absorptionShadow, float3 lightDir, float3 lightColor,
	float3 skyColor, float3 absorptionSky, float3 viewDir, float3 cameraPos, float2 noiseUV, float2 screenUV)
{
	float3 viewDirLocal = normalize(viewDir * volumeScaleInv.xyz);

	float stepSize = 1.0f / stepsCount;

	// box intersection
	float thickness = 0;
	float3 startPos = 0;
	{
		float sceneDepth = DepthToLinear(sys_depthGB.SampleLevel(samplerPointClamp, screenUV, 0).r);
		sceneDepth = length(viewDir * sceneDepth * volumeScaleInv.xyz) / abs(dot(g_CamDir, viewDir));

		// TODO: pass world inverse matrix
		float3 cameraPosLocal = cameraPos;// mul(float4(cameraPos, 1.0f), SCENE_DATA.WorldToLocal).xyz;
		cameraPosLocal = cameraPosLocal * volumeScaleInv.xyz + 0.5f;

		float3 invDirection = 1.0f / (-viewDirLocal);

		float3 firstPlaneIntersect = (0.0f - cameraPosLocal) * invDirection;
		float3 secondPlaneIntersect = (1.0f - cameraPosLocal) * invDirection;
		float3 closestPlaneIntersect = min(firstPlaneIntersect, secondPlaneIntersect);
		float3 furthestPlaneIntersect = max(firstPlaneIntersect, secondPlaneIntersect);

		float2 intersections;
		intersections.x = max(closestPlaneIntersect.x, max(closestPlaneIntersect.y, closestPlaneIntersect.z));
		intersections.y = min(furthestPlaneIntersect.x, min(furthestPlaneIntersect.y, furthestPlaneIntersect.z));

		float startOffset = 1.0f - frac((intersections.x - length(cameraPosLocal - 0.5f)) * stepsCount);
		intersections.x += startOffset * stepSize;

		intersections.x = max(0, intersections.x);
		intersections.y = min(intersections.y, sceneDepth);

		thickness = max(0, intersections.y - intersections.x);
		startPos = cameraPosLocal + (intersections.x * -viewDirLocal);
	}
	
	thickness *= stepsCount;
	int maxStepsCount = int(clamp(floor(thickness), 0, 512.0));
	float lastStepSize = frac(thickness);

	float shadowStepSize = 1.0f / SHADOW_STEPS_COUNT;
	lightDir *= shadowStepSize * 0.5;
	absorptionShadow *= shadowStepSize;

	// inverse beer-lambert law to calculate max shadow search distance
	float shadowSearchDist = -log(DISCARD_LIGHT_LEVEL) / luminance(absorptionShadow);
	
	viewDirLocal *= stepSize;

	// TODO: density acc should depend of scale
	float scaleDensityFactor = length(viewDir * volumeScale.xyz) / (0.3333f * (volumeScale.x + volumeScale.y + volumeScale.z));
	//density *= stepSize;// *scaleDensityFactor;
	float stepDensity = density * stepSize;
	
	// screen temporal noise
	float random = textureNoise.SampleLevel(samplerPointWrap, noiseUV, 0).a;
	
	// linear ray matching
	float3 currentPos = startPos + viewDirLocal * random;
	float transmittanceAcc = 1.0f;
	float3 lightAcc = 0;
	
	for (int i = 0; i < maxStepsCount; i++)
	{
		if (!DoVolumeStep(transmittanceAcc, lightAcc, stepDensity, currentPos, shadowSearchDist, 
			lightDir, absorptionShadow, lightColor, skyColor, absorptionSky))
			break;

		currentPos -= viewDirLocal;
	}

	// last fine step for smooth intersactions with geometry
	// code almost copy-pasted here from above
	if (transmittanceAcc > DISCARD_LIGHT_LEVEL)
	{
		// fractional step close to geometry
		currentPos += viewDirLocal  * (1.0f - lastStepSize);
		stepDensity *= lastStepSize;

		DoVolumeStep(transmittanceAcc, lightAcc, stepDensity, currentPos, shadowSearchDist, lightDir, absorptionShadow, lightColor, skyColor, absorptionSky);
	}
	
	return float4(lightAcc, 1.0f - transmittanceAcc);
}

float4 VolumeCubePS(PI_ToolMesh input) : SV_TARGET
{
	const float3 lightDir = normalize(float3(1, 2, 1));
	const float3 lightColor = float3(0.9, 0.8, 0.3) * 8.0;
	const float3 skyColor = float3(0.03, 0.03, 0.04) * 1.0;

	const float3 absorptionColor = float3(0.7, 0.2, 0.02);
	const float absorptionScale = 30.0f;
	const float absorptionSkyScale = 2.0f;
	const float asymmetry = 0.2f;

	float3 absorptionShadow = (1.0f - absorptionColor) * absorptionScale;
	float3 absorptionSky = (1.0f - absorptionColor) * absorptionSkyScale;

	float2 screenUV = input.position.xy * g_PixSize;
	float3 viewDir = normalize( - GetCameraVector(screenUV));

	float3 lightColorPhase = lightColor * SchlickPhase(viewDir, lightDir, asymmetry);

	float noiseW, noiseH;
	textureNoise.GetDimensions(noiseW, noiseH);
	float2 noiseUV = float2(input.position.xy) / float2(noiseW, noiseH);

	float4 volumeVis = VolumeTrace(64, 70.0f, absorptionShadow, lightDir, lightColorPhase, skyColor, absorptionSky, viewDir, g_CamPos, noiseUV, screenUV);
	
	return float4(volumeVis.rgb * volumeVis.a, volumeVis.a);
}

PI_ToolMesh VolumeCubeVS(VI_Mesh input)
{
    PI_ToolMesh output;

    output.position = mul(float4(input.position, 1), worldMatrix);
	output.worldPos = output.position;
    output.position = mul(output.position, g_viewProj);
	
    output.normal = mul(input.normal, (float3x3)normalMatrix);
    output.normal = normalize(output.normal);
	
	output.tex = input.tex;

    return output;
}