#pragma once

#include "ImGUIControl.h"
#include "FbxModelScene.h"
#include "MeshResources.h"
#include "FbxUtil.h" 
#include "JobQueue.h"


struct ImportListModel
{
	std::weak_ptr<Scene> CurrentScene;
	std::weak_ptr<std::map<std::string, std::shared_ptr<FbxModelScene>>> FbxModels;
	std::weak_ptr<std::map<std::string, std::shared_ptr<MeshResources>>> MeshResources;
	std::weak_ptr<std::map<std::string, AnimationClip>> Animations;
};

class ImportListControl
{
public:
	ImportListControl();

private:
	std::string RegisterMeshResources(const std::string key, FbxModelScene* fbxmodel);
	void RegisterMeshObject(const std::string name, MeshResources* meshResources);

	std::string RegisterAnimationClip(const std::string key, AnimationClip& animationClip);

	//FBX�ε� FbxModels�� �߰� MeshResource ����
	void fbxImport(std::wstring filePath);
	//�񵿱� FBX�ε� FbxModels�� �߰� MeshResource ����
	void fbxImportAsync(std::wstring filePath);

	void SetImportingState(bool state);
	bool IsImporting();

public:
	void Show();
	void Update();
	void Initialize(ImportListModel viewModel);
private:
	ImportListModel m_model;

	float m_lastImportTime = 0.0f;

	std::mutex m_isImportingMutex;
	bool m_isImporting = false;

	JobQueue m_updateQueue;
};