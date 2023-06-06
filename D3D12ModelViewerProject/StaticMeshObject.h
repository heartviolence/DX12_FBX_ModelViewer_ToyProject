#pragma once

#include "MeshObject.h"


class StaticMesh : public MeshObject, public std::enable_shared_from_this<StaticMesh>
{
private:
	friend class MeshObject;
public:
	virtual ~StaticMesh() = default;
public:
	virtual void Update() override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, RenderType renderType);
private:
	StaticMesh(std::string name, const MeshResources& meshResources);
	static std::shared_ptr<StaticMesh> Create(std::string name, const MeshResources& meshResources);
private:
	virtual void ReAllocateDescriptorHeaps(int frameIndex) override;
	virtual void UpdateInstanceBuffers(ID3D12Device* device, int frameIndex);
	virtual void ReAllocateInstanceBuffer(ID3D12Device* device, int frameIndex);
	virtual std::shared_ptr<SceneObject> SharedFromThis() override { return std::enable_shared_from_this<StaticMesh>::shared_from_this(); };	 
};

 