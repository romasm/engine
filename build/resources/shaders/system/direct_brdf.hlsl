// OPTIMIZE ALL

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

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
// [Lagarde 2012, "Spherical Gaussian approximation for Blinn-Phong, Phong and Fresnel"]
float3 F_Schlick( float3 SpecularColor, float VoH )
{
	float invVoH = 1 - VoH;
	float Fc = invVoH * invVoH;
	Fc *= Fc * invVoH;

	// Anything less than 2% is physically impossible and is instead considered to be shadowing 
	return saturate( 50.0 * SpecularColor.g ) * Fc + (1 - Fc) * SpecularColor;
	
	//Frostbyte
	//return SpecularColor + (1 - SpecularColor) * pow (1. f - VoH, 5.f);
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

float3 directSubScattering(float3 color, MaterialParamsStructBuffer params, float3 L, float3 N, float3 V)
{
	float3 vLight = L + N * params.ss_distortion;
	float VoL = pow(saturate(dot(V, -vLight)), params.ss_direct_pow) * params.ss_direct_translucency;
	float3 SSS = VoL + params.ss_indirect_translucency;
	return SSS * color;
}