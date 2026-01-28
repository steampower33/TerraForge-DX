
#include "Common.hlsli"
#include "SDF.hlsli"
#include "Intersect.hlsli"

#define STEPS_PRIMARY 32
#define STEPS_LIGHT 6

static const float3 CloudExtent = float3(100.0f, 40.0f, 100.0f);
static const float3 SigmaS = float3(1.0f, 1.0f, 1.0f);
static const float3 SigmaA = float3(0.0f, 0.0f, 0.0f);
static const float3 PhaseParams = float3(-0.1f, 0.3f, 0.7f); // g1, g2, weight

static const float3 SigmaE = max(SigmaS + SigmaA, float3(1e-6, 1e-6, 1e-6));

Texture2D NoiseAtlas : register(t0);
Texture2D BlueNoiseTex : register(t1);
SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// =================================================================================
// Helper Functions
// =================================================================================

float getGlow(float dist, float radius, float intensity)
{
    dist = max(dist, 1e-6);
    return pow(radius / dist, intensity);
}

float3 getSky(float3 rd)
{
    float3 zenithColor = float3(0.09f, 0.33f, 0.81f) * 0.7f;
    float3 horizonColor = float3(0.6f, 0.7f, 0.8f);
    
    float horizonMix = pow(1.0f - max(rd.y, 0.0f), 4.0f);
    float3 sky = lerp(zenithColor, horizonColor, horizonMix);

    float sunDot = max(dot(rd, SunDir), 0.0f);
    float sunDisk = getGlow(1.0f - sunDot, 0.0005f, 1.5f);
    float sunBloom = getGlow(1.0f - sunDot, 0.02f, 0.8f);

    float3 sunColor = float3(1.0, 1.0, 1.0);
    float3 sunGlow = sunColor * (sunDisk + sunBloom * 0.5f);

    return sky + sunGlow;
}

float circularOut(float t)
{
    return sqrt((2.0f - t) * t);
}

float remap(float x, float low1, float high1, float low2, float high2)
{
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

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

float getCloudMap(float3 p)
{
    float2 uv = p.xz / (1.8f * CloudExtent.x);
    float dist = circularOut(saturate(1.0f - length(uv * 5.0f)));
    dist = max(dist, 0.8f * circularOut(saturate(1.0f - length(uv * 6.0f + 0.65f))));
    dist = max(dist, 0.75f * circularOut(saturate(1.0f - length(uv * 7.8f - 0.75f))));
    return dist;
}

float getDensity(float3 p)
{
    // Use Hardcoded CloudExtent
    if (abs(p.x) > CloudExtent.x || abs(p.z) > CloudExtent.z || p.y < 0.0f || p.y > CloudExtent.y)
        return 0.0f;

    float cloudHeight = saturate(p.y / CloudExtent.y);
    float cloudMap = getCloudMap(p);
    if (cloudMap <= 0.0f)
        return 0.0f;

    float hLimit = pow(cloudMap, 0.75f);
    float verticalShaping = saturate(remap(cloudHeight, 0.0f, 0.25f * (1.0f - cloudMap), 0.0f, 1.0f))
                          * saturate(remap(cloudHeight, 0.75f * hLimit, hLimit, 1.0f, 0.0f));
    
    float baseDensity = cloudMap * verticalShaping;

    float3 shapePos = p * CloudScale * 0.4f + float3(Time * 2.0f, 0.0f, Time);
    float shapeNoise = getPerlinWorleyNoise(shapePos);
    float density = saturate(remap(baseDensity, ShapeStrength * shapeNoise, 1.0f, 0.0f, 1.0f));

    if (density <= 0.01f)
        return 0.0f;

    float3 detailPos = p * CloudScale * 0.8f + float3(Time * 3.0f, -Time * 3.0f, Time);
    float detailNoise = getPerlinWorleyNoise(detailPos);
    density = saturate(remap(density, DetailStrength * detailNoise, 1.0f, 0.0f, 1.0f));

    return density * DensityMult;
}

float HenyeyGreenstein(float g, float costh)
{
    return (1.0 / (4.0 * 3.14159)) * ((1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * costh, 1.5));
}

float3 multipleOctaves(float density, float mu, float stepL)
{
    float3 luminance = float3(0, 0, 0);
    float a = 1.0, b = 1.0, c = 1.0;
    
    // Use Hardcoded SigmaE
    float3 sigmaE = SigmaE;

    for (int i = 0; i < 4; i++)
    {
        // Use Hardcoded PhaseParams
        float phase = lerp(HenyeyGreenstein(PhaseParams.x * c, mu),
                           HenyeyGreenstein(PhaseParams.y * c, mu),
                           PhaseParams.z);
                           
        luminance += b * phase * exp(-stepL * density * sigmaE * a);
        a *= 0.2f;
        b *= 0.5f;
        c *= 0.5f;
    }
    return luminance;
}

float3 lightRay(float3 p, float mu)
{
    // Use Macro STEPS_LIGHT
    float stepL = (CloudExtent.y * 0.75f) / float(STEPS_LIGHT);
    float densityAcc = 0.0f;

    for (int j = 0; j < STEPS_LIGHT; j++)
    {
        densityAcc += getDensity(p + SunDir * (float(j) * stepL));
    }

    float3 beersLaw = multipleOctaves(densityAcc, mu, stepL);
    
    // Use Hardcoded SigmaE
    float3 sigmaE = SigmaE;
    float3 powder = 2.0f * (1.0f - exp(-stepL * densityAcc * 2.0f * sigmaE));
    
    return lerp(beersLaw * powder, beersLaw, 0.5f + 0.5f * mu);
}

// =================================================================================
// Main Pixel Shader
// =================================================================================

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 screenP = (input.uv - 0.5f) * 2.0f;
    screenP.x *= Resolution.x / Resolution.y;
    screenP.y = -screenP.y;
    
    float3 rd = normalize(screenP.x * CameraRight + screenP.y * CameraUp + 1.0f * CameraDir);
    float3 ro = CameraPos;
    float mu = dot(rd, SunDir);

    float3 skyColor = getSky(rd);
    float3 finalColor = skyColor;
    
    // Use Hardcoded CloudExtent
    float3 minCorner = float3(-CloudExtent.x, 0.0f, -CloudExtent.z);
    float3 maxCorner = float3(CloudExtent.x, CloudExtent.y, CloudExtent.z);
    
    float2 hit = intersectAABB(ro, rd, minCorner, maxCorner);
    
    if (hit.x <= hit.y && hit.y >= 0)
    {
        float tStart = max(0.0f, hit.x);
        
        float2 noiseUV = input.pos.xy / 64.0f;
        float blueNoise = BlueNoiseTex.Sample(PointSampler, noiseUV).r;
        float goldenRatio = 1.61803398875f;
        float dithering = frac(blueNoise + (Time * 60.0f) * goldenRatio);
        
        // Use Macro STEPS_PRIMARY
        float stepS = (hit.y - hit.x) / float(STEPS_PRIMARY);
        float t = tStart + stepS * dithering;

        float3 cloudColor = float3(0, 0, 0);
        float3 transmittance = float3(1.0f, 1.0f, 1.0f);
        
        // Use Hardcoded PhaseParams
        float phaseFunction = lerp(HenyeyGreenstein(PhaseParams.x, mu),
                                   HenyeyGreenstein(PhaseParams.y, mu),
                                   PhaseParams.z);
        
        // Use Hardcoded SigmaE
        float3 sigmaE = SigmaE;

        for (int i = 0; i < STEPS_PRIMARY; i++)
        {
            float3 p = ro + rd * t;
            
            if (p.y > CloudExtent.y || p.y < 0.0f)
            {
                t += stepS;
                continue;
            }

            float density = getDensity(p);

            if (density > 0.01f)
            {
                float3 baseSunColor = float3(1.0f, 1.0f, 1.0f);

                float3 ambient = baseSunColor * lerp(0.2f, 0.8f, saturate(p.y / CloudExtent.y));
                float3 sunLight = baseSunColor * SunIntensity * phaseFunction * lightRay(p, mu);
                
                float3 luminance = 0.1f * ambient + sunLight;
                luminance *= SigmaS * density; // Use Hardcoded SigmaS

                float3 stepTransmittance = exp(-sigmaE * density * stepS);
                
                cloudColor += transmittance * (luminance - luminance * stepTransmittance) / (sigmaE * density);
                transmittance *= stepTransmittance;

                if (length(transmittance) < 0.01f)
                    break;
            }
            t += stepS;
        }
        
        finalColor = cloudColor + (skyColor * transmittance);
    }

    finalColor *= 0.5f;
    finalColor = saturate((finalColor * (2.51f * finalColor + 0.03f)) / (finalColor * (2.43f * finalColor + 0.59f) + 0.14f));
    
    return float4(pow(finalColor, 0.4545f), 1.0f);
}