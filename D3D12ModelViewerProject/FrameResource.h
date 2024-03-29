#pragma once

#include "D3DUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"


struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
	DirectX::XMFLOAT4 BoneWeights;
	DirectX::XMUINT4 BoneIndices;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 ObjectTransform = MathHelper::Identity4x4();
};

struct InstanceConstants
{
	DirectX::XMFLOAT4X4 Transform = MathHelper::Identity4x4();
	UINT numOfBones = 0;
};

struct InstanceAnimations
{
	DirectX::XMFLOAT4X4 BoneTransform = MathHelper::Identity4x4();
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f,0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f,0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
	DirectX::XMFLOAT3 Ambient = { 0.1f,0.1f,0.1f };

	UINT DirectionalLightCount = 0;
	UINT PointLightCount = 0;
};

