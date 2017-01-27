
#define DFG_TEXTURE_SIZE 256
#define NOV_MIN 0.5f/DFG_TEXTURE_SIZE
#define NOV_MAX 1.0f - NOV_MIN

float calculateNoV(float3 N, float3 V)
{
	return saturate( dot(N, V) + 0.00001f );
}