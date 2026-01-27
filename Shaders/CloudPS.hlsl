#include "Common.hlsli"
#include "SDF.hlsli"
#include "Intersect.hlsli"
#include "Noise.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Generates a simple sky gradient based on ray direction
float3 getSky(float3 rd)
{
    float3 skyColour = 0.7f * float3(0.09, 0.33, 0.81);
    return lerp(skyColour, 0.5f * skyColour, 0.5f + 0.5f * rd.y);
}

// --- Volumetric Rendering ---

// Defines the cloud's volume by combining SDF shapes and noise erosion
float getDensity(float3 p)
{
    // Using the cut sphere SDF as currently defined
    float d = sdCutSphere(p, 10.0f, -3.0f);
    
    if (d > 2.0f)
        return 0.0f;

    // Use updated 'CloudScale' and 'Time' variables
    float3 noisePos = p * CloudScale + float3(0, Time * 0.2f, 0);
    float n = fbm(noisePos);
    
    // Calculate final density using 'CloudThreshold'
    return saturate((-d * 0.5f + n) - CloudThreshold);
}

// Main raymarching loop with light marching for self-shadowing
float3 getCloud(float3 ro, float3 rd, float2 hit)
{
    float3 col = float3(0, 0, 0);
    float transmittance = 1.0f;

    float3 hitPoint = ro + rd * hit.x;
    float range = hit.y - hit.x;
    
    for (float t = 0.0f; t < range; t += StepSize)
    {
        float3 pos = hitPoint + rd * t;
        float d = getDensity(pos);

        if (d > 0.01f)
        {
            // Light Marching towards the sun
            float lightStep = 0.5f;
            float shadowDensity = 0.0f;
            for (int j = 0; j < 4; j++)
            {
                // Use updated 'SunDir' (assuming -SunDir for light direction)
                shadowDensity += getDensity(pos + -SunDir * (float(j) * lightStep));
            }

            // Beer-Lambert Law for self-shadowing
            float lightTransmittance = exp(-shadowDensity * Absorption);
            float curAbs = d * StepSize * Absorption;
            
            // Accumulate color using 'SunColor'
            col += transmittance * curAbs * SunColor * lightTransmittance;
            transmittance *= exp(-curAbs);

            // Optimization: Stop if the volume becomes nearly opaque
            if (transmittance < 0.01f)
                break;
        }
    }
    return col;
}

// --- Main Entry Point ---
float4 main(VS_OUTPUT input) : SV_Target
{
    // Screen-space coordinates setup using updated 'Resolution'
    float2 p = (input.uv - 0.5f) * 2.0f;
    p.x *= Resolution.x / Resolution.y;
    p.y = -p.y;
    
    // View ray setup using updated Camera vectors
    float3 ro = CameraPos;
    float3 fwd = CameraForward;
    float3 right = CameraRight;
    float3 up = CameraUp;
    
    const float fl = 1.0f;
    float3 rd = normalize(p.x * right + p.y * up + fl * fwd);
    
    // Rendering passes
    float3 skyColor = getSky(rd);
    
    // Define the bounding box for the cloud
    float3 bMin = float3(-20.0f, -20.0f, -20.0f);
    float3 bMax = float3(20.0f, 20.0f, 20.0f);
    float2 hit = intersectAABB(ro, rd, bMin, bMax);
    
    float3 cloudColor = float3(0.0f, 0.0f, 0.0f);
    if (hit.x <= hit.y)
    {
        cloudColor = getCloud(ro, rd, hit);
    }
    
    // Composition and Gamma Correction
    float3 finalColor = skyColor + cloudColor;
    finalColor = pow(finalColor, 0.4545f); // Approx gamma 2.2
    
    return float4(finalColor, 1.0f);
}