#pragma once

#include "ImGUIControl.h"
#include "SceneObject.h"
#include "MeshObjectInfoControl.h" 

struct SceneHierachyModel
{
	std::weak_ptr<Scene> scene;
	std::weak_ptr<std::map<std::string, AnimationClip>> Animations;
};

class SceneHierachyControl
{
public:
	void Update();
	void Show();
public:
	void Initialize(SceneHierachyModel viewModel);
private:
	void MeshObjectInfoControlInitialize();
	//rootScene¿∫ id = -1
	bool ShowSceneHierachyRecursively(std::shared_ptr<SceneObject> sceneObject, int id = -1);

private:
	SceneHierachyModel m_model;

	MeshObjectInfoControl m_meshObjectInfoControl;
	std::weak_ptr<SceneObject> m_selectedSceneObject;

	std::weak_ptr<SceneObject> m_lastDragDropItem;
};
