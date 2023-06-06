#pragma once

#include "ImGUIControl.h"
#include "MeshResources.h"
#include "MeshObject.h"

struct MeshObjectInfoModel
{
	std::weak_ptr<MeshObject> MeshObject;
	std::weak_ptr<std::map<std::string, AnimationClip>> Animations;
};

class MeshObjectInfoControl
{
public:
	MeshObjectInfoControl();
public:
	void Show();
	void Update();
public:
	void Initialize(MeshObjectInfoModel viewModel);

private:
	std::string m_title = "MeshObject Infomation";

	MeshObjectInfoModel m_model;

	std::vector<PBRMaterial> m_changedMaterial;
	std::weak_ptr<MeshInstance> m_currentMeshInstance;

	std::string m_selectedAnimation;

};