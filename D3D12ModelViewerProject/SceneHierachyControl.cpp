
#include "SceneHierachyControl.h"
#include "MeshObject.h"

using namespace std;

void SceneHierachyControl::Update()
{
	m_meshObjectInfoControl.Update();
}

void SceneHierachyControl::Show()
{
	auto currentScene = m_model.scene.lock();
	if (currentScene == nullptr)
	{
		return;
	}

	{
		ImGui::Begin("Scene Hierachy");

		auto rootSceneObject = currentScene->GetRootSceneObject();
		ShowSceneHierachyRecursively(rootSceneObject);

		ImGui::End();
	}

	m_meshObjectInfoControl.Show();
}

void SceneHierachyControl::Initialize(SceneHierachyModel viewModel)
{
	m_model = viewModel;
	MeshObjectInfoControlInitialize();
}

void SceneHierachyControl::MeshObjectInfoControlInitialize()
{
	MeshObjectInfoModel meshObjectModel;
	meshObjectModel.Animations = m_model.Animations;

	if (auto meshObject = dynamic_pointer_cast<MeshObject>(m_selectedSceneObject.lock()))
	{
		meshObjectModel.MeshObject = meshObject;
	}
	m_meshObjectInfoControl.Initialize(meshObjectModel);
}

bool SceneHierachyControl::ShowSceneHierachyRecursively(std::shared_ptr<SceneObject> sceneObject, int id)
{
	if (sceneObject == nullptr)
	{
		return false;
	}
	string objectName = sceneObject->GetName();

	ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth |
		ImGuiTreeNodeFlags_DefaultOpen;

	if (sceneObject->GetChildCount() <= 0)
	{
		node_flags |= ImGuiTreeNodeFlags_Leaf;
	}

	//항목추가 및 선택아이템 변경 
	if (ImGui::TreeNodeEx((void*)(intptr_t)id, node_flags, objectName.c_str()))
	{
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			m_lastDragDropItem = sceneObject;
			ImGui::SetDragDropPayload("meshObject_DragDrop", &m_lastDragDropItem, sizeof(m_lastDragDropItem));
			ImGui::Text(objectName.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("meshObject_DragDrop"))
			{
				if (auto dragDropItem = m_lastDragDropItem.lock())
				{
					dragDropItem->ChangeParent(sceneObject);
					ImGui::EndDragDropTarget();
					ImGui::TreePop();
					return false;
				}
			}
			ImGui::EndDragDropTarget();			
		}

		string popupId = "meshObject Popup";

		if (ImGui::IsItemClicked())
		{
			m_selectedSceneObject = sceneObject;
			MeshObjectInfoControlInitialize();
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && id != -1)
		{
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str()))
		{
			if (ImGui::Selectable("Delete"))
			{
				ImGui::EndPopup();
				ImGui::TreePop();
				return true;
			}
			ImGui::EndPopup();
		}

		for (int i = 0; i < sceneObject->GetChildCount(); ++i)
		{
			auto child = sceneObject->GetChild(i);
			if (ShowSceneHierachyRecursively(child, i) == true)
			{
				sceneObject->RemoveChild(child);
				i--;
			}
		}

		ImGui::TreePop();
	}

	return false;
}
