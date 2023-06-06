
#include "MeshObject.h"
#include "D3DResourceManager.h"
#include "DynamicMeshObject.h"
#include "StaticMeshObject.h"

using namespace std;
using namespace DirectX;

MeshObject::MeshObject(std::string name, const MeshResources& meshResources, int perInstanceDescriptorCount, int perMaterialDescriptorCount, MeshType meshType)
	: SceneObject(name),
	m_geometry(meshResources.Geometry),
	m_materials(meshResources.Materials),
	m_skeleton(meshResources.Skeleton),
	m_numberAllocator(100000),
	m_perInstanceDescriptorCount(perInstanceDescriptorCount),
	m_perMaterialDescriptorCount(perMaterialDescriptorCount),
	m_meshType(meshType)
{
}


MeshObject::~MeshObject()
{
	DeleteTextureResource(m_textureFileNames);
}

std::shared_ptr<MeshObject> MeshObject::CreateMeshObject(std::string name, const MeshResources& meshResources)
{
	std::shared_ptr<MeshObject> newObject;
	if (meshResources.Skeleton.Joints.empty())
	{
		newObject = StaticMesh::Create(name, meshResources);
	}
	else
	{
		newObject = DynamicMesh::Create(name, meshResources);
	}
	if (newObject != nullptr)
	{
		newObject->initialize();
	}

	return newObject;
}

void MeshObject::Update()
{
	SceneObject::Update();

	ID3D12Device* device = D3DResourceManager::GetInstance().GetDevice();
	int frameIndex = D3DResourceManager::GetInstance().GetCurrentFrameIndex();

	if (m_instanceBufferDirty > 0)
	{
		ReAllocateInstanceBuffer(frameIndex);
	}

	if (m_HeapAllocationDirty > 0)
	{
		ReAllocateDescriptorHeaps(frameIndex);
	}

	for (auto& meshInstance : m_meshInstances)
	{
		meshInstance->Update();
	}

	UpdateObjectBuffer(frameIndex);
	UpdateInstanceBuffers(frameIndex);
	UpdateMaterialBuffer(frameIndex);
}

void MeshObject::SetEnable(bool enable)
{
	m_enable = enable;
}

std::string MeshObject::CreateMeshInstance(const Transform& transform)
{
	size_t CurrInstanceCount = m_meshInstances.size();

	if (CurrInstanceCount >= m_maxInstanceCount)
	{
		SetMaxMeshInstanceCount(m_maxInstanceCount + 50);
	}

	auto meshObject = dynamic_pointer_cast<MeshObject>(SharedFromThis());
	if (meshObject == nullptr)
	{
		return string();
	}

	string instanceName = "instance_" + to_string(m_numberAllocator.Allocate());

	std::shared_ptr<MeshInstance> instance = make_shared<MeshInstance>(instanceName, meshObject, transform);

	m_meshInstances.emplace_back(std::move(instance));
	m_InstanceUpdateDirty = FramesCount;
	return instanceName;
}

void MeshObject::DeleteMeshInstance(size_t instanceIndex)
{
	if (instanceIndex >= m_meshInstances.size())
	{
		return;
	}
	m_meshInstances.erase(m_meshInstances.begin() + instanceIndex);
	m_InstanceUpdateDirty = FramesCount;
}

void MeshObject::SetMaxMeshInstanceCount(uint32_t newCount)
{
	if (newCount != m_maxInstanceCount)
	{
		m_maxInstanceCount = newCount;
		m_instanceBufferDirty = FramesCount;
		m_HeapAllocationDirty = FramesCount;
	}
}

void MeshObject::ChangeMeshMaterials(std::string materialName, const PBRMaterial& meshMaterials)
{
	for (int i = 0; i < m_materials.size(); ++i)
	{
		PBRMaterial& material = m_materials.at(i);
		if (material.Name == materialName)
		{
			material = meshMaterials;
			material.NumFrameDirty = FramesCount;
			return;
		}
	}
}

void MeshObject::UpdateInstanceBuffers(int frameIndex)
{
	int i = 0;
	for (auto& meshInstance : m_meshInstances)
	{
		if (meshInstance->GetInstanceConstDirty() > 0 || m_InstanceUpdateDirty > 0)
		{
			m_instanceCBs.at(frameIndex)->CopyData(i, meshInstance->GetInstanceConstants());
		}

		i++;
	}

	if (m_InstanceUpdateDirty > 0)
	{
		m_InstanceUpdateDirty--;
	}
}

void MeshObject::UpdateMaterialBuffer(int frameIndex)
{
	for (int i = 0; i < m_materials.size(); ++i)
	{
		PBRMaterial& material = m_materials.at(i);
		if (material.NumFrameDirty > 0)
		{
			PBRMaterialConstants matConstants = material.ToMaterialConstants();

			m_materialCBs.at(frameIndex).get()->CopyData(i, matConstants);
			material.NumFrameDirty--;
		}
	}
}

void MeshObject::UpdateObjectBuffer(int frameIndex)
{
	ObjectConstants objConstant;
	XMStoreFloat4x4(&objConstant.ObjectTransform, XMMatrixTranspose(m_finalTransform));

	m_objectCB.at(frameIndex)->CopyData(0, objConstant);
}

void MeshObject::ReAllocateInstanceBuffer(int frameIndex)
{
	m_instanceCBs.at(frameIndex) = D3DResourceManager::GetInstance().CreateUploadBuffer<InstanceConstants>(m_maxInstanceCount, false);

	m_instanceBufferDirty--;
}

void MeshObject::ReAllocateObjectBuffer(int frameIndex)
{
	m_objectCB.at(frameIndex) = D3DResourceManager::GetInstance().CreateUploadBuffer<ObjectConstants>(1, true);
}

void MeshObject::ReAllocateDescriptorHeaps(int frameIndex)
{
	D3DResourceManager::GetInstance().CopyDescriptorHeapAllocation(m_cpuDescriptorHeapAllocations.at(frameIndex), m_gpuDescriptorHeapAllocations.at(frameIndex));
}

void MeshObject::ReAllocateMaterialBuffer(int frameIndex)
{
	m_materialCBs.at(frameIndex) = D3DResourceManager::GetInstance().CreateUploadBuffer<PBRMaterialConstants>(m_materials.size(), true);
}

void MeshObject::DeleteTextureResource(std::vector<std::wstring> textureFileNames)
{
	auto& resourceManager = D3DResourceManager::GetInstance();

	for (auto& textureName : textureFileNames)
	{
		resourceManager.DeleteTextureResource(textureName);
	}
}

void MeshObject::initialize()
{
	ID3D12Device* device = D3DResourceManager::GetInstance().GetDevice();

	m_cpuDescriptorHeapAllocations.resize(FramesCount);
	m_gpuDescriptorHeapAllocations.resize(FramesCount);
	m_instanceCBs.resize(FramesCount);
	m_materialCBs.resize(FramesCount);
	m_objectCB.resize(FramesCount);

	//buildRenderItems
	for (int i = 0; i < m_materials.size(); ++i)
	{
		PBRMaterial& meshMaterial = m_materials.at(i);
		RenderItem renderItem;

		renderItem.MaterialCBIndex = i;
		renderItem.Geo = m_geometry.get();
		renderItem.PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem.IndexCount = renderItem.Geo->DrawArgs[meshMaterial.Name].IndexCount;
		renderItem.BaseVertexLocation = renderItem.Geo->DrawArgs[meshMaterial.Name].BaseVertexLocation;
		renderItem.StartIndexLocation = renderItem.Geo->DrawArgs[meshMaterial.Name].StartIndexLocation;

		m_renderItems.push_back(renderItem);
	}

	//build frameResource
	for (int i = 0; i < FramesCount; ++i)
	{
		ReAllocateInstanceBuffer(i);
		ReAllocateMaterialBuffer(i);
		ReAllocateObjectBuffer(i);
	}

	for (int i = 0; i < FramesCount; ++i)
	{
		ReAllocateDescriptorHeaps(i);
	}

}
