/*
 * NoiseBaker.hlsl
 * * Purpose: Generates a Perlin-Worley noise atlas for cloud modeling.
 * This atlas stores 3D noise data in a 2D layout to be used as a lookup table (LUT).
 */

RWTexture2D<float4> OutputAtlas : register(u0);

#define SIZE 8.0f // Base tile frequency for the noise pattern

// Ensures the noise tiles perfectly by using modular arithmetic
float3 modulo(float3 m, float n)
{
    return m % n;
}

// Quintic interpolation curve for smooth transitions between noise gradients
float3 fade(float3 t)
{
    return (t * t * t) * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Deterministic 3D Hash function to generate pseudo-random vectors at grid points
float3 hash3D(float3 p3)
{
    p3 = modulo(p3, SIZE);
    p3 = frac(p3 * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yxz + 33.33f);
    return 2.0f * frac((p3.xxy + p3.yxx) * p3.zyx) - 1.0f;
}

// Perlin (Gradient) Noise: Defines the low-frequency base shape of clouds
float gradientNoise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = fade(f);

    // Trilinear interpolation across the 8 corners of the 3D grid cell
    return lerp(lerp(lerp(dot(hash3D(i + float3(0, 0, 0)), f - float3(0, 0, 0)),
                          dot(hash3D(i + float3(1, 0, 0)), f - float3(1, 0, 0)), u.x),
                     lerp(dot(hash3D(i + float3(0, 1, 0)), f - float3(0, 1, 0)),
                          dot(hash3D(i + float3(1, 1, 0)), f - float3(1, 1, 0)), u.x), u.y),
                lerp(lerp(dot(hash3D(i + float3(0, 0, 1)), f - float3(0, 0, 1)),
                          dot(hash3D(i + float3(1, 0, 1)), f - float3(1, 0, 1)), u.x),
                     lerp(dot(hash3D(i + float3(0, 1, 1)), f - float3(0, 1, 1)),
                          dot(hash3D(i + float3(1, 1, 1)), f - float3(1, 1, 1)), u.x), u.y), u.z);
}

// Worley Noise: Cellular noise used for cloud erosion and wispy details
float worley(float3 pos, float numCells)
{
    float3 p = pos * numCells;
    float d = 1e10;
    
    // Search neighboring 3D cells (3x3x3 grid) for the closest feature point
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            for (int z = -1; z <= 1; z++)
            {
                float3 tp = floor(p) + float3(x, y, z);
                // Modular hashing for tileable Voronoi patterns
                tp = p - tp - (0.5f + 0.5f * hash3D(tp % numCells));
                d = min(d, dot(tp, tp));
            }
    return 1.0f - saturate(d); // Inverted result to create "puffy" structures
}

// Maps 2D Atlas UV coordinates to 3D Volume coordinates for baking
float3 get3Dfrom2D(float2 uv, float tileRows)
{
    float2 tile = floor(uv);
    float z = floor(tileRows * tile.y + tile.x);
    return float3(frac(uv), z);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Texture Layout: 204x204 grid containing a 6x6 tile atlas
    // Each tile is 32x32 pixels with 1-pixel padding on each side
    float2 uv = DTid.xy / 34.0f;
    float3 p = get3Dfrom2D(uv, 6.0f);
    p.z /= 36.0f; // Normalize Z across the 36 total tiles (6x6)

    // Bake Perlin-Worley in R and Worley in G channels
    float pw = gradientNoise(p * SIZE);
    float w = worley(p, 4.0f);

    OutputAtlas[DTid.xy] = float4(pw, w, 0, 1);
}