#include "Common.hlsli"

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float3 sdSphere(float3 p, float s)
{
    return length(p) - s;
}

float rayMarch(float3 ro, float3 rd)
{
    float dO = 0.0; // Distance from Origin
    for (int i = 0; i < 100; i++)
    {
        float3 p = ro + rd * dO;
        float dS = sdSphere(p, 1.0); // Distance to Scene
        dO += dS;
        if (dO > 100.0 || dS < 0.001)
            break; // Hit or Miss
    }
    return dO;
}

float4 main(VS_OUTPUT input) : SV_Target
{
    float2 p = (input.uv - 0.5) * 2.0;
    p.x *= Resolution.x / Resolution.y;
    p.y = -p.y;
    
    float3 ro = CameraPos;
    float3 fwd = CameraDir;
    float3 right = CameraRight;
    float3 up = CameraUp;
    
    const float fl = 2.5;
    float3 rd = normalize(p.x * right + p.y * up + fl * fwd);
    
    float d = rayMarch(ro, rd);

    float3 col = float3(0, 0, 0);
    if (d < 100.0)
    {
        col = float3(1, 1, 1) * (1.0 - d / 20.0);
    }

    return float4(col, 1.0);
}