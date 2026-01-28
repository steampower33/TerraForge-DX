/*
 * Integrated Cloud Shader (No-Texture Version)
 * Uses 'Interleaved Gradient Noise' instead of Blue Noise Texture.
 * No C++ changes required. Just compile and run!
 * https://www.shadertoy.com/view/3sffzj 
*/

#include "Common.hlsli"
#include "SDF.hlsli"
#include "Intersect.hlsli"

// --- Constants ---
#define STEPS_PRIMARY 32
#define STEPS_LIGHT 6
#define CLOUD_EXTENT 100.0f
static const float3 SIGMA_S = float3(1.0f, 1.0f, 1.0f);
static const float3 SIGMA_A = float3(0.0f, 0.0f, 0.0f);
static const float3 SIGMA_E = max(SIGMA_S + SIGMA_A, float3(1e-6, 1e-6, 1e-6));

static const float POWER = 200.0f;
static const float DENSITY_MULTIPLIER = 0.5f;

// --- Resources ---
Texture2D NoiseAtlas : register(t0); // Baked Perlin-Worley
Texture2D BlueNoiseTex : register(t1);

SamplerState LinearSampler : register(s0); // For Cloud
SamplerState PointSampler : register(s1); // For Blue Noise

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Calculates the sun's halo/glow effect (Mie scattering approximation)
float getGlow(float dist, float radius, float intensity)
{
    dist = max(dist, 1e-6);
    return pow(radius / dist, intensity);
}

// Generates the background sky color based on view direction and sun position
float3 getSky(float3 rd)
{
    // 1. Base Gradient: Interpolate between Horizon color (FogColor) and Zenith color (Deep Blue)
    // We use FogColor from your cbuffer for the horizon to blend seamlessly with geometry fog
    float3 zenithColor = float3(0.09f, 0.33f, 0.81f) * 0.7f; // Deep sky blue
    float3 horizonColor = FogColor; // Use scene fog color for horizon
    
    // Mix based on vertical look direction (rd.y)
    float horizonMix = pow(1.0f - max(rd.y, 0.0f), 4.0f);
    float3 sky = lerp(zenithColor, horizonColor, horizonMix);

    // 2. Sun Glare/Spot
    float sunDot = max(dot(rd, SunDir), 0.0f);
    
    // Add a sharp sun disk
    float sunDisk = getGlow(1.0f - sunDot, 0.0005f, 1.5f);
    
    // Add a wide atmospheric bloom around the sun
    float sunBloom = getGlow(1.0f - sunDot, 0.02f, 0.8f);
    
    // Combine sun effects with SunColor strength
    float3 sunGlow = SunColor * (sunDisk + sunBloom * 0.5f);

    return sky + sunGlow;
}

// --- Helper Functions ---

float circularOut(float t)
{
    return sqrt((2.0f - t) * t);
}

float remap(float x, float low1, float high1, float low2, float high2)
{
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

// [Procedural Dithering] Interleaved Gradient Noise (Jorge Jimenez, Activision)
// Generates a high-frequency noise pattern without a texture look-up.
float GetIGN(float2 pixelXY)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixelXY, magic.xy)));
}

// Atlas Sampling (No changes)
float getPerlinWorleyNoise(float3 pos)
{
    const float atlasSize = 204.0f;
    const float tileSize = 32.0f;
    const float tileRows = 6.0f;
    
    float3 p = pos.xzy;
    float3 coord = fmod(abs(p), float3(tileSize, tileSize, 36.0f));
    
    float level = floor(coord.z);
    float f = frac(coord.z);

    float tileY = floor(level / tileRows);
    float tileX = fmod(level, tileRows);

    float2 offset = float2(tileX, tileY) * (tileSize + 2.0f) + 1.0f;
    float2 pixel = coord.xy + offset + 0.5f;
    
    float2 data = NoiseAtlas.SampleLevel(LinearSampler, pixel / atlasSize, 0).xy;
    return lerp(data.x, data.y, f);
}

// Cloud Map (No changes)
float getCloudMap(float3 p)
{
    float2 uv = p.xz / (1.8f * CLOUD_EXTENT);
    float dist = circularOut(saturate(1.0f - length(uv * 5.0f)));
    dist = max(dist, 0.8f * circularOut(saturate(1.0f - length(uv * 6.0f + 0.65f))));
    dist = max(dist, 0.75f * circularOut(saturate(1.0f - length(uv * 7.8f - 0.75f))));
    return dist;
}

// Density Function (No changes)
float getDensity(float3 p)
{
    if (abs(p.x) > CLOUD_EXTENT || abs(p.z) > CLOUD_EXTENT || p.y < 0.0f || p.y > CLOUD_EXTENT)
        return 0.0f;

    float cloudHeight = saturate(p.y / CLOUD_EXTENT);
    float cloudMap = getCloudMap(p);
    if (cloudMap <= 0.0f)
        return 0.0f;

    float hLimit = pow(cloudMap, 0.75f);
    float verticalShaping = saturate(remap(cloudHeight, 0.0f, 0.25f * (1.0f - cloudMap), 0.0f, 1.0f))
                          * saturate(remap(cloudHeight, 0.75f * hLimit, hLimit, 1.0f, 0.0f));
    
    float baseDensity = cloudMap * verticalShaping;

    float3 shapePos = p * CloudScale * 0.4f + float3(Time * 2.0f, 0.0f, Time);
    float shapeNoise = getPerlinWorleyNoise(shapePos);
    float density = saturate(remap(baseDensity, 0.6f * shapeNoise, 1.0f, 0.0f, 1.0f));

    if (density <= 0.01f)
        return 0.0f;

    float3 detailPos = p * CloudScale * 0.8f + float3(Time * 3.0f, -Time * 3.0f, Time);
    float detailNoise = getPerlinWorleyNoise(detailPos);
    density = saturate(remap(density, 0.35f * detailNoise, 1.0f, 0.0f, 1.0f));

    return density * DENSITY_MULTIPLIER;
}

float HenyeyGreenstein(float g, float costh)
{
    return (1.0 / (4.0 * 3.14159)) * ((1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * costh, 1.5));
}

float3 multipleOctaves(float extinction, float mu, float stepL)
{
    float3 luminance = float3(0, 0, 0);
    float a = 1.0, b = 1.0, c = 1.0;
    for (int i = 0; i < 4; i++)
    {
        float phase = lerp(HenyeyGreenstein(-0.1f * c, mu), HenyeyGreenstein(0.3f * c, mu), 0.7f);
        luminance += b * phase * exp(-stepL * extinction * SIGMA_E * a);
        a *= 0.2f;
        b *= 0.5f;
        c *= 0.5f;
    }
    return luminance;
}

float3 lightRay(float3 p, float mu)
{
    float stepL = (CLOUD_EXTENT * 0.75f) / float(STEPS_LIGHT);
    float densityAcc = 0.0f;

    for (int j = 0; j < STEPS_LIGHT; j++)
    {
        densityAcc += getDensity(p + SunDir * (float(j) * stepL));
    }

    float3 beersLaw = multipleOctaves(densityAcc, mu, stepL);
    float powder = 2.0f * (1.0f - exp(-stepL * densityAcc * 2.0f));
    return lerp(beersLaw * powder, beersLaw, 0.5f + 0.5f * mu);
}

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 screenP = (input.uv - 0.5f) * 2.0f;
    screenP.x *= Resolution.x / Resolution.y;
    screenP.y = -screenP.y;
    
    float3 rd = normalize(screenP.x * CameraRight + screenP.y * CameraUp + 1.0f * CameraForward);
    float3 ro = CameraPos;
    float mu = dot(rd, SunDir);

    float3 skyColor = getSky(rd);
    float3 finalColor = skyColor;
    
    // AABB Intersection
    float3 minCorner = float3(-CLOUD_EXTENT, 0.0f, -CLOUD_EXTENT);
    float3 maxCorner = float3(CLOUD_EXTENT, CLOUD_EXTENT, CLOUD_EXTENT);
    float2 hit = intersectAABB(ro, rd, minCorner, maxCorner);
    
    if (hit.x <= hit.y && hit.y >= 0)
    {
        float tStart = max(0.0f, hit.x);
        
        // Blue Noise Texture
        float2 pixelPos = input.pos.xy;
        //float dithering = GetIGN(pixelPos + float2(Time * 60.0f, 0.0f));
        float2 noiseUV = input.pos.xy / 64.0f;
        float blueNoise = BlueNoiseTex.Sample(PointSampler, noiseUV).r;
        float goldenRatio = 1.61803398875f;
        float dithering = frac(blueNoise + (Time * 60.0f) * goldenRatio);
        
        float stepS = (hit.y - hit.x) / float(STEPS_PRIMARY);
        float t = tStart + stepS * dithering;

        // Raymarching Loop variables
        float3 cloudColor = float3(0, 0, 0);
        float transmittance = 1.0f;
        float phaseFunction = lerp(HenyeyGreenstein(-0.3f, mu), HenyeyGreenstein(0.3f, mu), 0.7f);

        for (int i = 0; i < STEPS_PRIMARY; i++)
        {
            float3 p = ro + rd * t;
            float density = getDensity(p);

            if (density > 0.01f)
            {
                float3 ambient = SunColor * lerp(0.2f, 0.8f, saturate(p.y / CLOUD_EXTENT));
                float3 sunLight = SunColor * POWER * phaseFunction * lightRay(p, mu);
                
                float3 luminance = 0.1f * ambient + sunLight;
                luminance *= SIGMA_S * density;

                float3 stepTransmittance = exp(-SIGMA_E * density * stepS);
                cloudColor += transmittance * (luminance - luminance * stepTransmittance) / (SIGMA_E * density);
                transmittance *= stepTransmittance;

                if (transmittance < 0.01f)
                    break;
            }
            t += stepS;
        }
        
        finalColor = cloudColor + (skyColor * transmittance);
    }

    finalColor = saturate((finalColor * (2.51f * finalColor + 0.03f)) / (finalColor * (2.43f * finalColor + 0.59f) + 0.14f));
    
    return float4(pow(finalColor, 0.4545f), 1.0f);
}