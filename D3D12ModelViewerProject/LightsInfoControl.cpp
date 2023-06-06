
#include "LightsInfoControl.h"

using namespace std;

void LightsInfoControl::Show()
{
	if (ImGui::Begin("lights") == false)
	{
		ImGui::End();
		return;
	}

	auto lights = m_model.Lights.lock();
	if (lights == nullptr)
	{
		ImGui::End();
		return;
	}

	//ShowDirectionalLights
	if (ImGui::CollapsingHeader("Directional Lights"))
	{
		if (ImGui::Button("Create Directional Lights"))
		{
			lights->DirectionalLights.push_back(DirectionalLight());
		}
		ImGui::Separator();

		int treeIndex = 0;
		std::vector<size_t> deletedItems;

		for (auto& light : lights->DirectionalLights)
		{
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			string popupId = "directionalLightsPopup" + to_string(treeIndex);

			//항목추가 및 선택아이템 변경
			if (ImGui::TreeNodeEx((void*)(intptr_t)treeIndex, node_flags, light.name.c_str()))
			{
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup(popupId.c_str());
				}
				if (ImGui::BeginPopup(popupId.c_str()))
				{
					if (ImGui::Selectable("Delete"))
					{
						deletedItems.push_back(treeIndex);
					}
					ImGui::EndPopup();
				}

				if (ImGui::DragFloat("Power", &light.Power) == true)
				{
					if (light.Power < 0)
					{
						light.Power = 0;
					}
				}
				ImGui::DragFloat3("Color", (float*)(&light.Color), 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat3("Direction", (float*)(&light.Direction), 0.01f, -1.0f, 1.0f);
				ImGui::TreePop();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup(popupId.c_str());
			}

			if (ImGui::BeginPopup(popupId.c_str()))
			{
				if (ImGui::Selectable("Delete"))
				{
					deletedItems.push_back(treeIndex);
				}
				ImGui::EndPopup();
			}

			treeIndex++;
		}//end meshInstance for

		for (auto& itemindex : deletedItems)
		{
			lights->DirectionalLights.erase(lights->DirectionalLights.begin() + itemindex);
		}
	}

	// Show pointLights
	if (ImGui::CollapsingHeader("point Lights"))
	{
		if (ImGui::Button("Create PointLights"))
		{
			lights->PointLights.push_back(PointLight());
		}
		ImGui::Separator();

		int treeIndex = 0;
		int controlId = 100000;
		std::vector<size_t> deletedItems;

		for (auto& light : lights->PointLights)
		{
			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			string popupId = "PointLightsPopup" + to_string(treeIndex);

			//항목추가 및 선택아이템 변경
			if (ImGui::TreeNodeEx((void*)(intptr_t)controlId++, node_flags, light.name.c_str()))
			{
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup(popupId.c_str());
				}
				if (ImGui::BeginPopup(popupId.c_str()))
				{
					if (ImGui::Selectable("Delete"))
					{
						deletedItems.push_back(treeIndex);
					}
					ImGui::EndPopup();
				}

				if (ImGui::DragFloat("Power", &light.Power) == true)
				{
					if (light.Power < 0)
					{
						light.Power = 0;
					}
				}
				ImGui::DragFloat3("Color", (float*)(&light.Color), 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat3("Position", (float*)(&light.Position));
				ImGui::TreePop();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup(popupId.c_str());
			}

			if (ImGui::BeginPopup(popupId.c_str()))
			{
				if (ImGui::Selectable("Delete"))
				{
					deletedItems.push_back(treeIndex);
				}
				ImGui::EndPopup();
			}

			treeIndex++;
		}//end meshInstance for

		for (auto& itemindex : deletedItems)
		{
			lights->PointLights.erase(lights->PointLights.begin() + itemindex);
		}
	}

	ImGui::End();
}
