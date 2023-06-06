#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include <algorithm>
#include <string>
#include <fbxsdk.h>
#include <DirectXPackedVector.h>
#include <set>
#include "FrameResource.h"
#include <map> 
#include "D3DUtil.h" 
#include "RenderTypes.h"

struct Keyframe;
struct TempPolygon;
struct BlendingIndexWeightPair;
struct Skeleton;
struct ControlPoint;
struct MeshMaterialConstants;
struct MeshMaterial;
struct SubMeshVertex;
class FbxUtil;
class MaterialConverter;
class VertexConverter;
struct LayerRenderItems;
struct TempPolygon;
struct Keyframe;
struct Joint;
struct Skeleton;
struct BlendingIndexWeightPair;
struct ControlPoint;
struct BoneAnimation;
struct AnimationClip;
enum class KeyFrameModes;

struct MeshMaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f,0.01f,0.01f };
	float Roughness = 0.25f;

	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct SubMeshVertex
{
	DirectX::XMFLOAT3 Position = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 Normal = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 TangentU = { 0.0f,0.0f ,0.0f };
	DirectX::XMFLOAT2 TexC = { 0.0f,0.0f };
	std::string MaterialName;

	std::vector<BlendingIndexWeightPair> BoneWeights;

	bool operator==(const SubMeshVertex& rhs) const
	{
		return (Position.x == rhs.Position.x &&
			Position.y == rhs.Position.y &&
			Position.z == rhs.Position.z &&
			Normal.x == rhs.Normal.x &&
			Normal.y == rhs.Normal.y &&
			Normal.z == rhs.Normal.z &&
			TangentU.x == rhs.TangentU.x &&
			TangentU.y == rhs.TangentU.y &&
			TangentU.z == rhs.TangentU.z &&
			TexC.x == rhs.TexC.x &&
			TexC.y == rhs.TexC.y &&
			(MaterialName.compare(rhs.MaterialName) == 0));
	}
};


std::string WstringToUTF8(std::wstring w_str);

std::wstring UTF8ToWstring(const char* pUTF8);

DirectX::XMFLOAT3 ToXMFLOAT3(FbxPropertyT<FbxDouble3> double3);
DirectX::XMFLOAT3 ToXMFLOAT3(FbxDouble3 double3);
DirectX::XMVECTOR GetXMVector(float x, float y, float z);

inline void XMFLOAT3Multiply(DirectX::XMFLOAT3& float3, float float1)
{
	float3.x *= float1;
	float3.y *= float1;
	float3.z *= float1;
}



FbxDouble3 CalculateAlbedo(FbxDouble3 diffuse, FbxDouble3 specular, double specularStrength, double metalness);

std::string FindFbxTextureName(FbxProperty& prop);

std::set<FbxFileTexture*> GetFbxMaterialTextures(const FbxSurfaceMaterial& material);

RenderType GetRenderType(const PBRMaterial& material, MeshType meshType);

bool IsFbx(std::wstring filePath);

class FbxUtil
{
public:
	static FbxAMatrix GetGeometryTransformation(FbxNode* pNode)
	{
		if (pNode == nullptr)
		{
			throw std::exception("Null for mesh Geometry");
		}
		FbxVector4 transition = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 rotation = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 scale = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

		return FbxAMatrix(transition, rotation, scale);
	}

	static DirectX::XMVECTOR ConvertToXMVector(const FbxVector4& vector)
	{
		return DirectX::XMVectorSet(vector[0], vector[1], vector[2], vector[3]);
	}

	static DirectX::XMMATRIX ConvertToXMMatrix(const FbxAMatrix& fbxAMatrix)
	{
		return DirectX::XMMATRIX(ConvertToXMVector(fbxAMatrix.GetRow(0)),
			ConvertToXMVector(fbxAMatrix.GetRow(1)),
			ConvertToXMVector(fbxAMatrix.GetRow(2)),
			ConvertToXMVector(fbxAMatrix.GetRow(3)));
	}

	static AffineMatrix ConvertToAffineMatrix(const FbxAMatrix& fbxAMatrix)
	{
		AffineMatrix affineMatrix;

		FbxQuaternion quat = fbxAMatrix.GetQ();

		affineMatrix.Scale = ConvertToXMVector(fbxAMatrix.GetS());
		affineMatrix.Translation = ConvertToXMVector(fbxAMatrix.GetT());
		affineMatrix.Quaternion = DirectX::XMVectorSet(quat[0], quat[1], quat[2], quat[3]);
		return affineMatrix;
	}

	static FbxDouble3 FbxDouble3Mul(FbxDouble3 d3, double d)
	{
		return FbxDouble3(d3[0] * d, d3[1] * d, d3[2] * d);
	}
};

class MaterialConverter
{
public:

	static PBRMaterial ToPBRMaterial(FbxSurfaceMaterial* fbxMaterial);
private:
	static float CalculateRoughness(FbxDouble3 specular, float shinness);
	static double CalculateDiffuseBrightness(FbxDouble3 diffuse);
	static double CalcuateSpecularIntensity(FbxDouble3 specular);
	static double CalculateSpecularBrightness(FbxDouble3 specular);
	static double CalculateSpecularStrength(FbxDouble3 specular);
	static double CalculateMetalness(double specularStrength, double diffuseBrightness, double specularBrightness);
	static FbxDouble3 CalculateAlbedo(FbxDouble3 diffuse, FbxDouble3 specular, double specularStrength, double metalness);
};


class VertexConverter
{
public:
	static Vertex ConvertFromSubMeshVertex(SubMeshVertex subMeshVertex);
};

struct TempPolygon
{
	std::vector<SubMeshVertex> Vertices;
	int Count = 3;
};

struct Keyframe
{
	FbxLongLong FrameNum;
	FbxAMatrix GlobalTransform;

	AffineMatrix Transform;
};

struct Joint
{
	int	ParentIndex;
	std::string Name;
	FbxAMatrix GlobalBindposeInverse;
	AffineMatrix GlobalBindposeInverseTransform;
	AffineMatrix LocalTransform;

	FbxNode* Node = nullptr;
	int Depth;
};

struct Skeleton
{
	std::string Name;
	std::vector<Joint> Joints;
};

struct BlendingIndexWeightPair
{
	size_t BlendingIndex = 0;
	double BlendingWeight = 0;
};


struct ControlPoint
{
	std::vector<BlendingIndexWeightPair> BlendingInfo;
};

enum class KeyFrameModes : int
{
	KeyFrame24 = 0
};

struct BoneAnimation
{
public:
	Joint Joint;
	std::vector<Keyframe> keyFrames;
	KeyFrameModes FrameMode = KeyFrameModes::KeyFrame24;

	void Interpolate(float time, DirectX::XMMATRIX& boneTransform) const;

	float GetFramePerSecond() const;	

	float GetEndTime() const
	{
		return keyFrames.size() / GetFramePerSecond();
	}
};

struct AnimationClip
{
	std::string Name;
	//Joint
	std::vector<BoneAnimation> BoneAnimations;

	void Interpolate(float time, std::vector<DirectX::XMMATRIX>& boneTransforms) const;

	float GetEndTime() const;
};