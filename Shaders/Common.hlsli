
cbuffer cbGlobal : register(b0)
{
    float3 CameraPos;
    float Time;
    float3 CameraDir;
    float pad0;
    float3 CameraRight;
    float pad1;
    float3 CameraUp;
    float pad2;
    float2 Resolution;
    float2 pad3;
};

cbuffer cbCloudParams : register(b1)
{
    float3 SunDir;
    float SunIntensity;

    float CloudScale;
    float ShapeStrength;
    float DetailStrength;
    float DensityMult;
};