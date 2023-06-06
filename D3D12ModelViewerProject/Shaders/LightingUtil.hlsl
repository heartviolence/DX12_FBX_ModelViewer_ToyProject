//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16
#define M_PI 3.1415926538

struct Light
{
    float3 Direction; // directional ,spot
    float Power; // Point,Spot
    float3 Color; // All
    float3 Position; // point ,spot
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
} 
 

float DistrubutionTrowbridgeReitzGGX(float3 normal, float3 halfVec, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(normal, halfVec), 0.0f);
    
    float num = alpha2;
    float denom = M_PI * pow(NdotH * NdotH * (alpha2 - 1.0f) + 1.0f, 2.0f);
    
    return num / denom;
}

float GeometrySchlickGGX(float3 normal, float3 v, float roughness)
{
    float k = pow((roughness + 1.0f), 2.0f) / 8.0f;
    float NdotV = max(dot(normal, v), 0.0f);
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 normal, float3 v, float3 light, float roughness)
{
    float ggx1 = GeometrySchlickGGX(normal, v, roughness);
    float ggx2 = GeometrySchlickGGX(normal, light, roughness);
    
    return ggx1 * ggx2;
}

float3 CalculateColor(float3 radiance, float3 albedo, float3 toLight, float3 toEyeW, float3 normal, float Metalic, float roughness)
{
    float3 F0 = 0.04f;
    F0 = lerp(F0, albedo, Metalic);
    
    float3 halfVector = normalize(toEyeW + toLight);
        
        //cook-torrance BRDF
    float NDF = DistrubutionTrowbridgeReitzGGX(normal, halfVector, roughness);
    float3 F = SchlickFresnel(F0, normal, toLight);
    float G = GeometrySmith(normal, toEyeW, toLight, roughness);
        
    float3 kS = F;
    float3 kD = 1.0f - kS;
    kD *= (1.0f - Metalic);
        
    float3 numerator = NDF * F * G;
    float NdotL = max(dot(normal, toLight), 0.0f);
    float denominator = 4.0 * max(dot(normal, toEyeW), 0.0f) * NdotL + 0.0001f; // 0으로 나눌수없게 0.0001을더함
    float3 specular = numerator / denominator;
    
    return ((kD * albedo / M_PI) + specular) * radiance * NdotL;
}