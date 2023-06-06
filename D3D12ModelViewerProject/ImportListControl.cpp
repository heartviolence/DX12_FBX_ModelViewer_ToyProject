
#include "ImportListControl.h"
#include "ThreadManager.h"
#include "GameTimer.h"
#include "FileDialog.h"
#include "MeshObject.h"

using namespace std;

ImportListControl::ImportListControl()
{

}


std::string ImportListControl::RegisterMeshResources(const std::string key, FbxModelScene* fbxmodel)
{
	if (fbxmodel == nullptr)
	{
		return string();
	}

	auto meshResource = m_model.MeshResources.lock();
	if (meshResource == nullptr)
	{
		return string();
	}

	int nameIndex = 1;
	string baseName(key + "_v_");
	string uniqueName(key);

	while (meshResource->find(uniqueName) != meshResource->end())
	{
		uniqueName = baseName + to_string(nameIndex++);
	}

	meshResource->emplace(uniqueName, fbxmodel->CreateMeshResource());

	return uniqueName;
}

void ImportListControl::RegisterMeshObject(const std::string name, MeshResources* meshResources)
{
	if (meshResources == nullptr)
	{
		return;
	}

	shared_ptr<MeshObject> meshObject = meshResources->CreateMeshObject(name);
	meshObject->CreateMeshInstance();

	if (auto currentScene = m_model.CurrentScene.lock())
	{
		auto rootSceneObject = currentScene->GetRootSceneObject();
		if (rootSceneObject != nullptr)
		{
			rootSceneObject->AddChild(meshObject);
		}
	}
}


std::string ImportListControl::RegisterAnimationClip(const std::string key, AnimationClip& animationClip)
{
	auto animations = m_model.Animations.lock();
	if (animations == nullptr)
	{
		return string();
	}

	int nameIndex = 1;
	string baseName(key + "_v_");
	string uniqueName(key);

	while (animations->find(uniqueName) != animations->end())
	{
		uniqueName = baseName + to_string(nameIndex++);
	}

	animations->emplace(uniqueName, animationClip);
	return uniqueName;
}


void ImportListControl::fbxImport(std::wstring filePath)
{
	if (IsFbx(filePath) == false)
	{
		return;
	}
	SetImportingState(true);

	auto start = std::chrono::system_clock::now();
	auto fbxModel = make_shared<FbxModelScene>(filePath);
	auto end = std::chrono::system_clock::now();

	auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(end - start);

	std::function func([=]()
		{
			auto fbxModels = m_model.FbxModels.lock();
			if (fbxModels == nullptr)
			{
				return;
			}
			string modelName = fbxModel->GetName();
			fbxModels->emplace(modelName, fbxModel);

			RegisterMeshResources(modelName, fbxModel.get());

			unordered_map<string, AnimationClip>& animClips = fbxModel->GetAnimationClips();
			for (auto& element : animClips)
			{
				RegisterAnimationClip(element.first, element.second);
			}

			m_lastImportTime = delta.count();
		});

	m_updateQueue.AddJobQueue(func);
	SetImportingState(false);
}

void ImportListControl::fbxImportAsync(std::wstring filePath)
{
	if (IsFbx(filePath) == false)
	{
		return;
	}

	std::function asyncImportFunc([=]()
		{
			fbxImport(filePath);
		});
	ThreadManager::GetInstance().EnqueueJob(asyncImportFunc);
}


void ImportListControl::SetImportingState(bool state)
{
	std::lock_guard<mutex> lock(m_isImportingMutex);
	m_isImporting = state;
}

bool ImportListControl::IsImporting()
{
	std::lock_guard<mutex> lock(m_isImportingMutex);
	return m_isImporting;
}

void ImportListControl::Show()
{
	{
		ImGui::Begin("Model Informations");

		if (IsImporting() == false)
		{
			if (ImGui::Button("Open File Win"))
			{
				FileDialogControl dialog;
				dialog.Show();
				fbxImportAsync(dialog.GetSelectedItem());
			}
			ImGui::SameLine();
			ImGui::Text("Import Time :");
			ImGui::SameLine();
			ImGui::Text(to_string(m_lastImportTime).c_str());
			ImGui::Separator();
		}
		else
		{
			ImGui::Text("Importing...");
		}

		ImGuiTreeNodeFlags Base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

		if (ImGui::TreeNode("MeshResources"))
		{
			if (auto meshResources = m_model.MeshResources.lock())
			{

				int treeId = 0;

				auto iter = meshResources->begin();
				while (iter != meshResources->end())
				{
					string popupId = "ImportList_MeshResources_Popup" + to_string(treeId);
					auto nextIter = iter;
					nextIter++;

					string meshResourceName = iter->first;
					shared_ptr<MeshResources> meshResource = iter->second;

					ImGuiTreeNodeFlags node_flags =
						Base_flags |
						ImGuiTreeNodeFlags_Leaf |
						ImGuiTreeNodeFlags_NoTreePushOnOpen;

					//항목추가
					ImGui::TreeNodeEx((void*)(intptr_t)treeId++, node_flags, meshResourceName.c_str());
					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
					{
						ImGui::OpenPopup(popupId.c_str());
					}

					if (ImGui::BeginPopup(popupId.c_str()))
					{
						if (ImGui::Selectable("CreateMeshObject"))
						{
							RegisterMeshObject(meshResourceName, meshResource.get());
						}
						if (ImGui::Selectable("DeleteMeshResource"))
						{
							nextIter = meshResources->erase(iter);
						}
						ImGui::EndPopup();
					}

					iter = nextIter;
				}

			}
			ImGui::TreePop();
		}//MeshResources Tree End

		if (ImGui::TreeNode("Animations"))
		{
			if (auto animations = m_model.Animations.lock())
			{
				int treeId = 0;
				auto iter = animations->begin();
				while (iter != animations->end())
				{
					string popupId = "ImportList Animations Popup" + to_string(treeId);
					auto nextIter = iter;
					nextIter++;

					string animationName = iter->first;

					ImGuiTreeNodeFlags node_flags = Base_flags |
						ImGuiTreeNodeFlags_Leaf |
						ImGuiTreeNodeFlags_NoTreePushOnOpen;

					//항목추가
					ImGui::TreeNodeEx((void*)(intptr_t)treeId++, node_flags, animationName.c_str());
					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
					{
						ImGui::OpenPopup(popupId.c_str());
					}
					if (ImGui::BeginPopup(popupId.c_str()))
					{
						if (ImGui::Selectable("DeleteAnimations"))
						{
							nextIter = animations->erase(iter);
						}
						ImGui::EndPopup();
					}
					iter = nextIter;
				}
			}
			ImGui::TreePop();
		}//AnimationsTree End
		ImGui::End();
	}//Imgui window End

}

void ImportListControl::Update()
{
	m_updateQueue.ProcessJobQueue();
}

void ImportListControl::Initialize(ImportListModel viewModel)
{
	m_model = viewModel;
}
