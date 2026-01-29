#include "Common.hlsli"
#include "SDF.hlsli"
#include "Intersect.hlsli"

#define STEPS_PRIMARY 32
#define STEPS_LIGHT 6

static const float3 CloudExtent = float3(100.0, 40.0, 100.0);
static const float3 SigmaS = float3(1.0, 1.0, 1.0);
static const float3 SigmaA = float3(0.0, 0.0, 0.0);
static const float3 PhaseParams = float3(-0.1, 0.3, 0.7); // g1, g2, weight
static const float GoldenRatio = 1.61803398875;

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
    float3 zenithColor = float3(0.09, 0.33, 0.81) * 0.7;
    float3 horizonColor = float3(0.6, 0.7, 0.8);
    
    float horizonMix = pow(1.0 - max(rd.y, 0.0), 4.0);
    float3 sky = lerp(zenithColor, horizonColor, horizonMix);

    float mu = 0.5 + 0.5 * dot(rd, SunDir);
    float sunDisk = getGlow(1.0 - mu, 0.00015, 0.9);

    float3 sunColor = float3(1.0, 1.0, 1.0);
    float3 sunGlow = sunColor * sunDisk;

    return sky + sunGlow;
}

float circularOut(float t)
{
    return sqrt((2.0 - t) * t);
}

float remap(float x, float low1, float high1, float low2, float high2)
{
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

float getPerlinWorleyNoise(float3 pos)
{
    const float atlasSize = 204.0;
    const float tileSize = 32.0;
    const float tileRows = 6.0;
    
    float3 p = pos.xzy;
    float3 coord = fmod(abs(p), float3(tileSize, tileSize, 36.0));
    
    float level = floor(coord.z);
    float f = frac(coord.z);

    float tileY = floor(level / tileRows);
    float tileX = fmod(level, tileRows);

    float2 offset = float2(tileX, tileY) * (tileSize + 2.0) + 1.0;
    float2 pixel = coord.xy + offset + 0.5;
    
    float2 data = NoiseAtlas.SampleLevel(LinearSampler, pixel / atlasSize, 0).xy;
    return lerp(data.x, data.y, f);
}

float getCloudMap(float3 p)
{
    float2 uv = p.xz / (1.8 * CloudExtent.x);
    float dist = circularOut(saturate(1.0 - length(uv * 5.0)));
    dist = max(dist, 0.8 * circularOut(saturate(1.0 - length(uv * 6.0 + 0.65))));
    dist = max(dist, 0.75 * circularOut(saturate(1.0 - length(uv * 7.8 - 0.75))));
    return dist;
}

float getDensity(float3 p)
{
    if (abs(p.x) > CloudExtent.x || abs(p.z) > CloudExtent.z || p.y < 0.0 || p.y > CloudExtent.y)
        return 0.0;

    float cloudHeight = saturate(p.y / CloudExtent.y);
    float cloudMap = getCloudMap(p);
    if (cloudMap <= 0.0)
        return 0.0;

    float hLimit = pow(cloudMap, 0.75);
    float verticalShaping = saturate(remap(cloudHeight, 0.0, 0.25 * (1.0 - cloudMap), 0.0, 1.0))
                          * saturate(remap(cloudHeight, 0.75 * hLimit, hLimit, 1.0, 0.0));
    
    float baseDensity = cloudMap * verticalShaping;

    float3 shapePos = p * CloudScale * 0.4 + float3(Time * 2.0, 0.0, Time);
    float shapeNoise = getPerlinWorleyNoise(shapePos);
    float density = saturate(remap(baseDensity, ShapeStrength * shapeNoise, 1.0, 0.0, 1.0));

    if (density <= 0.01)
        return 0.0;

    float3 detailPos = p * CloudScale * 0.8 + float3(Time * 3.0, -Time * 3.0, Time);
    float detailNoise = getPerlinWorleyNoise(detailPos);
    density = saturate(remap(density, DetailStrength * detailNoise, 1.0, 0.0, 1.0));

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
    
    float3 sigmaE = SigmaE;

    for (int i = 0; i < 4; i++)
    {
        float phase = lerp(HenyeyGreenstein(PhaseParams.x * c, mu),
                           HenyeyGreenstein(PhaseParams.y * c, mu),
                           PhaseParams.z);
                           
        luminance += b * phase * exp(-stepL * density * sigmaE * a);
        a *= 0.2;
        b *= 0.5;
        c *= 0.5;
    }
    return luminance;
}

float3 lightRay(float3 p, float mu)
{
    float stepL = (CloudExtent.y * 0.75) / float(STEPS_LIGHT);
    float densityAcc = 0.0;

    for (int j = 0; j < STEPS_LIGHT; j++)
    {
        densityAcc += getDensity(p + SunDir * (float(j) * stepL));
    }

    float3 beersLaw = multipleOctaves(densityAcc, mu, stepL);
    
    float3 sigmaE = SigmaE;
    float3 powder = 2.0 * (1.0 - exp(-stepL * densityAcc * 2.0 * sigmaE));
    
    return lerp(beersLaw * powder, beersLaw, 0.5 + 0.5 * mu);
}

// =================================================================================
// Main Pixel Shader
// =================================================================================

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 screenP = (input.uv - 0.5) * 2.0;
    screenP.x *= Resolution.x / Resolution.y;
    screenP.y = -screenP.y;
    
    float3 rd = normalize(screenP.x * CameraRight + screenP.y * CameraUp + 1.0 * CameraDir);
    float3 ro = CameraPos;
    float mu = dot(rd, SunDir);

    float3 skyColor = getSky(rd);
    float3 finalColor = skyColor;
    
    float3 minCorner = float3(-CloudExtent.x, 0.0, -CloudExtent.z);
    float3 maxCorner = float3(CloudExtent.x, CloudExtent.y, CloudExtent.z);
    
    float2 hit = intersectAABB(ro, rd, minCorner, maxCorner);
    
    if (hit.x <= hit.y && hit.y >= 0.0)
    {
        float tStart = max(0.0, hit.x);
        
        float2 noiseUV = input.pos.xy / 64.0;
        float blueNoise = BlueNoiseTex.Sample(PointSampler, noiseUV).r;
        float dithering = frac(blueNoise + (Time * 60.0) * GoldenRatio);
        
        float stepS = (hit.y - hit.x) / float(STEPS_PRIMARY);
        float t = tStart + stepS * dithering;

        float3 cloudColor = float3(0, 0, 0);
        float3 transmittance = float3(1.0, 1.0, 1.0);
        
        float phaseFunction = lerp(HenyeyGreenstein(PhaseParams.x, mu),
                                   HenyeyGreenstein(PhaseParams.y, mu),
                                   PhaseParams.z);
        
        float3 sigmaE = SigmaE;

        for (int i = 0; i < STEPS_PRIMARY; i++)
        {
            float3 p = ro + rd * t;
            
            if (p.y > CloudExtent.y || p.y < 0.0)
            {
                t += stepS;
                continue;
            }

            float density = getDensity(p);

            if (density > 0.01)
            {
                float3 baseSunColor = float3(1.0, 1.0, 1.0);

                float3 ambient = baseSunColor * lerp(0.2, 0.8, saturate(p.y / CloudExtent.y));
                float3 sunLight = baseSunColor * SunIntensity * phaseFunction * lightRay(p, mu);
                
                float3 luminance = 0.1 * ambient + sunLight;
                luminance *= SigmaS * density;

                float3 stepTransmittance = exp(-sigmaE * density * stepS);
                
                cloudColor += transmittance * (luminance - luminance * stepTransmittance) / (sigmaE * density);
                transmittance *= stepTransmittance;

                if (length(transmittance) < 0.01)
                    break;
            }
            t += stepS;
        }
        
        finalColor = cloudColor + (skyColor * transmittance);
    }

    finalColor *= 0.5;
    finalColor = saturate((finalColor * (2.51 * finalColor + 0.03)) / (finalColor * (2.43 * finalColor + 0.59) + 0.14));
    
    return float4(pow(finalColor, 0.4545), 1.0);
}