
#define PI 3.1415926535897932384626433832795f
#define PIDIV2 PI/2.0
#define PI_MUL2 2*PI
#define INV_PI 0.31830988618379067153776752674503f
#define INV_LOG2 1.4426950408889634073599246810019f

#define SQRT2 1.4142135623730950488016887242097f
#define SQRT2DIV2 SQRT2/2.0f

#define DIV1SQRT3 0.57735026918962576450914878050196f

float Square( float x )
{
	return x*x;
}
float2 Square( float2 x )
{
	return x*x;
}
float3 Square( float3 x )
{
	return x*x;
}
float4 Square( float4 x )
{
	return x*x;
}

float PowAbsNoNull(float X,float Y)
{
	return pow(max(abs(X),0.000001f),Y);
}
float2 PowAbsNoNull(float2 X,float2 Y)
{
	return pow(max(abs(X),float2(0.000001f,0.000001f)),Y);
}
float3 PowAbsNoNull(float3 X,float3 Y)
{
	return pow(max(abs(X),float3(0.000001f,0.000001f,0.000001f)),Y);
}  
float4 PowAbsNoNull(float4 X,float4 Y)
{
	return pow(max(abs(X),float4(0.000001f,0.000001f,0.000001f,0.000001f)),Y);
} 

float PowAbs(float X,float Y)
{
	return pow(abs(X),Y);
}
float2 PowAbs(float2 X,float2 Y)
{
	return pow(abs(X),Y);
}
float3 PowAbs(float3 X,float3 Y)
{
	return pow(abs(X),Y);
}  
float4 PowAbs(float4 X,float4 Y)
{
	return pow(abs(X),Y);
} 

uint2 ScrambleTEA(uint2 v, uint IterationCount = 3)
{
	// Start with some random data (numbers can be arbitrary but those have been used by others and seem to work well)
	uint k[4] ={ 0xA341316Cu , 0xC8013EA4u , 0xAD90777Du , 0x7E95761Eu };
	
	uint y = v[0];
	uint z = v[1];
	uint sum = 0;
	
	[unroll]
	for(uint i = 0; i < IterationCount; ++i)
	{
		sum += 0x9e3779b9;
		y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
		z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
	}

	return uint2(y, z);
}

float luminance(float3 i)
{
   //return dot(i, float3(0.2125, 0.7154, 0.0721));
   return dot(i, float3(0.3, 0.59, 0.11));
}

static const float2 ClipToScreenConsts[2] =
{
	float2(0.5f,-0.5f),
	float2(0.5f,0.5f)
};

float2 ClipToScreen(float4 clipCoords)
{
	float2 screenCoords = ClipToScreenConsts[0] * clipCoords.xy / clipCoords.w + ClipToScreenConsts[1];
	return screenCoords;
}

float2 CubeVectorTo2DCoords(float3 vect)
{
	vect = normalize(vect);
	float2 res = 0;
	res.y = acos(vect.z)/PI;
	
	float2 proj = vect.xy;
	proj = normalize(proj);
	res.x = 0.5;
	if(proj.y >= 0)
		res.x += (acos(proj.x)/PI)/2;
	else
		res.x -= (acos(proj.x)/PI)/2;
		
	return res;
}

float3 CoordsToCubeVector(float2 coords)
{
	float3 vect = 0;
	vect.z = cos(coords.y * PI);
	vect.x = cos(abs((coords.x*2)-1) * PI);
	vect.y = sqrt(1 - vect.x*vect.x);//sin(acos(vect.x));
	
	if(coords.x > 0.5)
		vect.y = -vect.y ;
	
	float2 vectXY = float2(vect.x, vect.y);
	vectXY = vectXY * sqrt(1 - vect.z*vect.z);
	vect.x = vectXY.x;
	vect.y = vectXY.y;
	
	return normalize(vect);
}

float3 LinToGamma(float3 color)
{
	return PowAbs(color, 1.0f/2.2f);
}

float3 GammaToLin(float3 color)
{
	return PowAbs(color, 2.2f);
}

float LinToGamma(float color)
{
	return PowAbs(color, 1.0f/2.2f);
}

float GammaToLin(float color)
{
	return PowAbs(color, 2.2f);
}

float3 SRGBToLinear(float3 color )
{
	float3 linearRGBLo = color / 12.92f;
	float3 linearRGBHi = pow (( abs(color) + 0.055f) / 1.055f , 2.4f) ;
	float3 linearRGB = ( color <= 0.04045f) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

float3 LinearToSRGB(float3 color )
{
	float3 sRGBLo = color * 12.92f;
	float3 sRGBHi = ( pow( abs ( color ) , 1.0f/2.4f) * 1.055f) - 0.055f;
	float3 sRGB = ( color <= 0.0031308f) ? sRGBLo : sRGBHi;
	return sRGB;
}

float3 Uint3ToFloat3(RWTexture2D<unsigned int> tex[3], float2 coords)
{
	return float3(
		asfloat(tex[0][coords]), 
		asfloat(tex[1][coords]),
		asfloat(tex[2][coords])
		);
}

float3 Uint3ToFloat3(uint r, uint g, uint b)
{
	return float3(
		asfloat(r), 
		asfloat(g),
		asfloat(b)
		);
}

void Float3ToUint3(float3 color, RWTexture2D<unsigned int> tex[3], float2 coords)
{
	tex[0][coords] = asuint(color.r);
	tex[1][coords] = asuint(color.g);
	tex[2][coords] = asuint(color.b);
}

float MinFromBoxToPoint(float3 box_mins, float3 box_maxs, float3 oPoint)
{
	const float3 distancesMin = oPoint < box_mins ? abs(oPoint - box_mins) : 0;
	const float3 distancesMax = oPoint > box_maxs ? abs(oPoint - box_maxs) : 0;

	const float distance = dot(distancesMin, 1);
	return distance + dot(distancesMax, 1);
}

//Returns the closest intersection along the ray in x, and furthest in y. If the ray did not intersect the box, then the furthest intersection <= the closest intersection.
float2 RayBoxIntersect(float3 origin, float3 dir, float3 cornerMin, float3 cornerMax)
{
	float3 invdir = 1.0f / dir;
	
	float3 FirstPlaneIntersect = (cornerMin - origin) * invdir;
	float3 SecondPlaneIntersect = (cornerMax - origin) * invdir;
	
	float3 ClosestPlaneIntersect = min(FirstPlaneIntersect, SecondPlaneIntersect);
	float3 FurthestPlaneIntersect = max(FirstPlaneIntersect, SecondPlaneIntersect);

	float2 intersections;
	intersections.x = max(ClosestPlaneIntersect.x, max(ClosestPlaneIntersect.y, ClosestPlaneIntersect.z));
	intersections.y = min(FurthestPlaneIntersect.x, min(FurthestPlaneIntersect.y, FurthestPlaneIntersect.z));
	
	return saturate(intersections);
}

float2 RaySphereIntersect(float3 origin, float3 dir, float radius)
{
	float SphereRadiusSquared = radius * radius;
	float ReceiverToSphereCenterSq = dot(origin, origin);

	float3 QuadraticCoef;
	QuadraticCoef.x = dot(dir, dir);
	QuadraticCoef.y = 2 * dot(dir, origin);
	QuadraticCoef.z = ReceiverToSphereCenterSq - SphereRadiusSquared;

	float Determinant = QuadraticCoef.y * QuadraticCoef.y - 4 * QuadraticCoef.x * QuadraticCoef.z;

	if (Determinant >= 0)
	{ 
		float Epsilon = .000001f;
		float InvTwoA = .5 / (QuadraticCoef.x + Epsilon);
		float FarIntersection = (sqrt(Determinant) - QuadraticCoef.y) * InvTwoA;
		return float2(0, saturate(FarIntersection));
	}
	return float2(0, 0);
}

float4 EncodeTBNasFloat4(float3 N, float3 T)
{
	const float3 up = float3(0,0,1);
	const float3 forward = float3(1,0,0);
	
	float3 localUp;
	if(abs(N.z) > 0.9)
		localUp = normalize(forward - dot(forward, N) * N);
	else
		localUp= normalize(up - dot(up, N) * N);
	float3 localLeft = cross(N, localUp);
	
	float cosOffset = 0;
	float LdotT = dot(localLeft, T);
	
	if(LdotT == 0.0f)
		return float4(N, 0);
	
	float3 normal = N;
	if(LdotT < 0)
		normal *= 2.0;
		
	float angleTangent = dot(localUp, T) / abs(LdotT);
	
	return float4(normal, angleTangent);
}

void DecodeTBNfromFloat4(out float3 T, out float3 B, out float3 N, in float4 tbn)
{
	const float3 up = float3(0,0,1);
	const float3 forward = float3(1,0,0);
	
	float3 normal = tbn.xyz;
	
	int leftAlong = 1;
	if(dot(normal, normal) > 1.50)
	{
		leftAlong = -1;
		normal = normalize(normal);
	}
	
	float3 localUp;
	if(abs(normal.z) > 0.9)
		localUp = normalize(forward - dot(forward, normal) * normal);
	else
		localUp = normalize(up - dot(up, normal) * normal);
	float3 localLeft = cross(normal, localUp);

	T = normalize(localUp * tbn.w + localLeft * leftAlong);
	N = normal;
	B = cross(N, T);
}

uint GetMatID(uint gb_ID)
{
	return gb_ID >> 16;
}

uint GetObjID(uint gb_ID)
{
	return gb_ID & 0x0000ffff;
}

uint StoreMatObjID(uint matID, uint objID)
{
	return (objID & 0x0000ffff) + (matID << 16);
}