
struct SHcoef
{
	float3 L[9];
};

SHcoef CalculateSHBasis(float3 dir)
{
	SHcoef res;
	res.L[0] = 0.282095; // Y00 = sqrt(1.0 / (4 * PI))
	res.L[1] = 0.488603 * dir.x; // Y11 = - sqrt(3.0 / (4 * PI)) * X
	res.L[2] = 0.488603 * dir.z; // Y10 = sqrt(3.0 / (4 * PI)) * Z
	res.L[3] = 0.488603 * dir.y; // Y1-1 = - sqrt(3.0 / (4 * PI)) * Y
	res.L[4] = 1.092548 * dir.x * dir.z; // Y21 = - sqrt(15.0 / (8 * PI)) * X * Z
	res.L[5] = 1.092548 * dir.y * dir.z; // Y2-1 = - sqrt(15.0 / (4 * PI)) * Y * Z
	res.L[6] = 1.092548 * dir.y * dir.x; // Y2-2 = sqrt(15.0 / (4 * PI)) * X * Y
	res.L[7] = 0.946176 * dir.z * dir.z - 0.315392; // Y20 = sqrt(5.0 / (16 * PI)) * (3 * Z * Z - 1)
	res.L[8] = 0.546274 * (dir.x * dir.x - dir.y * dir.y); // Y22 = sqrt(15.0 / (16 * PI)) * (X * X - Y * Y)
	return res;
}

SHcoef CalculateSHCoefs(float3 color, float3 dir)
{
	SHcoef res = CalculateSHBasis(dir);
	
	[unroll]
	for(int i = 0; i < 9; i++)
		res.L[i] *= color;
	 
	return res;
}

static const float shA0 = PI;
static const float shA1 = 2.094395; // 2/3 * PI
static const float shA2 = 0.785398; // PI/4

float3 ReconstrucColor(SHcoef sh, float3 dir)
{
	SHcoef basis = CalculateSHBasis(dir);

	float3 color = shA0 * sh.L[0] * basis.L[0] +
		shA1 * (sh.L[1] * basis.L[1] + sh.L[2] * basis.L[2] + sh.L[3] * basis.L[3]) +
		shA2 * (sh.L[4] * basis.L[4] + sh.L[5] * basis.L[5] + sh.L[6] * basis.L[6] + sh.L[7] * basis.L[7] + sh.L[8] * basis.L[8]);
	
	return color;
}