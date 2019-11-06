#define _hammersleySize 1024
#define nbSamples 16384

#define cubeSamples 32768
#define diffuseCubeSamples 16384

#define mippedCubeSamples 16384

#define diffuseCubeMip 4

float G_Smith( float Roughness, float NoV, float NoL )
{
	float a = Square( Roughness );
	float a2 = a*a;

	float G_SmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
	float G_SmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
	return rcp( G_SmithV * G_SmithL );
}

float hammersley1D(int i)
{
  float xInHammersleyTex = (float(i)+0.5) / _hammersleySize;
  xInHammersleyTex = frac(xInHammersleyTex);
  return hammersleyTexture.SampleLevel(samplerPointClamp, float2(xInHammersleyTex, 0.5), 0).x;
}

float2 hammersley2D(int i, int Samples)
{
  return float2(
		(float(i)+0.5) / float(Samples),
		hammersley1D(i)
		);
}

uint BRDFReverseBits( uint bits )
{
#if SM5_PROFILE
	return reversebits( bits );
#else
	bits = ( bits << 16) | ( bits >> 16);
	bits = ( (bits & 0x00ff00ff) << 8 ) | ( (bits & 0xff00ff00) >> 8 );
	bits = ( (bits & 0x0f0f0f0f) << 4 ) | ( (bits & 0xf0f0f0f0) >> 4 );
	bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
	bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
	return bits;
#endif
}

float2 Hammersley( uint Index, uint NumSamples )
{
	float E1 = frac( (float)Index / NumSamples );
	float E2 = float( BRDFReverseBits(Index) ) * 2.3283064365386963e-10;
	return float2( E1, E2 );
}

float3 ImportanceSampleGGX( float2 E, float Roughness )
{
	float m = Roughness * Roughness;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m*m - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	return H;
}

float3 ImportanceSampleGGXDir( float2 E, float Roughness )
{
	float m = Roughness * Roughness;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m*m - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	return H;
}

float3 ImportanceSampleGGX_Envmap( float2 E, float Roughness, float3 N )
{
	float3 H = ImportanceSampleGGXDir(E, Roughness);

	float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
	float3 TangentX = normalize( cross( UpVector, N ) );
	float3 TangentY = cross( N, TangentX );
	
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float3 ImportanceSampleGGXDir_Diffuse( float2 E )
{
	float Phi = 2 * PI * E.y;
	float CosTheta = sqrt( max(1 - E.x, 0) );
	float SinTheta = sqrt( E.x );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	return H;
}

float3 ImportanceSampleGGX_Diffuse( float2 E, float3 N )
{
	float3 H = ImportanceSampleGGXDir_Diffuse(E);

	float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
	float3 TangentX = normalize( cross( UpVector, N ) );
	float3 TangentY = cross( N, TangentX );
	
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float F_Schlick_Diffuse (float f90, float u )
{ 
	return 1 + ( f90 - 1 ) * pow (1.f - u , 5.f);
}

float Diffuse_Burley( float Roughness, float NoV, float NoL, float VoH )
{
	float energyBias = lerp (0 , 0.5 , Roughness );
	float energyFactor = lerp (1.0 , 1.0 / 1.51 , Roughness );
	float FD90 = energyBias + 2 * VoH * VoH * Roughness;
	float FdV = F_Schlick_Diffuse(FD90, NoV);
	float FdL = F_Schlick_Diffuse(FD90, NoL);
	return FdV * FdL * energyFactor;// / PI;
}

float3 importanceSampleCosDir(float2 E)
{
	float r = sqrt(E.x);
	float phi = E.y * PI * 2;

	float3 L = float3(r* cos( phi ), r* sin( phi ), sqrt(max(0.0f ,1.0f-E.x)));
	return normalize( L );
}

float3 IntegrateBRDF( float Roughness , float NoV )
{
	float3 V;
	V.x = sqrt( 1.0f - NoV * NoV ); // sin
	V.y = 0;
	V.z = NoV; // cos
	float A = 0;
	float B = 0;
	float C = 0;
	const uint NumSamples = nbSamples;
	for( uint i = 0; i < NumSamples; i++ )
	{
		//float2 Xi = hammersley2D( i, NumSamples );
		float2 Xi = Hammersley( i, NumSamples );
		float3 H = ImportanceSampleGGX( Xi, Roughness );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( L.z );
		float NoH = saturate( H.z );
		float VoH = saturate( dot( V, H ) );
		if( NoL > 0 )
		{
			float G = G_Smith( Roughness , NoV, NoL );
			//float G_Vis = G * VoH / (NoH * NoV);
			float G_Vis = 4 * G * VoH * (NoL / NoH);
			float Fc = pow( 1 - VoH, 5 );
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
		
		// diff
		L = importanceSampleCosDir(Xi);
		NoL = saturate( L.z );
		if(NoL>0)
		{
			float LoH = saturate( dot(L, normalize(V + L)) );
			C += Diffuse_Burley(Roughness, NoV , NoL , LoH);
		}
	}
	return float3( A, B, C ) / NumSamples;
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX( float Roughness, float NoH )
{
	float m = Roughness * Roughness;
	float d = m / max(0.00000001, ( m * m - 1.0 ) * NoH * NoH + 1);
	return d * d * INV_PI;
}

float distortion(float3 L)
{
  float sinT = sqrt(1.0-L.y*L.y);
  return sinT;
}

float4 PrefilterMippedEnvMap( float Roughness, float3 R, TextureCube t_cubemap, SamplerState t_sampler, float mips, int res )
{
	float3 N = R;
	float3 V = R;
	float4 PrefilteredColor = 0;
	float TotalWeight = 0;
	const uint NumSamples = mippedCubeSamples;
	for( uint i = 0; i < NumSamples; i++ )
	{
		float2 Xi = hammersley2D( i, NumSamples );
		float3 H = ImportanceSampleGGX_Envmap( Xi, Roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		if( NoL > 0 )
		{
			float NoH = saturate( dot(N, H) );
			float pdf = D_GGX( Roughness, NoH )*0.25f;
			float omegaS = 1.0 / ( NumSamples * pdf );
			//float omegaP = 4.0 * PI / (6.0 * res * res ) ;
			float omegaP = distortion(L) / ( res * res ) ;
			float mipLevel = clamp(0.5 * log2( omegaS / omegaP ) , 0, mips );

			PrefilteredColor += t_cubemap.SampleLevel( t_sampler, L, mipLevel ) * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

float4 PrefilterEnvMap( float Roughness, float3 R, TextureCube t_cubemap, SamplerState t_sampler )
{
	float3 N = R;
	float3 V = R;
	float4 PrefilteredColor = 0;
	float TotalWeight = 0;
	const uint NumSamples = cubeSamples;
	for( uint i = 0; i < NumSamples; i++ )
	{
		float2 Xi = hammersley2D( i, NumSamples );
		float3 H = ImportanceSampleGGX_Envmap( Xi, Roughness, N );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		if( NoL > 0 )
		{
			PrefilteredColor += t_cubemap.SampleLevel( t_sampler, L, 0 ) * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

float4 PrefilterDiffEnvMap( float3 R, TextureCube t_cubemap, SamplerState t_sampler )
{
	float3 N = R;
	float3 V = R;
	float4 PrefilteredColor = 0;
	float TotalWeight = 0;
	const uint NumSamples = diffuseCubeSamples;
	for( uint i = 0; i < NumSamples; i++ )
	{
		float2 Xi = hammersley2D( i, NumSamples );
		float3 H = ImportanceSampleGGX_Diffuse( Xi, N );
		float3 L = 2 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		if( NoL > 0 )
		{
			PrefilteredColor += t_cubemap.SampleLevel( t_sampler, L, diffuseCubeMip ) * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}