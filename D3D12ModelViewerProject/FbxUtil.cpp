#include "FbxUtil.h"
#include <algorithm>
#include "MathHelper.h"
#include <set>
#include <filesystem>
#include "FileUtil.h"
#include "PipelineState.h"
#include <cmath>

using namespace std;
using namespace DirectX;

std::string WstringToUTF8(std::wstring w_str)
{
	char* pUTF8;

	FbxWCToUTF8(w_str.c_str(), pUTF8);
	std::string result(pUTF8);

	delete[] pUTF8;

	return result;
}

std::wstring UTF8ToWstring(const char* pUTF8)
{
	wchar_t* pWide;
	FbxUTF8ToWC(pUTF8, pWide);
	std::wstring result(pWide);

	delete[] pWide;

	return result;
}

DirectX::XMFLOAT3 ToXMFLOAT3(FbxPropertyT<FbxDouble3> double3)
{
	DirectX::XMFLOAT3 result(
		double3.Get()[0],
		double3.Get()[1],
		double3.Get()[2]
	);
	return result;
}

DirectX::XMFLOAT3 ToXMFLOAT3(FbxDouble3 double3)
{
	DirectX::XMFLOAT3 result(
		double3[0],
		double3[1],
		double3[2]
	);
	return result;
}


DirectX::XMVECTOR GetXMVector(float x, float y, float z)
{
	DirectX::XMFLOAT3 f3(x, y, z);
	return DirectX::XMLoadFloat3(&f3);
}

float MaterialConverter::CalculateRoughness(FbxDouble3 specular, float shinness)
{
	float specularIntensity = CalcuateSpecularIntensity(specular);
	return sqrt(2 / (shinness * specularIntensity + 2));
}

double MaterialConverter::CalculateDiffuseBrightness(FbxDouble3 diffuse)
{
	return (0.299 * diffuse[0] * diffuse[0]) + (0.587 * diffuse[2] * diffuse[2]) + (0.114 * diffuse[2] * diffuse[2]);
}

double MaterialConverter::CalcuateSpecularIntensity(FbxDouble3 specular)
{
	return specular[0] * 0.2125 + specular[1] * 0.7154 + specular[2] * 0.0721;
}

double MaterialConverter::CalculateSpecularBrightness(FbxDouble3 specular)
{
	return (0.299 * specular[0] * specular[0]) + (0.587 * specular[2] * specular[2]) + (0.114 * specular[2] * specular[2]);
}

double MaterialConverter::CalculateSpecularStrength(FbxDouble3 specular)
{
	return std::max(std::max(specular[0], specular[1]), specular[2]);
}

double MaterialConverter::CalculateMetalness(double specularStrength, double diffuseBrightness, double specularBrightness)
{
	double dielectricSpecularReflectance = 0.04;
	double oneMinusSpecularStrength = 1 - specularStrength;

	double& A = dielectricSpecularReflectance;
	double B = (diffuseBrightness * (oneMinusSpecularStrength / (1 - A)) + specularBrightness) - 2 * A;
	double C = A - specularBrightness;

	double squareRoot = sqrt(std::max(0.0, B * B - 4 * A * C));
	double value = (-B + squareRoot) / (2 * A);
	double Metalness = std::clamp(value, 0.0, 1.0);

	return Metalness;
}

FbxDouble3 MaterialConverter::CalculateAlbedo(FbxDouble3 diffuse, FbxDouble3 specular, double specularStrength, double metalness)
{
	double dielectricSpecularReflectance = 0.04;
	double oneMinusSpecularStrength = 1 - specularStrength;

	FbxDouble3 AlbedoRGB;

	for (int i = 0; i < 3; ++i)
	{
		double dielectricColor = diffuse[i] * (oneMinusSpecularStrength / (1.0f - dielectricSpecularReflectance) / max(1e-4, 1.0 - metalness));
		double metalColor = (specular[i] - dielectricSpecularReflectance * (1.0 - metalness)) * (1.0 / max(1e-4, metalness));
		AlbedoRGB[i] = clamp(lerp(dielectricColor, metalColor, metalness * metalness), 0.0, 1.0);
	}

	return AlbedoRGB;
}

std::string FindFbxTextureName(FbxProperty& prop)
{
	int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
	std::string textureName;

	if (layeredTextureCount > 0)
	{
		return textureName;
	}

	int textureCount = prop.GetSrcObjectCount<FbxTexture>();
	if (textureCount == 1)
	{
		FbxTexture* texture = prop.GetSrcObject<FbxTexture>(0);
		if (texture->GetClassId().Is(FbxFileTexture::ClassId))
		{
			FbxFileTexture* fileTex = (FbxFileTexture*)texture;
			textureName = fileTex->GetRelativeFileName();
		}
	}
	else
	{
		//assert(false);
	}

	return textureName;
}

std::set<FbxFileTexture*> GetFbxMaterialTextures(const FbxSurfaceMaterial& material)
{
	std::set<FbxFileTexture*> textureSet;

	int textureIndex;
	FBXSDK_FOR_EACH_TEXTURE(textureIndex)
	{
		FbxProperty prop = material.FindProperty(FbxLayerElement::sTextureChannelNames[textureIndex]);
		if (prop.IsValid())
		{
			auto AddSrcTextureToSet = [&textureSet](const auto& InObject)
			{
				int textureCount = InObject.template GetSrcObjectCount<FbxTexture>();
				for (int texIndex = 0; texIndex < textureCount; ++texIndex)
				{
					FbxFileTexture* texture = InObject.template GetSrcObject<FbxFileTexture>(texIndex);
					if (texture)
					{
						textureSet.insert(texture);
					}
				}
			};

			const int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
			if (layeredTextureCount > 0)
			{
				for (int layerIndex = 0; layerIndex < layeredTextureCount; ++layerIndex)
				{
					if (const FbxLayeredTexture* layeredTexture = prop.GetSrcObject<FbxLayeredTexture>(layerIndex))
					{
						AddSrcTextureToSet(*layeredTexture);
					}
				}
			}
			else
			{
				AddSrcTextureToSet(prop);
			}
		}
	}

	return textureSet;
}


PBRMaterial MaterialConverter::ToPBRMaterial(FbxSurfaceMaterial* fbxMaterial)
{
	PBRMaterial pbrMaterial;

	pbrMaterial.Name = fbxMaterial->GetName();
	if (fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
	{
		FbxSurfacePhong* phong = (FbxSurfacePhong*)fbxMaterial;

		FbxDouble3 diffuse = phong->Diffuse.Get();
		FbxDouble3 specular = FbxUtil::FbxDouble3Mul(phong->Specular.Get(), phong->SpecularFactor.Get());
		double specularStrength = CalculateSpecularStrength(specular);
		double metalness = CalculateMetalness(specularStrength, CalculateDiffuseBrightness(diffuse), CalculateSpecularBrightness(specular));
		FbxDouble3 albedo = CalculateAlbedo(diffuse, specular, specularStrength, metalness);

		FbxDouble transpar = phong->TransparencyFactor.Get();
		float opacity = 1.0f - (float)phong->TransparencyFactor.Get();

		pbrMaterial.Albedo = SimpleMath::Color(albedo[0], albedo[1], albedo[2], 1.0f);
		pbrMaterial.Roughness = CalculateRoughness(specular, phong->Shininess.Get());
		pbrMaterial.Metalic = metalness;
	}
	else if (fbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		FbxSurfaceLambert* lambert = (FbxSurfacePhong*)fbxMaterial;

		FbxDouble3 diffuse = lambert->Diffuse.Get();
		float opacity = 1.0f - (float)lambert->TransparencyFactor.Get();

		pbrMaterial.Albedo = SimpleMath::Color(diffuse[0], diffuse[1], diffuse[2], 1.0f);
	}

	return pbrMaterial;
}

Vertex VertexConverter::ConvertFromSubMeshVertex(SubMeshVertex subMeshVertex)
{
	Vertex vertex;
	vertex.Pos = subMeshVertex.Position;
	vertex.Normal = subMeshVertex.Normal;
	vertex.TexC = subMeshVertex.TexC;

	std::vector<BlendingIndexWeightPair>& boneWeightPairs = subMeshVertex.BoneWeights;
	sort(boneWeightPairs.begin(), boneWeightPairs.end(), [](const BlendingIndexWeightPair& w1, const BlendingIndexWeightPair& w2) {return w1.BlendingWeight > w2.BlendingWeight; });

	if (subMeshVertex.BoneWeights.size() >= 4)
	{
		vertex.BoneIndices.x = boneWeightPairs.at(0).BlendingIndex;
		vertex.BoneIndices.y = boneWeightPairs.at(1).BlendingIndex;
		vertex.BoneIndices.z = boneWeightPairs.at(2).BlendingIndex;
		vertex.BoneIndices.w = boneWeightPairs.at(3).BlendingIndex;

		vertex.BoneWeights.x = boneWeightPairs.at(0).BlendingWeight;
		vertex.BoneWeights.y = boneWeightPairs.at(1).BlendingWeight;
		vertex.BoneWeights.z = boneWeightPairs.at(2).BlendingWeight;
		vertex.BoneWeights.w = boneWeightPairs.at(3).BlendingWeight;

		float sumOfWeights = 0;
		sumOfWeights += vertex.BoneWeights.x;
		sumOfWeights += vertex.BoneWeights.y;
		sumOfWeights += vertex.BoneWeights.z;
		sumOfWeights += vertex.BoneWeights.w;

		vertex.BoneWeights.x /= sumOfWeights;
		vertex.BoneWeights.y /= sumOfWeights;
		vertex.BoneWeights.z /= sumOfWeights;
		vertex.BoneWeights.w /= sumOfWeights;
	}

	return vertex;
}


RenderType GetRenderType(const PBRMaterial& material, MeshType meshType)
{
	RenderType result;
	bool isOpaque = material.Albedo.A() >= 0.95f;

	switch (meshType)
	{
	case MeshType::StaticMesh:
		result.ShaderMode = ShaderType::StaticMeshShader;
		break;
	case MeshType::DynamicMesh:
		result.ShaderMode = ShaderType::DynamicMeshShader;
		break;
	default:
		result.ShaderMode = ShaderType::StaticMeshShader;
	}

	if (isOpaque)
	{
		result.BlendMode = BlendType::Opaque;
	}
	else
	{
		result.BlendMode = BlendType::Transparent;
	}

	return result;
}

bool IsFbx(std::wstring filePath)
{
	return (FileUtil::GetFileExtension(filePath) == _TEXT(".fbx"));
}

void BoneAnimation::Interpolate(float time, DirectX::XMMATRIX& boneTransform) const
{
	using namespace DirectX;
	XMMATRIX globalBindposeInverse = Joint.GlobalBindposeInverseTransform.GetFinalTransformMatrix();

	if (keyFrames.empty())
	{
		boneTransform = globalBindposeInverse;
		return;
	}

	if (time < 0)
	{

		boneTransform = XMMatrixMultiply(globalBindposeInverse, keyFrames.front().Transform.GetFinalTransformMatrix());

	}
	else if (time > GetEndTime())
	{

		boneTransform = XMMatrixMultiply(globalBindposeInverse, keyFrames.back().Transform.GetFinalTransformMatrix());

	}
	else
	{
		float currentFrame = time * GetFramePerSecond();

		for (size_t i = 0; i < keyFrames.size() - 1; ++i)
		{
			const int leftframe = i;
			const int rightframe = i + 1;

			if (currentFrame >= leftframe && currentFrame <= rightframe)
			{
				float lerpPercent = (currentFrame - leftframe);

				const AffineMatrix& leftFrameTransform = keyFrames.at(leftframe).Transform;
				const AffineMatrix& rightFrameTransform = keyFrames.at(rightframe).Transform;

				XMVECTOR scale = XMVectorLerp(leftFrameTransform.Scale, rightFrameTransform.Scale, lerpPercent);
				XMVECTOR translation = XMVectorLerp(leftFrameTransform.Translation, rightFrameTransform.Translation, lerpPercent);
				XMVECTOR quarternion = XMQuaternionSlerp(leftFrameTransform.Quaternion, rightFrameTransform.Quaternion, lerpPercent);

				boneTransform = XMMatrixMultiply(globalBindposeInverse, XMMatrixAffineTransformation(scale, XMVectorZero(), quarternion, translation));
				break;
			}
		}
	}
}

float BoneAnimation::GetFramePerSecond() const
{
	float framePerSecond = 1.0f;

	switch (FrameMode)
	{
	case KeyFrameModes::KeyFrame24:
		framePerSecond = 24.0f;
		break;
	default:
		framePerSecond = 1.0f;
		break;
	}
	return framePerSecond;
}

void AnimationClip::Interpolate(float time, std::vector<DirectX::XMMATRIX>& boneTransforms) const
{
	boneTransforms.resize(BoneAnimations.size());
	for (size_t i = 0; i < BoneAnimations.size(); ++i)
	{
		const BoneAnimation& boneAnimation = BoneAnimations.at(i);
		if (boneAnimation.keyFrames.empty() == true)
		{
			boneTransforms.at(i) = boneTransforms.at(boneAnimation.Joint.ParentIndex);
		}
		else
		{
			boneAnimation.Interpolate(time, boneTransforms.at(i));
		}
	}
}

float AnimationClip::GetEndTime() const
{
	float t = 0.0f;
	for (UINT i = 0; i < BoneAnimations.size(); ++i)
	{
		t = (std::max)(t, BoneAnimations.at(i).GetEndTime());
	}
	return t;
}
