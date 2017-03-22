// OPTIMIZE ALL

// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf

float3 getSpecularDominantDirArea( float3 N, float3 R, float roughness )
{
	float lerpFactor = 1.0 - roughness;
	return normalize( lerp(N, R, lerpFactor));
}

float F_Schlick_Diffuse(float f90, float u )
{ 
	return 1 + ( f90 - 1 ) * pow(1.0f - u , 5.0f);
}

// [Burley 2012, "Physically-Based Shading at Disney"], Renormalized (Frostbyte)
float3 Diffuse_Burley( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
	float energyBias = lerp (0 , 0.5 , Roughness );
	float energyFactor = lerp (1.0 , 1.0 / 1.51 , Roughness );
	float FD90 = energyBias + 2 * VoH * VoH * Roughness;
	float FdV = F_Schlick_Diffuse(FD90, NoV);
	float FdL = F_Schlick_Diffuse(FD90, NoL);
	return DiffuseColor * FdV * FdL * energyFactor * INV_PI;
}

float3 Diffuse_Lambert( float3 DiffuseColor )
{
	return DiffuseColor * INV_PI;
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX( float Roughness, float NoH )
{
	float a = Roughness * Roughness;	
	float d = a / max(0.00000001, ( NoH * a * a - NoH ) * NoH + 1);
	return INV_PI * d * d;		
}

// Anisotropic GGX
// [Burley 2012, "Physically-Based Shading at Disney"]
float D_GGXaniso( float RoughnessX, float RoughnessY, float NoH, float3 H, float3 X, float3 Y )
{
	float mx = RoughnessX * RoughnessX;
	float my = RoughnessY * RoughnessY;
	float XoH = dot( X, H );
	float YoH = dot( Y, H );
	float d = XoH*XoH / (mx*mx) + YoH*YoH / (my*my) + NoH*NoH;
	return INV_PI / max(0.000000000000001, mx*my * d*d );
}

// Оптимизированный Smith GGX (Frostbyte)
float G_SmithGGX( float NoL, float NoV, float Roughness)
{
	float a = Roughness * Roughness;
	float a2 = a * a;
	
	// Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
	float Lambda_GGXV = NoL * sqrt (( -NoV * a2 + NoV ) * NoV + a2 );
	float Lambda_GGXL = NoV * sqrt (( -NoL * a2 + NoL ) * NoL + a2 );
	
	return 0.5f / ( Lambda_GGXV + Lambda_GGXL );
}

// Original G GGX
float G_GGX(float R, float NoX)
{
	float a = R * R;
	float NoXsqr = NoX * NoX;
	float G = 2.0 / (1 + sqrt(1 + a * a * (1 - NoXsqr) / NoXsqr));
	return G;
}

float GG_GGX(float NoL, float NoV, float R)
{
	return G_GGX(R, NoV) * G_GGX(R, NoL);
}

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
// [Lagarde 2012, "Spherical Gaussian approximation for Blinn-Phong, Phong and Fresnel"]
float3 F_Schlick( float3 SpecularColor, float VoH )
{
	float invVoH = 1 - VoH;
	float Fc = invVoH * invVoH;
	Fc *= Fc * invVoH;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing 
	return saturate( 50.0 * SpecularColor.g ) * Fc + (1 - Fc) * SpecularColor;
}

float F_Schlick_Refract( float R0, float LoN )
{
	float invLoN = 1 - LoN;
	float Fc = invLoN * invLoN;
	Fc *= Fc * invLoN;

	//return R0 + (1 - R0) * Fc;
	return saturate( 50.0 * R0 ) * Fc + (1 - Fc) * R0;
}

float IORtoR0( float IOR )
{
	float R0 = (IOR - 1) / (IOR + 1);
	R0 *= R0;
	return R0;
}

float3 directSpecularBRDF(float3 S, float2 R, float NoH, float NoV, float NoL, float VoH, float3 H, float3 X, float3 Y, float avgR)
{
	float dGGX = 0;
	if(R.x != R.y)
		dGGX = D_GGXaniso( R.x, R.y, NoH, H, X, Y );
	else
		dGGX = D_GGX(avgR, NoH);	
	return dGGX * G_SmithGGX(NoL, NoV, avgR) * F_Schlick(S, VoH);
}

float3 directDiffuseBRDF(float3 A, float R, float NoV, float NoL, float VoH)
{
	return Diffuse_Burley( A, R, NoV, NoL, VoH );
}

float3 directSubScattering(in GBufferData gbuffer, in DataForLightCompute mData, MaterialParams params, float3 L, float3 V)
{
	/*float3 vLight = L + N * params.ss_distortion;
	float VoL = pow(saturate(dot(V, -vLight)), params.ss_direct_pow) * params.ss_direct_translucency;
	float3 SSS = VoL + params.ss_indirect_translucency;
	return SSS * color;*/

	const float attenuationCoef = 1.0;
	const float asymmetry = 0.8;
	const float ior = 1.0 / 1.5;
	
	float refractCos = sqrt( 1 - ior * ior * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractionDir = ior * (-V) + ( ior * mData.NoV - refractCos ) * gbuffer.normal;

	// Schlick phase function
	float cosLV = dot(refractionDir, L);
	float phase = (1 - asymmetry * asymmetry) / (4 * PI * (1 - asymmetry * cosLV ));
	
	// Lambert-Beer law
	float3 attenFactor = -gbuffer.thickness;
	float3 scattering = exp(attenFactor); // travelDist in shadow

	return scattering * phase * gbuffer.subsurf;

	/*float3 vLight = L + N * 0.1;
	float VoL = pow(saturate(dot(V, -vLight)), 1.0) * 0.1;
	float3 SSS = VoL + 0.1;
	return SSS * color;*/
}