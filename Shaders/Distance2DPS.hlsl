/*
 * 2D Signed Distance Field Functions
 * * Original Math by Inigo Quilez
 * Reference: https://iquilezles.org/articles/distfunctions2d/
 * * Ported from GLSL to HLSL by [SeungMin Lee]
 */

#include "Common.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float3 sdCircle(float2 p, float s)
{
    return length(p) - s;
}

float sdBox(in float2 p, in float2 b)
{
    float2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 p = (input.uv - 0.5) * 2.0;
    p.x *= Resolution.x / Resolution.y;
    p.y = -p.y;
    
    //float d = sdCircle(p, 0.5);
    float d = sdBox(p, float2(0.5, 0.2));
    
    float3 col = (d > 0.0) ? float3(1.0, 0.45, 0.26) : float3(1.0, 0.97, 0.87);
    col *= 1.0 - exp(-5.0 * abs(d));
    col *= 0.8 + 0.2 * cos(180.0 * d);
    col = lerp(col, float3(1.0, 1.0, 1.0), 1.0 - smoothstep(0.0, 0.01, abs(d)));
    
    return float4(col, 1.0);
}