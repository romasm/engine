TECHNIQUE_DEFAULT
{
	Queue = SC_OPAQUE;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	StencilEnable = false;
	BlendEnable = false;

	FillMode = SOLID;
	CullMode = BACK;

	VertexShader = "hdrSkyVS";
	PixelShader = "hdrSkyPS";
}

#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

Texture2D skyTexture : register(t0);
SamplerState samplerAnisotropicClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float4 skyColor;

	float skyTexOpacity;
	float pad0;
	float pad1;
	float pad2;
};

cbuffer materialId : register(b2)
{
	uint iddata[4];
};

cbuffer matrixBuffer : register(b3)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

// pixel
struct PIsky
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float4 worldPos : POSITION;
};

PO_Gbuffer hdrSkyPS(PIsky input)
{
	PO_Gbuffer res;

	float3 sky = lerp(skyColor.rgb, skyTexture.Sample(samplerAnisotropicClamp, input.tex).rgb, skyTexOpacity);

	res.albedo_roughY = 0;
	res.tbn = 0;
	res.vnormXY = 0;
	res.spec_roughX = 0;
	res.emiss_vnormZ = float4(sky.rgb, 0);
	res.id = iddata[0];
	res.subs_thick = 0;
	res.ao = 0;
	return res;
}

// vertex
PIsky hdrSkyVS(VI_Mesh input)
{
    PIsky output;
		
    output.position = mul(float4(input.position, 1), worldMatrix);
	output.position.xyz += g_CamPos; 
	output.worldPos = output.position;
    output.position = mul(output.position, g_viewProj);
	
	output.tex = input.tex;

    return output;
}
