#pragma once

#include "MeshObject.h"

class DynamicMesh : public MeshObject, public std::enable_shared_from_this<DynamicMesh>
{
private:
	friend class MeshObject;
	using InstanceAnimationBuffer = UploadBuffer<InstanceAnimations>;
private:
	DynamicMesh(std::string name, const MeshResources& meshResource);
	static std::shared_ptr<DynamicMesh> Create(std::string name, const MeshResources& meshResources);
public:
	virtual ~DynamicMesh() = default;
protected:
	virtual void ReAllocateDescriptorHeaps(int frameIndex) override;
	virtual void UpdateInstanceBuffers(int frameIndex) override;
	virtual void ReAllocateInstanceBuffer(int frameIndex) override;
	virtual std::shared_ptr<SceneObject> SharedFromThis() override { return std::enable_shared_from_this<DynamicMesh>::shared_from_this(); }
public:
	virtual void Update() override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, RenderType renderType);

private:
	std::vector<std::unique_ptr<InstanceAnimationBuffer>> m_instanceAnimatonBuffer;
};

