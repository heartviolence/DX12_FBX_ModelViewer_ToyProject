#include "FbxModelScene.h"
#include "FbxUtil.h"
#include <unordered_set>
#include <set>
#include <functional>
#include "D3DResourceManager.h"
#include <algorithm>
#include "GameTimer.h"
#include "FileUtil.h"

using namespace std;
using namespace DirectX;


FbxModelScene::FbxModelScene(std::wstring filename)
{
	bool result;

	m_manager = FbxManager::Create();
	assert(m_manager != nullptr);

	FbxIOSettings* ios = FbxIOSettings::Create(m_manager, IOSROOT);
	m_manager->SetIOSettings(ios);

	m_scene = FbxScene::Create(m_manager, "");
	assert(m_scene != nullptr);

	std::string fileStr = WstringToUTF8(filename);
	FbxString filePath(fileStr.c_str());
	result = LoadScene(m_manager, m_scene, filePath.Buffer());
	m_sceneEvaluator = m_scene->GetAnimationEvaluator();
	ProcessMaterialTable(m_scene);
	ProcessScene(m_scene);

	m_directory = FileUtil::GetDirectory(filename);
	m_name = WstringToUTF8(FileUtil::GetFileNameWithoutExtension(filename));
}

FbxModelScene::~FbxModelScene()
{
	if (m_scene != nullptr)
	{
		m_scene->Destroy();
	}
	if (m_manager != nullptr)
	{
		m_manager->Destroy();
	}
}

bool FbxModelScene::LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int fileMajor, fileMinor, fileRevision;
	int SDKMajor, SDKMinor, SDKRevision;
	bool status;

	FbxManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);

	FbxImporter* importer = FbxImporter::Create(pManager, "");
	const bool ImportStatus = importer->Initialize(pFilename, -1, pManager->GetIOSettings());
	importer->GetFileVersion(fileMajor, fileMinor, fileRevision);

	if (!ImportStatus)
	{
		FbxString error = importer->GetStatus().GetErrorString();
		return false;
	}

	status = importer->Import(m_scene);

	if (status == false || (importer->GetStatus() != FbxStatus::eSuccess))
	{
		return false;
	}

	RenameDuplicatedMaterial(m_scene);

	importer->Destroy();

	return status;
}

void FbxModelScene::ProcessNode(FbxNode* pNode)
{
	FbxNodeAttribute* attribute = pNode->GetNodeAttribute();

	if (attribute != nullptr)
	{
		switch (attribute->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			LoadMesh(pNode);
			break;
		default:
			break;
		}
	}

	int childCount = pNode->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		ProcessNode(pNode->GetChild(i));
	}
}

void FbxModelScene::ProcessScene(FbxScene* pScene)
{
	FbxNode* node = pScene->GetRootNode();

	if (node == nullptr)
	{
		return;
	}

	ProcessSkeletonHierachy(node);

	for (int i = 0; i < node->GetChildCount(); ++i)
	{
		ProcessNode(node->GetChild(i));
	}

}

void FbxModelScene::LoadMesh(FbxNode* pNode)
{
	FbxMesh* mesh = (FbxMesh*)pNode->GetNodeAttribute();

	if (mesh->RemoveBadPolygons() < 0)
	{
		return;
	}
	ProcessPolygons(mesh);
}

void FbxModelScene::RenameDuplicatedMaterial(FbxScene* pScene)
{
	FbxArray<FbxSurfaceMaterial*> materialArray;
	pScene->FillMaterialArray(materialArray);

	set<string> AllMaterialAndTextureNames;
	set<FbxFileTexture*> MaterialTextures;

	auto FixName = [this, &AllMaterialAndTextureNames](const string& assetName,
		std::function<void(const string&)> applyFunction)
	{
		string	uniqueName(assetName);
		//중복이 있으면 이름수정
		if (AllMaterialAndTextureNames.find(uniqueName) != AllMaterialAndTextureNames.end())
		{
			string assetBaseName = uniqueName + "_v_";
			int nameIndex = 1;
			do
			{
				uniqueName = assetBaseName + to_string(nameIndex++);
			} while (AllMaterialAndTextureNames.find(uniqueName) != AllMaterialAndTextureNames.end());

			applyFunction(uniqueName);
		}
		AllMaterialAndTextureNames.insert(uniqueName);
	};

	for (int materialIndex = 0; materialIndex < materialArray.Size(); ++materialIndex)
	{
		FbxSurfaceMaterial* material = materialArray[materialIndex];
		std::string materialName = material->GetName();
		//마테리얼 별 텍스처 가져온후 저장
		auto texSet = GetFbxMaterialTextures(*material);
		MaterialTextures.insert(texSet.begin(), texSet.end());

		//마테리얼 중복이름 고치기
		FixName(materialName,
			[&](const string uniqueName) { material->SetName(uniqueName.c_str()); });
	}

	for (FbxFileTexture* currentTexture : MaterialTextures)
	{
		string textureName = currentTexture->GetRelativeFileName();
		m_allFbxFileTexture.insert(std::make_pair(currentTexture, textureName));
	}
}

void FbxModelScene::ProcessMaterialTable(FbxScene* pScene)
{
	FbxArray<FbxSurfaceMaterial*> materialArray;
	pScene->FillMaterialArray(materialArray);

	for (int i = 0; i < materialArray.Size(); ++i)
	{
		m_fbxMaterials.push_back(materialArray.GetAt(i));
	}
}

FbxSurfaceMaterial* FbxModelScene::FindFbxMaterial(std::string UTF8MaterialName)
{
	return m_scene->GetMaterial(UTF8MaterialName.c_str());
}


std::shared_ptr<MeshResources> FbxModelScene::CreateMeshResource()
{
	MeshResourcesInfo meshResourceInfo;

	//fbxMaterial -> meshMaterial
	for (int i = 0; i < m_fbxMaterials.size(); ++i)
	{
		auto& fbxmaterial = m_fbxMaterials.at(i);

		PBRMaterial PBRMaterial = MaterialConverter::ToPBRMaterial(fbxmaterial);

		FbxProperty FbxDiffuseProperty = fbxmaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (FbxDiffuseProperty.IsValid())
		{
			wstring relativeName = UTF8ToWstring(FindFbxTextureName(FbxDiffuseProperty).c_str());
			PBRMaterial.AlbedoMap = FileUtil::GetAbsoluteFilePath(m_directory, relativeName);
		}

		meshResourceInfo.Materials.push_back(std::move(PBRMaterial));
	}

	//submeshVertex ->vertex
	meshResourceInfo.VertexTable.reserve(m_vertexTable.size());
	std::transform(m_vertexTable.begin(), m_vertexTable.end(), std::back_inserter(meshResourceInfo.VertexTable), VertexConverter::ConvertFromSubMeshVertex);

	meshResourceInfo.SubMeshes = m_subMeshes;
	meshResourceInfo.Skeleton = m_skeleton;

	return make_shared<MeshResources>(meshResourceInfo);
}

void FbxModelScene::Triangulate(TempPolygon& polygon, std::vector<TempPolygon>& output)
{
	assert(polygon.Count >= 3);

	output.clear();

	for (int i = 0; i < polygon.Count - 2; ++i)
	{
		TempPolygon temp;
		temp.Count = 3;

		temp.Vertices.push_back(polygon.Vertices.at(0));
		temp.Vertices.push_back(polygon.Vertices.at(i + 1));
		temp.Vertices.push_back(polygon.Vertices.at(i + 2));

		output.push_back(std::move(temp));
	}
}



void FbxModelScene::ProcessPolygons(FbxMesh* pMesh)
{
	if (pMesh->GetNode()->GetMaterialCount() <= 0)
	{
		return;
	}

	int polygonCount = pMesh->GetPolygonCount();
	FbxVector4* controlPoints = pMesh->GetControlPoints();

	FbxAMatrix transform = m_sceneEvaluator->GetNodeLocalTransform(pMesh->GetNode());

	std::vector<ControlPoint> controlPointboneWeights;
	ProcessJointsAndAnimations(pMesh->GetNode(), controlPointboneWeights);

	//매쉬내 머티리얼이 같은지 체크
	bool isAllSame = true;
	for (int i = 0; i < pMesh->GetElementMaterialCount(); ++i)
	{
		FbxGeometryElementMaterial* materialElement = pMesh->GetElementMaterial(i);
		if (materialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
		{
			isAllSame = false;
			break;
		}
	}

	//각 폴리곤별 정점에 대한 머티리얼 버텍스컬러,노말,탄젠트 등의 정보를 저장
	vector<SubMeshVertex> vertexInfoArray;

	vector<TempPolygon> polygonArray;
	polygonArray.reserve(polygonCount);

	int vertexId = 0;
	for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
	{
		int polygonSize = pMesh->GetPolygonSize(polygonIndex);
		assert(polygonSize >= 3);
		TempPolygon newPolygon;
		newPolygon.Count = polygonSize;

		for (int i = 0; i < polygonSize; i++)
		{
			SubMeshVertex vInfo;
			int controlPointIndex = pMesh->GetPolygonVertex(polygonIndex, i);
			FbxVector4 verPos = controlPoints[controlPointIndex];
			verPos = transform.MultT(verPos);

			vInfo.Position.x = (float)verPos[0];
			vInfo.Position.y = (float)verPos[1];
			vInfo.Position.z = (float)verPos[2];

			std::vector<BlendingIndexWeightPair>& boneWeights = controlPointboneWeights[controlPointIndex].BlendingInfo;
			vInfo.BoneWeights.insert(vInfo.BoneWeights.begin(), boneWeights.begin(), boneWeights.end());

			//for (int elementIndex = 0; elementIndex < pMesh->GetElementVertexColorCount(); ++elementIndex)
			//{
			//	FbxGeometryElementVertexColor* eVerColor = pMesh->GetElementVertexColor(elementIndex);

			//	switch (eVerColor->GetMappingMode())
			//	{
			//	default:
			//		break;
			//	case FbxGeometryElement::eByControlPoint:
			//		switch (eVerColor->GetReferenceMode())
			//		{
			//		case FbxGeometryElement::eDirect:
			//			int ColorIndex = controlPointIndex;
			//			break;
			//		case FbxGeometryElement::eIndexToDirect:
			//			int index = eVerColor->GetIndexArray().GetAt(controlPointIndex);
			//			int ColorIndex = index;
			//			break;
			//		default:
			//			break;
			//		}
			//		break;

			//	case FbxGeometryElement::eByPolygonVertex:
			//	{
			//		switch (eVerColor->GetReferenceMode())
			//		{
			//		case FbxGeometryElement::eDirect:
			//			int ColorIndex = vertexId;
			//			break;
			//		case FbxGeometryElement::eIndexToDirect:
			//			int ColorIndex = eVerColor->GetIndexArray().GetAt(vertexId);
			//			break;
			//		default:
			//			break;
			//		}
			//	}

			//	case FbxGeometryElement::eByPolygon:
			//	case FbxGeometryElement::eAllSame:
			//	case FbxGeometryElement::eNone:
			//		break;
			//	}
			//}

			// Get UV / FBX UV좌표는 1사분면
			for (int elementIndex = 0; elementIndex < pMesh->GetElementUVCount(); ++elementIndex)
			{
				FbxGeometryElementUV* eVerUV = pMesh->GetElementUV(elementIndex);

				switch (eVerUV->GetMappingMode())
				{
				default:
					break;
				case FbxGeometryElement::eByControlPoint:
					switch (eVerUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{

						int UVIndex = controlPointIndex;
						FbxVector2 UV = eVerUV->GetDirectArray().GetAt(UVIndex);
						vInfo.TexC.x = (float)UV[0];
						vInfo.TexC.y = 1 - (float)UV[1];
						break;
					}
					case FbxGeometryElement::eIndexToDirect:
					{
						int UVIndex = eVerUV->GetIndexArray().GetAt(controlPointIndex);
						FbxVector2 UV = eVerUV->GetDirectArray().GetAt(UVIndex);
						vInfo.TexC.x = (float)UV[0];
						vInfo.TexC.y = 1 - (float)UV[1];
						break;
					}
					default:
						break;
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:
				{
					int textureUVIndex = pMesh->GetTextureUVIndex(polygonIndex, i);
					switch (eVerUV->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					case FbxGeometryElement::eIndexToDirect:
					{
						int UVIndex = textureUVIndex;
						FbxVector2 UV = eVerUV->GetDirectArray().GetAt(UVIndex);
						vInfo.TexC.x = (float)UV[0];
						vInfo.TexC.y = 1 - (float)UV[1];
						break;
					}
					default:
						break;
					}
				}

				case FbxGeometryElement::eByPolygon:
				case FbxGeometryElement::eAllSame:
				case FbxGeometryElement::eNone:
					break;
				}
			}

			for (int elementIndex = 0; elementIndex < pMesh->GetElementNormalCount(); ++elementIndex)
			{
				FbxGeometryElementNormal* eVerNormal = pMesh->GetElementNormal(elementIndex);

				if (eVerNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					switch (eVerNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
					{
						int NormalIndex = vertexId;
						FbxVector4 Normal = eVerNormal->GetDirectArray().GetAt(NormalIndex);
						vInfo.Normal.x = (float)Normal[0];
						vInfo.Normal.y = (float)Normal[1];
						vInfo.Normal.z = (float)Normal[2];
						break;
					}
					case FbxGeometryElement::eIndexToDirect:
					{
						int NormalIndex = eVerNormal->GetIndexArray().GetAt(vertexId);
						FbxVector4 Normal = eVerNormal->GetDirectArray().GetAt(NormalIndex);
						vInfo.Normal.x = (float)Normal[0];
						vInfo.Normal.y = (float)Normal[1];
						vInfo.Normal.z = (float)Normal[2];
						break;
					}
					default:
						break;
					}
				}
			}

			//for (int elementIndex = 0; elementIndex < pMesh->GetElementTangentCount(); ++elementIndex)
			//{
			//	FbxGeometryElementTangent* eVerTangent = pMesh->GetElementTangent(elementIndex);

			//	if (eVerTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			//	{
			//		switch (eVerTangent->GetReferenceMode())
			//		{
			//		case FbxGeometryElement::eDirect:
			//			int TangentIndex = vertexId;
			//			break;
			//		case FbxGeometryElement::eIndexToDirect:
			//			int TangentIndex = eVerTangent->GetIndexArray().GetAt(vertexId);
			//			break;
			//		default:
			//			break;
			//		}
			//	}
			//}

			//for (int elementIndex = 0; elementIndex < pMesh->GetElementBinormalCount(); ++elementIndex)
			//{
			//	FbxGeometryElementBinormal* eVerBinormal = pMesh->GetElementBinormal(elementIndex);

			//	if (eVerBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			//	{
			//		switch (eVerBinormal->GetReferenceMode())
			//		{
			//		case FbxGeometryElement::eDirect:
			//			int BinormalIndex = vertexId;
			//			break;
			//		case FbxGeometryElement::eIndexToDirect:
			//			int BinormalIndex = eVerBinormal->GetIndexArray().GetAt(vertexId);
			//			break;
			//		default:
			//			break;
			//		}
			//	}
			//}

			int matId = -1;
			if (isAllSame)
			{
				FbxGeometryElementMaterial* materialElement = pMesh->GetElementMaterial(0);
				matId = materialElement->GetIndexArray().GetAt(0);
			}
			else
			{
				for (int elementalMaterialIndex = 0; elementalMaterialIndex < pMesh->GetElementMaterialCount(); ++elementalMaterialIndex)
				{
					FbxGeometryElementMaterial* materialElement = pMesh->GetElementMaterial(elementalMaterialIndex);
					matId = materialElement->GetIndexArray().GetAt(polygonIndex);
				}
			}
			if (matId >= 0)
			{
				FbxSurfaceMaterial* surfaceMaterial = pMesh->GetNode()->GetMaterial(matId);
				vInfo.MaterialName = surfaceMaterial->GetName();
			}

			vertexId++;
			newPolygon.Vertices.push_back(std::move(vInfo));
		}
		polygonArray.push_back(std::move(newPolygon));
	}

	vertexInfoArray.reserve(polygonArray.size() * 3);
	for (auto& element : polygonArray)
	{
		std::vector<TempPolygon> trianglePolygons;
		Triangulate(element, trianglePolygons);

		for (auto& triPolygon : trianglePolygons)
		{
			vertexInfoArray.push_back(triPolygon.Vertices.at(0));
			vertexInfoArray.push_back(triPolygon.Vertices.at(1));
			vertexInfoArray.push_back(triPolygon.Vertices.at(2));
		}
	}

	//겹치는 정점 최적화

	std::unordered_map<SubMeshVertex, IndexBufferFormat> hashMap;
	std::vector<IndexBufferFormat> tempIndexTable;
	tempIndexTable.reserve(vertexInfoArray.size());

	int indexTableElement = 0;
	for (auto& vInfo : vertexInfoArray)
	{
		auto findResult = hashMap.find(vInfo);

		if (findResult == hashMap.end())
		{
			hashMap[vInfo] = m_vertexTable.size();
			indexTableElement = m_vertexTable.size();
			m_vertexTable.push_back(vInfo);
		}
		else
		{
			indexTableElement = findResult->second;
		}
		tempIndexTable.push_back(indexTableElement);
	}


	//마테리얼 별로 분리
	for (auto& tempIndex : tempIndexTable)
	{
		string matName = m_vertexTable.at(tempIndex).MaterialName;

		m_subMeshes[matName].push_back(tempIndex);
	}
}

void FbxModelScene::ProcessSkeletonHierachy(FbxNode* pRootNode)
{
	for (int childIndex = 0; childIndex < pRootNode->GetChildCount(); ++childIndex)
	{
		FbxNode* childNode = pRootNode->GetChild(childIndex);
		ProcessSkeletonHierachyRecursively(childNode, 0, 0, -1);
	}
}

void FbxModelScene::ProcessSkeletonHierachyRecursively(FbxNode* pNode, int depth, int index, int parentIndex)
{
	if (pNode->GetNodeAttribute() && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint currJoint;
		currJoint.ParentIndex = parentIndex;
		currJoint.Name = pNode->GetName();
		currJoint.Depth = depth++;
		m_skeleton.Joints.push_back(currJoint);
	}

	for (int i = 0; i < pNode->GetChildCount(); i++)
	{
		ProcessSkeletonHierachyRecursively(pNode->GetChild(i), depth, m_skeleton.Joints.size(), index);
	}
}

void FbxModelScene::ProcessJointsAndAnimations(FbxNode* pNode, std::vector<ControlPoint>& controlPoints)
{
	FbxMesh* currMesh = pNode->GetMesh();
	controlPoints.clear();
	controlPoints.resize(currMesh->GetControlPointsCount());

	FbxAMatrix geometryTransform = FbxUtil::GetGeometryTransformation(pNode);
	size_t numOfDeformers = currMesh->GetDeformerCount();

	for (size_t deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
	{
		//skin만 처리
		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(currMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));

		if (!currSkin) { continue; }

		size_t numOfClusters = currSkin->GetClusterCount();
		for (size_t clusterIndex = 0; clusterIndex < numOfClusters; ++clusterIndex)
		{
			FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
			std::string currJointName = currCluster->GetLink()->GetName();
			size_t currJointIndex = FindJointIndexUsingName(currJointName);

			FbxAMatrix transformMatrix;
			FbxAMatrix transformLinkMatrix;

			//Get matrix associated with the node containing the link. 대부분의경우 항등행렬
			currCluster->GetTransformMatrix(transformMatrix);

			// The transformation of the mesh at binding time 
			currCluster->GetTransformLinkMatrix(transformLinkMatrix);

			// The transformation of the cluster(joint) at binding time from joint space to world space 
			FbxAMatrix globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			m_skeleton.Joints[currJointIndex].GlobalBindposeInverse = globalBindposeInverseMatrix;
			m_skeleton.Joints[currJointIndex].GlobalBindposeInverseTransform = FbxUtil::ConvertToAffineMatrix(globalBindposeInverseMatrix);
			m_skeleton.Joints[currJointIndex].LocalTransform = FbxUtil::ConvertToAffineMatrix(m_sceneEvaluator->GetNodeLocalTransform(pNode));
			m_skeleton.Joints[currJointIndex].Node = currCluster->GetLink();

			// Associate each joint with the control points it affects 
			size_t numOfIndices = currCluster->GetControlPointIndicesCount();
			for (size_t i = 0; i < numOfIndices; ++i)
			{
				BlendingIndexWeightPair currBlendingIndexWeightPair;
				currBlendingIndexWeightPair.BlendingIndex = currJointIndex;
				currBlendingIndexWeightPair.BlendingWeight = currCluster->GetControlPointWeights()[i];

				controlPoints[currCluster->GetControlPointIndices()[i]].BlendingInfo.push_back(currBlendingIndexWeightPair);
			}

			int animStackCount = m_scene->GetSrcObjectCount<FbxAnimStack>();

			for (int i = 0; i < animStackCount; ++i)
			{
				FbxAnimStack* currAnimStack = m_scene->GetSrcObject<FbxAnimStack>(i);
				FbxString animStackName = currAnimStack->GetName();

				string clipName = animStackName.Buffer();

				AnimationClip& animClip = m_animations[clipName];
				animClip.Name = clipName;
				animClip.BoneAnimations.resize(m_skeleton.Joints.size());
				animClip.BoneAnimations[currJointIndex].Joint = m_skeleton.Joints[currJointIndex];

				FbxTakeInfo* takeInfo = m_scene->GetTakeInfo(animStackName);
				FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
				FbxTime end = takeInfo->mLocalTimeSpan.GetStop();

				for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
				{
					Keyframe currFrame;
					FbxTime currTime;
					currTime.SetFrame(i, FbxTime::eFrames24);
					currFrame.FrameNum = i;

					FbxAMatrix currentTransformOffset = pNode->EvaluateGlobalTransform(currTime) * geometryTransform;
					currFrame.GlobalTransform = currentTransformOffset.Inverse() * currCluster->GetLink()->EvaluateGlobalTransform(currTime);
					currFrame.Transform = FbxUtil::ConvertToAffineMatrix(currFrame.GlobalTransform);

					animClip.BoneAnimations[currJointIndex].keyFrames.push_back(std::move(currFrame));
				}
			}
		}
	}


	//bone weight의 개수는 4가 아닐수 있고, weight의 합은 1이 아닐수있다
	BlendingIndexWeightPair currBlendIndexWeightPair;
	for (auto iter = controlPoints.begin(); iter != controlPoints.end(); ++iter)
	{
		for (size_t i = iter->BlendingInfo.size(); i < 4; ++i)
		{
			iter->BlendingInfo.push_back(currBlendIndexWeightPair);
		}
	}
}

size_t FbxModelScene::FindJointIndexUsingName(const std::string& jointName)
{
	for (size_t i = 0; i < m_skeleton.Joints.size(); ++i)
	{
		if (m_skeleton.Joints[i].Name == jointName)
		{
			return i;
		}
	}

	throw std::exception("Skeleton information in Fbx file is corrupted");
	return -1;
}

