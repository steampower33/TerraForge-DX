
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

float2 intersectAABB(float3 ro, float3 rd, float3 bMin, float3 bMax)
{
    float3 tMin = (bMin - ro) / rd;
    float3 tMax = (bMax - ro) / rd;
    
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    
    return float2(tNear, tFar);
}