
// Include structures and functions for lighting.
#include "LightingUtil.hlsl"


SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per material.
cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float3 gAmbient;
    
    uint gDirectionalLightCount;
    uint gPointLightCount;
};

cbuffer ObjectConstant : register(b1)
{
    float4x4 gObjectTransform;
};

struct InstanceData
{
    float4x4 World;
    uint NumOfBones;
};

//cbuffer MaterialData : register(b2)
//{
//    float4 gDiffuseAlbedo;
//    float3 gFresnelR0;
//    float gRoughness;
//    float3 gAmbientLight;
//    float padding;
//    float4x4 gMatTransform;
//};

Texture2D gAlbedoMap : register(t0);
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<Light> gLights : register(t0, space3);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

cbuffer PBRMaterial : register(b2)
{
    float4 Albedo;
    float Metalic;
    float Roughness;
    uint padd0;
    uint padd1;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;

    InstanceData instData = gInstanceData[instanceID];

    float4x4 World = mul(instData.World, gObjectTransform);
	// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), World);
    vout.PosW = posW.xyz;
    
	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) World);

	// Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
    
    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 albedo = (gAlbedoMap.Sample(gsamAnisotropicWrap, pin.TexC) * Albedo).rgb;
    
	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

	// Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    
    float3 Lo = 0.0f;
    
    for (uint DirectionalLightIndex = 0; DirectionalLightIndex < gDirectionalLightCount; ++DirectionalLightIndex)
    {
        //calculate per-light radiance
        float3 toLight = normalize(-gLights[DirectionalLightIndex].Direction);
        float3 halfVector = normalize(toEyeW + toLight);
        float3 radiance = gLights[DirectionalLightIndex].Color * gLights[DirectionalLightIndex].Power;
        
        Lo += CalculateColor(radiance, Albedo.rgb, toLight, toEyeW, pin.NormalW, Metalic, Roughness);
    }
    
    for (uint pointLightIndex = gDirectionalLightCount; pointLightIndex < gDirectionalLightCount + gPointLightCount; ++pointLightIndex)
    {
        //calculate per-light radiance
        float3 toLight = normalize(gLights[pointLightIndex].Position - pin.PosW);
        float3 halfVector = normalize(toEyeW + toLight);
        float distance = length(gLights[pointLightIndex].Position - pin.PosW);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = gLights[pointLightIndex].Color * gLights[pointLightIndex].Power * attenuation;
        
        //cook-torrance BRDF
        Lo += CalculateColor(radiance, albedo, toLight, toEyeW, pin.NormalW, Metalic, Roughness);
    }
    
    float3 litColor = Lo + +gAmbient * albedo;

    litColor = litColor / (litColor + 1.0);
    litColor = pow(litColor, (1.0 / 2.2));

    return float4(litColor, Albedo.a);
}


