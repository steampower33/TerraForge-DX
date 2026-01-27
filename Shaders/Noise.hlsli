// --- Procedural Noise Functions ---

// Simple hash for pseudo-random value generation
float hash(float3 p)
{
    return frac(sin(p) * 753.5453123f);
}

// Standard 3D Value Noise
float noise(float3 x)
{
    float3 p = floor(x);
    float3 f = frac(x);
    f = f * f * (3.0f - 2.0f * f);
    
    float n = p.x + p.y * 157.0f + 113.0f * p.z;
    return lerp(lerp(lerp(hash(n + 0.0f), hash(n + 1.0f), f.x),
                     lerp(hash(n + 157.0f), hash(n + 158.0f), f.x), f.y),
                lerp(lerp(hash(n + 113.0f), hash(n + 114.0f), f.x),
                     lerp(hash(n + 270.0f), hash(n + 271.0f), f.x), f.y), f.z);
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

float cloudNoise(float scale, float3 p, float3 dir)
{
    float3 q = p + dir;
    float f;
    f = 0.50000f * noise(q);
    q = q * scale * 2.02f + dir;
    f += 0.25000f * noise(q);
    q = q * 2.03f + dir;
    f += 0.12500f * noise(q);
    q = q * 2.01f + dir;
    f += 0.06250f * noise(q);
    q = q * 2.02f + dir;
    f += 0.03125f * noise(q);
    return f;
}