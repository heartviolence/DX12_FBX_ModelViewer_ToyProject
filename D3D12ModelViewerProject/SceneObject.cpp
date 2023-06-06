
#include "SceneObject.h"
#include "D3DResourceManager.h"

using namespace std;
using namespace DirectX;

SceneObject::SceneObject(std::string name)
	:m_name(name)
{

}

void SceneObject::Update()
{
	UpdateFinalTransform();
	UpdateChild();
}


void SceneObject::UpdateFinalTransform()
{
	if (auto parent = m_parent.lock())
	{
		XMMATRIX parentMatrix = parent->GetFinalTransform();
		m_finalTransform = XMMatrixMultiply(m_transform.GetFinalTransformMatrix(), parentMatrix);
	}
	else
	{
		m_finalTransform = m_transform.GetFinalTransformMatrix();
	}
}

void SceneObject::SetParent(std::shared_ptr<SceneObject> parent)
{
	m_parent = parent;
	XMMATRIX relativeMatrix = XMMatrixMultiply(m_finalTransform, XMMatrixInverse(nullptr, parent->GetFinalTransform()));

	m_transform.Initialize(relativeMatrix);
}

void SceneObject::ChangeParent(std::shared_ptr<SceneObject> parent)
{
	std::shared_ptr<SceneObject> sharedThis = SharedFromThis();

	auto grandParent = parent->GetParent();
	while (grandParent != nullptr)
	{
		if (grandParent == sharedThis)
		{
			return;
		}
		grandParent = grandParent->GetParent();
	}

	if (auto beforeParent = m_parent.lock())
	{
		beforeParent->RemoveChild(SharedFromThis());
	}

	parent->AddChild(SharedFromThis());
}

void SceneObject::AddChild(std::shared_ptr<SceneObject> sceneObject)
{
	m_childSceneObjects.push_back(sceneObject);
	sceneObject->SetParent(SharedFromThis());
}

void SceneObject::RemoveChild(std::shared_ptr<SceneObject> sceneObject)
{
	auto iter = std::find(m_childSceneObjects.begin(), m_childSceneObjects.end(), sceneObject);
	if (iter != m_childSceneObjects.end())
	{
		m_childSceneObjects.erase(iter);
	}
}

Scene::Scene(std::string name)
	: m_name(name),
	m_rootSceneObject(static_pointer_cast<SceneObject>(RootSceneObject::Create()))
{
}

void Scene::PrepareRender()
{
	int childCount = m_rootSceneObject->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		PushRenderListRecursively(m_rootSceneObject->GetChild(i));
	}
}

void Scene::PushRenderListRecursively(const std::shared_ptr<SceneObject> sceneObject) const
{
	int childCount = sceneObject->GetChildCount();

	D3DResourceManager::GetInstance().PushRenderItem(sceneObject);

	for (int i = 0; i < childCount; ++i)
	{
		auto child = sceneObject->GetChild(i);
		PushRenderListRecursively(child);
	}
}

std::shared_ptr<RootSceneObject> RootSceneObject::Create(std::string name)
{
	return std::shared_ptr<RootSceneObject>(new RootSceneObject(name));
}
