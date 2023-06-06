#pragma once

#include "MeshResources.h"
#include "MeshInstance.h"

class MeshObject : public SceneObject
{
protected:
	using InstanceConstantsBuffer = UploadBuffer<InstanceConstants>;
	using MaterialConstantsBuffer = UploadBuffer<PBRMaterialConstants>;
	using ObjectConstantsBuffer = UploadBuffer<ObjectConstants>;
public:
	virtual ~MeshObject();

	static std::shared_ptr<MeshObject> CreateMeshObject(std::string name, const MeshResources& meshResources);

protected:
	MeshObject(
		std::string name,
		const MeshResources& meshResources,
		int _perInstanceDescriptorCount,
		int _perMaterialDescriptorCount,
		MeshType meshType);
protected:
	virtual void UpdateInstanceBuffers(int frameIndex);
	virtual void ReAllocateInstanceBuffer(int frameIndex);
	virtual void ReAllocateObjectBuffer(int frameIndex);
	virtual void ReAllocateDescriptorHeaps(int frameIndex);
public:
	virtual void Update() override;
public:

	void SetEnable(bool enable);
	std::string CreateMeshInstance(const Transform& transform = Transform());
	void DeleteMeshInstance(size_t instanceIndex);
	void ChangeMeshMaterials(std::string materialName, const PBRMaterial& meshMaterials);
	const std::vector<PBRMaterial>& GetMaterials() const { return m_materials; }
	const std::vector<std::shared_ptr<MeshInstance>>& GetMeshInstances() const { return m_meshInstances; }
	MeshType GetMeshType() const { return m_meshType; }
	const Skeleton& GetSkeleton() { return m_skeleton; }
protected:
	size_t GetDescriptorsCount() { return  m_perInstanceDescriptorCount + m_perMaterialDescriptorCount * m_materials.size(); }
	void UpdateMaterialBuffer(int frameIndex);
	void UpdateObjectBuffer(int frameIndex);
	void ReAllocateMaterialBuffer(int frameIndex);
	void SetMaxMeshInstanceCount(uint32_t newCount);
	void DeleteTextureResource(std::vector<std::wstring> textureFileNames);
private:
	//create함수에서만 사용 
	void initialize();
protected:
	bool m_enable = true;
	// needFix
	int m_perInstanceDescriptorCount = 0;
	int m_perMaterialDescriptorCount = 0;

	std::shared_ptr<MeshGeometry> m_geometry;
	std::vector<RenderItem> m_renderItems;

	//for MaterialCBIndex
	std::vector<PBRMaterial> m_materials;
	std::vector<std::unique_ptr<MaterialConstantsBuffer>> m_materialCBs;

	std::vector<std::wstring> m_textureFileNames;

	std::vector<DescriptorHeapAllocation> m_cpuDescriptorHeapAllocations;
	std::vector<DescriptorHeapAllocation> m_gpuDescriptorHeapAllocations;
	int m_HeapAllocationDirty = FramesCount;

	std::vector<std::unique_ptr<InstanceConstantsBuffer>> m_instanceCBs;
	int m_InstanceUpdateDirty = FramesCount; // meshInstance 정보를 update해야할때
	int m_instanceBufferDirty = FramesCount; // m_maxInstanceCount 가 달라졌을때

	std::vector<std::unique_ptr<ObjectConstantsBuffer>> m_objectCB;

	std::vector<std::shared_ptr<MeshInstance>> m_meshInstances;

	uint32_t m_maxInstanceCount = 5;
	NumberAllocator m_numberAllocator;

	Skeleton m_skeleton;

	MeshType m_meshType = MeshType::StaticMesh;
};