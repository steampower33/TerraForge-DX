#include "Common.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Generates a simple sky gradient based on ray direction
float3 getSky(float3 rd)
{
    float3 sc = lerp(float3(1.0, 1.0, 1.0), float3(0.1, 0.5, 1.0), clamp(rd.y, -1.0, 1.0) * 0.5 + 0.5);
    return sc;
}

// --- Signed Distance Functions (SDF) ---

// Sphere Distance Function
float sdSphere(float3 p, float s)
{
    return length(p) - s;
}

// Vertical Cylinder Distance Function
float sdCylinder(float3 p, float2 h)
{
    float2 d = abs(float2(length(p.xz), p.y)) - h;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

// Smoothly blends two distance fields using a smoothing factor k
float opSmoothUnion(float d1, float d2, float k)
{
    float h = saturate(0.5 + 0.5 * (d2 - d1) / k);
    return lerp(d2, d1, h) - k * h * (1.0 - h);
}

// --- Intersection Logic ---

// Determines if a ray hits a bounding sphere for optimization
bool intersectSphere(float3 ro, float3 rd, float rad, out float tMin, out float tMax)
{
    float3 oc = ro;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - rad * rad;
    float h = b * b - c;
    
    if (h < 0.0)
        return false;
    
    h = sqrt(h);
    tMin = -b - h;
    tMax = -b + h;
    
    return true;
}

// Determines if a ray hits a horizontal cloud layer slab
bool intersectCloudLayer(float3 ro, float3 rd, float minH, float maxH, out float tMin, out float tMax)
{
    if (abs(rd.y) < 0.0001)
        return false;

    float t1 = (minH - ro.y) / rd.y;
    float t2 = (maxH - ro.y) / rd.y;

    tMin = max(0.0, min(t1, t2));
    tMax = max(t1, t2);

    return tMax > 0.0 && tMin < 100.0;
}

// --- Procedural Noise Functions ---

// Simple hash for pseudo-random value generation
float hash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Standard 3D Value Noise
float noise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(lerp(hash(i + float3(0, 0, 0)), hash(i + float3(1, 0, 0)), f.x),
             lerp(hash(i + float3(0, 1, 0)), hash(i + float3(1, 1, 0)), f.x), f.y),
        lerp(lerp(hash(i + float3(0, 0, 1)), hash(i + float3(1, 0, 1)), f.x),
             lerp(hash(i + float3(0, 1, 1)), hash(i + float3(1, 1, 1)), f.x), f.y), f.z);
}

// Fractal Brownian Motion for layered cloud details
float fbm(float3 p)
{
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; i++)
    {
        v += noise(p) * amp;
        p *= 2.0;
        amp *= 0.5;
    }
    return v;
}

// --- Volumetric Rendering ---

// Defines the cloud's volume by combining SDF shapes and noise erosion
float getDensity(float3 p)
{
    float s0 = sdSphere(p - float3(0, 0, 0), 5.0);
    float s1 = sdSphere(p - float3(0, 8, 0), 3.0);
    
    float d = opSmoothUnion(s0, s1, 5.0);
    if (d > 2.0)
        return 0.0;

    float3 noisePos = p * iCloudScale + float3(0, iTime * 0.2, 0);
    float n = fbm(noisePos);
    
    // Calculate final density based on distance and noise offset
    return saturate((-d * 0.5 + n) - iCloudThreshold);
}

// Main raymarching loop with light marching for self-shadowing
float3 getCloud(float3 ro, float3 rd)
{
    float tMin, tMax;
    if (!intersectSphere(ro, rd, 40.0, tMin, tMax))
        return float3(0, 0, 0);

    float3 col = float3(0, 0, 0);
    float transmittance = 1.0;

    for (float t = tMin; t < tMax; t += iStepSize)
    {
        float3 pos = ro + rd * t;
        float d = getDensity(pos);

        if (d > 0.01)
        {
            // Light Marching: Estimate light attenuation towards the sun
            float lightStep = 0.5;
            float shadowDensity = 0.0;
            for (int j = 0; j < 4; j++)
            {
                shadowDensity += getDensity(pos + -iSunDir * (float(j) * lightStep));
            }

            // Beer-Lambert Law for self-shadowing
            float lightTransmittance = exp(-shadowDensity * iAbsorption);
            float curAbs = d * iStepSize * iAbsorption;
            
            // Accumulate color and update transmittance
            col += transmittance * curAbs * iSunColor * lightTransmittance;
            transmittance *= exp(-curAbs);

            // Optimization: Stop if the volume becomes nearly opaque
            if (transmittance < 0.01)
                break;
        }
    }
    return col;
}

// --- Main Entry Point ---

float4 main(VS_OUTPUT input) : SV_Target
{
    // Screen-space coordinates setup
    float2 p = (input.uv - 0.5) * 2.0;
    p.x *= iResolution.x / iResolution.y;
    p.y = -p.y;
    
    // View ray setup
    float3 ro = iCameraPos;
    float3 fwd = iCameraForward;
    float3 right = iCameraRight;
    float3 up = iCameraUp;
    const float fl = 1.0;
    float3 rd = normalize(p.x * right + p.y * up + fl * fwd);
    
    // Rendering passes
    float3 skyColor = getSky(rd);
    float3 cloudColor = getCloud(ro, rd);
    
    // Composition and Gamma Correction
    float3 finalColor = cloudColor;
    finalColor = pow(finalColor, 0.4545);
    
    return float4(finalColor, 1.0);
}