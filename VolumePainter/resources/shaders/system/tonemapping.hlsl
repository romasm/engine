float3 Uncharted2Tonemap(float3 x)
{
	return ( (x * (tm_A * x + tm_C * tm_B) + tm_D * tm_E) / (x * (tm_A * x + tm_B) + tm_D * tm_F) ) - tm_E / tm_F;
}

float3 FilmicTonemapping(float3 color, float avgLum)
{	
	float3 texColor = color * (middleGray / avgLum);
	texColor *= exposure;

	return Uncharted2Tonemap(texColor) / Uncharted2Tonemap(tm_W);
}