#pragma once

#include <Windows.h>
#include <fbxsdk.h>
#include <string>
#include <stdint.h>
#include <assert.h>
#include "FbxUtil.h"
#include <map>
#include "MeshResources.h"


namespace std
{
	//해쉬맵용 해쉬함수
	template<>
	struct hash<SubMeshVertex>
	{
		std::size_t operator()(const SubMeshVertex& vertex) const
		{
			return (hash<float>()(vertex.Position.x) ^
				hash<float>()(vertex.Position.y) ^
				hash<float>()(vertex.Position.z) ^
				hash<string>()(vertex.MaterialName) ^
				hash<float>()(vertex.TexC.x) ^
				hash<float>()(vertex.TexC.y));
		}
	};
}

// FBX는 내부적으로 UTF8을 사용함
class FbxModelScene
{
public:
	FbxModelScene(std::wstring filename);
	~FbxModelScene();

	std::string GetName() { return m_name; }
	std::shared_ptr<MeshResources> CreateMeshResource();
	std::unordered_map<std::string, AnimationClip>& GetAnimationClips() { return m_animations; }
private:
	bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
	void ProcessNode(FbxNode* pNode);
	void ProcessScene(FbxScene* pScene);
	void LoadMesh(FbxNode* pNode);
	void RenameDuplicatedMaterial(FbxScene* pScene);
	void ProcessMaterialTable(FbxScene* pScene);
	void ProcessPolygons(FbxMesh* pMesh);
	void ProcessSkeletonHierachy(FbxNode* pRootNode);
	void ProcessSkeletonHierachyRecursively(FbxNode* pNode, int depth, int index, int parentIndex);
	void ProcessJointsAndAnimations(FbxNode* pNode, std::vector<ControlPoint>& controlPoints);
	size_t FindJointIndexUsingName(const std::string& jointName);
	FbxSurfaceMaterial* FindFbxMaterial(std::string UTF8MaterialName);


	void Triangulate(TempPolygon& polygon, std::vector<TempPolygon>& Output);

private:
	FbxScene* m_scene;
	FbxManager* m_manager;

	std::wstring m_directory;

	//fbxfiletexture -> uniqueName(utf8)
	std::map<FbxFileTexture*, std::string> m_allFbxFileTexture;

	std::vector<FbxSurfaceMaterial*> m_fbxMaterials;

	std::vector<SubMeshVertex> m_vertexTable;
	std::map<std::string, IndexTableType> m_subMeshes;

	std::string m_name;

	FbxAnimEvaluator* m_sceneEvaluator;

	Skeleton m_skeleton;

	std::unordered_map<std::string, AnimationClip> m_animations;
};
