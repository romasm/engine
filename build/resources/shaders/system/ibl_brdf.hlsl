#define _hammersleySize 1024
#define nbSamples 32
#define maxLod 9
#define R_IBL_MIP 6

float normal_distrib(
  float ndh,
  float Roughness)
{
  // use GGX / Trowbridge-Reitz, same as Disney and Unreal 4
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  float alpha = Roughness * Roughness;
  float tmp = alpha / max(1e-8,(ndh*ndh*(alpha*alpha-1.0)+1.0));
  return tmp * tmp * INV_PI;
}

float3 fresnel(
  float vdh,
  float3 F0)
{
  // Schlick with Spherical Gaussian approximation
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  float sphg = pow(2.0, (-5.55473*vdh - 6.98316) * vdh);
  return F0 + (float3(1.0, 1.0, 1.0) - F0) * sphg;
}

float G1(
float ndw, // w is either Ln or Vn
float k)
{
  // One generic factor of the geometry function divided by ndw
  // NB : We should have k > 0
  return 1.0 / ( ndw*(1.0-k) +  k );
}

float visibility(
  float ndl,
  float ndv,
  float Roughness)
{
  // Schlick with Smith-like choice of k
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  // visibility is a Cook-Torrance geometry function divided by (n.l)*(n.v)
  float k = max(Roughness * Roughness * 0.5, 1e-5);
  return G1(ndl,k)*G1(ndv,k);
}

float3 cook_torrance_contrib(
  float vdh,
  float ndh,
  float ndl,
  float ndv,
  float3 Ks,
  float Roughness)
{
  // This is the contribution when using importance sampling with the GGX based
  // sample distribution. This means ct_contrib = ct_brdf / ggx_probability
  return fresnel(vdh,Ks) * (visibility(ndl,ndv,Roughness) * vdh * ndl / ndh );
}

float3 ImportanceSampleGGX( float2 E, float Roughness, float3 N )
{
	float m = Roughness * Roughness;

	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( (1 - E.y) / ( 1 + (m*m - 1) * E.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
	float3 TangentX = normalize( cross( UpVector, N ) );
	float3 TangentY = cross( N, TangentX );
	// tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float probabilityGGX(float ndh, float vdh, float Roughness)
{
  return normal_distrib(ndh, Roughness) * ndh / (4.0*vdh);
}

float distortion(float3 Wn)
{
  // Computes the inverse of the solid angle of the (differential) pixel in
  // the cube map pointed at by Wn
  float sinT = sqrt(1.0-Wn.y*Wn.y);
  return sinT;
}

float hammersley1D(int i)
{
  float xInHammersleyTex = (float(i)+0.5) / _hammersleySize;
  return hammersleyTexture.SampleLevel(SampleTypeWarp, float2(xInHammersleyTex, 0.5), 0).x;
}

float2 hammersley2D(int i, int Samples)
{
  return float2(
		(float(i)+0.5) / float(Samples),
		hammersley1D(i)
		);
}

float computeLOD(float3 Ln, float p)
{
  return max(0.0, (maxLod-1.5) - 0.5*(log(float(nbSamples)) + log( p * distortion(Ln) )) * INV_LOG2);
}

float3 CalcIBL(float3 V, float3 N, float3 S, float R, float3 A, float2 screen_pos, float rtime)
{
	float ndv = dot(V, N);
	if (ndv < 0) 
	{
		V = reflect(V, N);
		ndv = abs(ndv);
	}
	
	float3 contribE = 0;
	
	float3 offsets[7] = 
	{
		float3(0,0,0),
		float3(1,0,0),
		float3(-1,0,0),
		float3(0,1,0),
		float3(0,-1,0),
		float3(0,0,1),
		float3(0,0,-1),
	};
	
	for(int j=0; j<7; j++)
	{
		float2 diff_coords = CubeVectorTo2DCoords(N+offsets[j]*0.1);
		contribE += shaderCubemap.SampleLevel(SampleTypeFilterWarp, diff_coords, R_IBL_MIP).rgb;
	}
	contribE = contribE/7 * A;
	
	float3 contribS = 0;
	for(int i=0; i<nbSamples; ++i)
	{
		float2 Xi = hammersley2D(i, nbSamples);
		float3 Hn = ImportanceSampleGGX(Xi,R,N);
		float3 Ln = -reflect(V,Hn);
		float ndl = dot(N, Ln);

		// Horizon fading trick from http://marmosetco.tumblr.com/post/81245981087
		const float horizonFade = 1.3;
		float horiz = clamp( 1.0 + horizonFade * ndl, 0.0, 1.0 );
		horiz *= horiz;

		ndl = max( 1e-8, abs(ndl) );
		float vdh = max(1e-8, dot(V, Hn));
		float ndh = max(1e-8, dot(N, Hn));
		float lodS = R < 0.01 ? 0.0 : computeLOD(Ln, probabilityGGX(ndh, vdh, R));
		contribS +=	shaderCubemap.SampleLevel(SampleTypeFilterWarp, CubeVectorTo2DCoords(Ln), lodS).rgb * cook_torrance_contrib(vdh, ndh, ndl, ndv, S, R) * horiz;
	}
	contribS /= float(nbSamples);
	
	return contribS + contribE;
}