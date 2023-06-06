#pragma once

#include <unordered_map>
#include <memory>
#include "D3DUtil.h"
#include "RenderTypes.h"

class SceneObject
{
protected:
	SceneObject(std::string name = "");
public:
	virtual void Render(class ID3D12GraphicsCommandList* cmdList, RenderType renderType) = 0;
protected:
	virtual std::shared_ptr<SceneObject> SharedFromThis() = 0;
public:
	virtual void Update();
	virtual std::shared_ptr<SceneObject> GetChild(int index) { return m_childSceneObjects.at(index); }
	void SetTransform(const Transform& transform) { m_transform = transform; }
	const Transform& GetTransform() { return m_transform; }
	const DirectX::XMMATRIX& GetFinalTransform() { return m_finalTransform; }

	int GetChildCount() { return m_childSceneObjects.size(); }
	std::string GetName() { return m_name; }

	void SetParent(std::shared_ptr<SceneObject> parent);
	void ChangeParent(std::shared_ptr<SceneObject> parent);
	std::shared_ptr<SceneObject> GetParent() { return m_parent.lock(); }

	void AddChild(std::shared_ptr<SceneObject> sceneObject);
	void RemoveChild(std::shared_ptr<SceneObject> sceneObject);

protected:
	void UpdateFinalTransform();
	void UpdateChild()
	{
		for (auto& child : m_childSceneObjects)
		{
			child->Update();
		}
	}
protected:
	std::weak_ptr<SceneObject> m_parent;
	std::vector<std::shared_ptr<SceneObject>> m_childSceneObjects;
	std::string m_name;
	Transform m_transform;
	DirectX::XMMATRIX m_finalTransform = DirectX::XMMatrixIdentity();
};


class RootSceneObject : public SceneObject, public std::enable_shared_from_this<RootSceneObject>
{
public:
	static std::shared_ptr<RootSceneObject> Create(std::string name = "rootSceneObject");

private:
	RootSceneObject(std::string name) :
		SceneObject(name) {}
public:
	virtual void Render(class ID3D12GraphicsCommandList* cmdList, RenderType renderType) override {}
	virtual std::shared_ptr<SceneObject> SharedFromThis() override { return std::dynamic_pointer_cast<SceneObject>(shared_from_this()); }
};



class Scene
{
public:
	Scene(std::string name = "");
public:
	std::shared_ptr<SceneObject> GetRootSceneObject() { return m_rootSceneObject; }
	void Update() { m_rootSceneObject->Update(); }
	void PrepareRender();

	std::string GetName() { return m_name; }
private:
	void PushRenderListRecursively(const std::shared_ptr<SceneObject> sceneObject) const;
private:
	std::shared_ptr<SceneObject> m_rootSceneObject;
	std::string m_name;
};



