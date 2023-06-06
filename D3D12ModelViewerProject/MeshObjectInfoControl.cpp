
#include "MeshObjectInfoControl.h"
#include <string>

using namespace std;
using namespace DirectX;

MeshObjectInfoControl::MeshObjectInfoControl()
{
}

void MeshObjectInfoControl::Show()
{
	if (ImGui::Begin(m_title.c_str()) == false)
	{
		ImGui::End();
		return;
	}

	auto meshObject = m_model.MeshObject.lock();

	if (meshObject == nullptr)
	{
		ImGui::End();
		return;
	}

	//Show Materials
	ImGui::Text(meshObject->GetName().c_str());
	ImGui::Separator();

	if (ImGui::CollapsingHeader("MeshObject Transform"))
	{
		//Show Transform
		const Transform& beforeTransform = meshObject->GetTransform();

		Transform transform = beforeTransform;
		XMFLOAT3& scale = transform.Scale;
		XMFLOAT3& rotation = transform.Rotation;
		XMFLOAT3& translation = transform.Translation;

		ImGui::SetNextItemWidth(200.0f);
		ImGui::DragFloat3("Scale(x,y,z)", (float*)(&scale));

		ImGui::SetNextItemWidth(200.0f);
		ImGui::DragFloat3("rotation(x,y,z)", (float*)(&rotation));

		ImGui::SetNextItemWidth(200.0f);
		ImGui::DragFloat3("translation(x,y,z)", (float*)(&translation));

		if (transform != beforeTransform)
		{
			meshObject->SetTransform(transform);
		}
	}

	if (ImGui::CollapsingHeader("Mesh Materials"))
	{
		int treeId = 0;

		for (auto& element : meshObject->GetMaterials())
		{
			//copy
			PBRMaterial material = element;
			if (ImGui::TreeNode(material.Name.c_str()))
			{
				ImGui::Text("material Name");
				ImGui::Text(material.Name.c_str());

				ImGui::Text("Albedo");
				ImGui::DragFloat4("Albedo", (float*)&material.Albedo, 0.01f, 0.0f, 1.0f);

				ImGui::Text("Roughness");
				ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);

				ImGui::Text("Metalic");
				ImGui::DragFloat("Metalic", &material.Metalic, 0.01f, 0.0f, 1.0f);

				ImGui::Text("AlbedoMap");
				ImGui::Text(WstringToUTF8(material.AlbedoMap).c_str());
				ImGui::TreePop();
			}
			//value changed
			if (material != element)
			{
				m_changedMaterial.push_back(material);
			}
		}
	}//EndCollapseHeader


	if (ImGui::CollapsingHeader("MeshInstances"))
	{
		if (ImGui::Button("Create Instance"))
		{
			meshObject->CreateMeshInstance();
		}
		ImGui::Separator();

		int treeId = 0;
		std::vector<size_t> deletedItems;

		for (auto& meshInstance : meshObject->GetMeshInstances())
		{
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			//항목추가 및 선택아이템 변경
			if (ImGui::TreeNodeEx((void*)(intptr_t)treeId, node_flags, meshInstance->GetName().c_str()))
			{
				string popupId = "meshInstancePopup";
				//MeshInstance Delete popup
				if (ImGui::IsItemClicked())
				{
					m_currentMeshInstance = meshInstance;
				}

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup(popupId.c_str());
				}
				if (ImGui::BeginPopup(popupId.c_str()))
				{
					if (ImGui::Selectable("Delete"))
					{
						deletedItems.push_back(treeId);
					}
					ImGui::EndPopup();
				}
				//Show Transform
				const Transform& beforeTransform = meshInstance->GetTransform();

				Transform transform = beforeTransform;
				XMFLOAT3& scale = transform.Scale;
				XMFLOAT3& rotation = transform.Rotation;
				XMFLOAT3& translation = transform.Translation;

				ImGui::SetNextItemWidth(200.0f);
				ImGui::DragFloat3("Scale(x,y,z)", (float*)(&scale));

				ImGui::SetNextItemWidth(200.0f);
				ImGui::DragFloat3("rotation(x,y,z)", (float*)(&rotation));

				ImGui::SetNextItemWidth(200.0f);
				ImGui::DragFloat3("translation(x,y,z)", (float*)(&translation));

				if (transform != beforeTransform)
				{
					meshInstance->SetTransform(transform);
				}
				//Show Animations 
				ImGui::Separator();

				bool isSelected = false;
				auto animations = m_model.Animations.lock();

				ImGui::Text("Animations");
				if (ImGui::BeginListBox("##Animations"))
				{
					if (animations != nullptr)
					{
						for (auto& animation : *animations)
						{
							isSelected = (m_selectedAnimation == animation.first);
							if (ImGui::Selectable(animation.first.c_str(), &isSelected))
							{
								m_selectedAnimation = animation.first;
							}
							if (isSelected == true)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
					}
					ImGui::EndListBox();
				}

				if (ImGui::Button("Play"))
				{
					if (animations != nullptr)
					{
						auto iter = animations->find(m_selectedAnimation);
						if (iter != animations->end())
						{
							meshInstance->SetAnimation(iter->second);
						}
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Stop"))
				{
					meshInstance->StopAnimation();
				}

				ImGui::SameLine();
				if (ImGui::Button("Pause"))
				{
					meshInstance->PauseAnimation();
				}

				ImGui::TreePop();
			}

			if (ImGui::IsItemClicked())
			{
				m_currentMeshInstance = meshInstance;
			}
			treeId++;
		}//end meshInstance for
		for (auto& item : deletedItems)
		{
			meshObject->DeleteMeshInstance(item);
		}
	}//end CollapseHeader
	ImGui::End();
}

void MeshObjectInfoControl::Update()
{
	auto meshObject = m_model.MeshObject.lock();
	if (meshObject != nullptr)
	{
		for (auto& material : m_changedMaterial)
		{
			meshObject->ChangeMeshMaterials(material.Name, material);
		}
	}

	m_changedMaterial.clear();
}



void MeshObjectInfoControl::Initialize(MeshObjectInfoModel viewModel)
{
	m_model = viewModel;
}
