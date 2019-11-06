
struct SHcoef
{
	float Y[9];
};

struct SHcoef3
{
	float3 L[9];
};

SHcoef CalculateSHBasis(float3 dir)
{
	SHcoef res;
	res.Y[0] = 0.282095; // Y00 = sqrt(1.0 / (4 * PI))

	res.Y[1] = 0.488603 * dir.x; // Y11 = - sqrt(3.0 / (4 * PI)) * X
	res.Y[2] = 0.488603 * dir.z; // Y10 = sqrt(3.0 / (4 * PI)) * Z
	res.Y[3] = 0.488603 * dir.y; // Y1-1 = - sqrt(3.0 / (4 * PI)) * Y

	res.Y[4] = 1.092548 * dir.x * dir.z; // Y21 = - sqrt(15.0 / (8 * PI)) * X * Z
	res.Y[5] = 1.092548 * dir.y * dir.z; // Y2-1 = - sqrt(15.0 / (4 * PI)) * Y * Z
	res.Y[6] = 1.092548 * dir.y * dir.x; // Y2-2 = sqrt(15.0 / (4 * PI)) * X * Y
	res.Y[7] = 0.946176 * dir.z * dir.z - 0.315392; // Y20 = sqrt(5.0 / (16 * PI)) * (3 * Z * Z - 1)
	res.Y[8] = 0.546274 * (dir.x * dir.x - dir.y * dir.y); // Y22 = sqrt(15.0 / (16 * PI)) * (X * X - Y * Y)
	return res;
}

static const float shA0 = PI;
static const float shA1 = 2.094395; // 2/3 * PI
static const float shA2 = 0.785398; // PI/4

SHcoef CalculateSHCosineLobe(float3 dir)
{
	SHcoef res;
	res.Y[0] = shA0 * 0.282095; // Y00 = sqrt(1.0 / (4 * PI))

	res.Y[1] = shA1 * 0.488603 * dir.x; // Y11 = - sqrt(3.0 / (4 * PI)) * X
	res.Y[2] = shA1 * 0.488603 * dir.z; // Y10 = sqrt(3.0 / (4 * PI)) * Z
	res.Y[3] = shA1 * 0.488603 * dir.y; // Y1-1 = - sqrt(3.0 / (4 * PI)) * Y

	res.Y[4] = shA2 * 1.092548 * dir.x * dir.z; // Y21 = - sqrt(15.0 / (8 * PI)) * X * Z
	res.Y[5] = shA2 * 1.092548 * dir.y * dir.z; // Y2-1 = - sqrt(15.0 / (4 * PI)) * Y * Z
	res.Y[6] = shA2 * 1.092548 * dir.y * dir.x; // Y2-2 = sqrt(15.0 / (4 * PI)) * X * Y
	res.Y[7] = shA2 * 0.946176 * dir.z * dir.z - shA2 * 0.315392; // Y20 = sqrt(5.0 / (16 * PI)) * (3 * Z * Z - 1)
	res.Y[8] = shA2 * 0.546274 * (dir.x * dir.x - dir.y * dir.y); // Y22 = sqrt(15.0 / (16 * PI)) * (X * X - Y * Y)
	return res;
} 

SHcoef3 CalculateSHCoefs(float3 color, float3 dir)
{
	SHcoef basis = CalculateSHBasis(dir);
	
	SHcoef3 res;
	[unroll]
	for(int i = 0; i < 9; i++)
		res.L[i] = basis.Y[i] * color;
	 
	return res;
}

float3 ReconstrucColor(SHcoef3 sh, float3 dir)
{
	SHcoef basis = CalculateSHCosineLobe(dir);

	float3 color = 0;
	[unroll]
	for (int i = 0; i < 9; i++)
		color += sh.L[i] * basis.Y[i];

	//shA0 * sh.L[0] * basis.Y[0] +
		//shA1 * (sh.L[1] * basis.Y[1] + sh.L[2] * basis.Y[2] + sh.L[3] * basis.Y[3]) +
		//shA2 * (sh.L[4] * basis.Y[4] + sh.L[5] * basis.Y[5] + sh.L[6] * basis.Y[6] + sh.L[7] * basis.Y[7] + sh.L[8] * basis.Y[8]);
	
	return color;
}