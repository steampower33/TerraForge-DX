
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

float sdCutSphere(float3 p, float r, float h)
{
    float w = sqrt(r * r - h * h);

    float2 q = float2(length(p.xz), p.y);
    float s = max((h - r) * q.x * q.x + w * w * (h + r - 2.0 * q.y), h * q.x - w * q.y);
    return (s < 0.0) ? length(q) - r :
         (q.x < w) ? h - q.y :
                   length(q - float2(w, h));
}

float sdBox(float3 p, float3 b)
{
    float3 d = abs(p) - b;
    return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0f));
}