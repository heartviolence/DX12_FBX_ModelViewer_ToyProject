
#include "StaticMeshObject.h"
#include "D3DResourceManager.h"
#include "FileUtil.h"

using namespace std;
using namespace DirectX;

StaticMesh::StaticMesh(std::string name, const MeshResources& meshResources)
	: MeshObject(name, meshResources, 2, 2, MeshType::StaticMesh)
{

}

std::shared_ptr<StaticMesh> StaticMesh::Create(std::string name, const MeshResources& meshResources)
{
	return std::shared_ptr<StaticMesh>(new StaticMesh(name, meshResources));
}

void StaticMesh::Update()
{
	MeshObject::Update();
}

void StaticMesh::Render(ID3D12GraphicsCommandList* cmdList, RenderType renderType)
{
	int frameIndex = D3DResourceManager::GetInstance().GetCurrentFrameIndex();

	if (m_enable == false)
	{
		return;
	}

	std::vector<RenderItem> rItems;

	for (int i = 0; i < m_materials.size(); ++i)
	{
		if (GetRenderType(m_materials.at(i), m_meshType) == renderType)
		{
			rItems.push_back(m_renderItems.at(i));
		}
	}

	if (rItems.empty())
	{
		return;
	}
	DescriptorHeapAllocation& gpuAlloc = m_gpuDescriptorHeapAllocations.at(frameIndex);

	size_t Offset = 0;
	cmdList->SetGraphicsRootDescriptorTable(1, gpuAlloc.GetGpuHandle());
	Offset += m_perInstanceDescriptorCount;

	for (size_t i = 0; i < rItems.size(); ++i)
	{
		RenderItem renderItem = rItems[i];

		auto vertexBufferView = renderItem.Geo->VertexBufferView();
		auto indexBufferView = renderItem.Geo->IndexBufferView();

		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);
		cmdList->IASetPrimitiveTopology(renderItem.PrimitiveType);

		D3D12_GPU_DESCRIPTOR_HANDLE matHandle = gpuAlloc.GetGpuHandle(Offset + renderItem.MaterialCBIndex * m_perMaterialDescriptorCount);
		cmdList->SetGraphicsRootDescriptorTable(2, matHandle);

		cmdList->DrawIndexedInstanced(
			renderItem.IndexCount,
			m_meshInstances.size(),
			renderItem.StartIndexLocation,
			renderItem.BaseVertexLocation,
			0
		);
	}
}

void StaticMesh::ReAllocateDescriptorHeaps(int frameIndex)
{
	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();

	std::vector<wstring> oldTexture = m_textureFileNames;
	m_textureFileNames.clear();

	m_cpuDescriptorHeapAllocations.at(frameIndex) =
		resourceManager.CpuDescriptorHeapAlloc(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			GetDescriptorsCount());

	m_gpuDescriptorHeapAllocations.at(frameIndex) =
		resourceManager.GpuDynamicDescriptorHeapAlloc(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			GetDescriptorsCount());

	UINT materialCBElementSize = m_materialCBs.at(frameIndex)->GetElementByteSize();

	auto& heapAllocation = m_cpuDescriptorHeapAllocations.at(frameIndex);

	D3D12_GPU_VIRTUAL_ADDRESS MatCBAddress = m_materialCBs.at(frameIndex)->Resource()->GetGPUVirtualAddress();
	auto& objBuffer = m_objectCB.at(frameIndex);

	auto& instanceDataBuffer = m_instanceCBs.at(frameIndex);

	int Offset = 0;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
	cbDesc.BufferLocation = objBuffer->Resource()->GetGPUVirtualAddress();
	cbDesc.SizeInBytes = objBuffer->GetElementByteSize();

	resourceManager.CreateCBV(heapAllocation.GetCpuHandle(Offset++), cbDesc);

	D3D12_SHADER_RESOURCE_VIEW_DESC instanceDataDesc = {};
	instanceDataDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instanceDataDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instanceDataDesc.Format = DXGI_FORMAT_UNKNOWN;
	instanceDataDesc.Buffer.FirstElement = 0;
	instanceDataDesc.Buffer.NumElements = m_maxInstanceCount;
	instanceDataDesc.Buffer.StructureByteStride = instanceDataBuffer->GetElementByteSize();

	resourceManager.CreateSRV(instanceDataBuffer->Resource(), heapAllocation.GetCpuHandle(Offset++), instanceDataDesc);


	//Mat CBV,diffuse SRV
	for (int materialIndex = 0; materialIndex < m_materials.size(); ++materialIndex)
	{
		//get TexResource
		wstring albedoMapFileName = m_materials.at(materialIndex).AlbedoMap;
		m_textureFileNames.push_back(albedoMapFileName);

		resourceManager.CreateTextureResource(albedoMapFileName, albedoMapFileName);

		ID3D12Resource* diffuseResource = resourceManager.GetTextureResource(albedoMapFileName);
		UINT handleOffset = m_perInstanceDescriptorCount + (m_perMaterialDescriptorCount * materialIndex);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc;
		cbDesc.BufferLocation = MatCBAddress + (materialIndex * materialCBElementSize);
		cbDesc.SizeInBytes = materialCBElementSize;

		// Mat CBV
		resourceManager.CreateCBV(heapAllocation.GetCpuHandle(handleOffset++), cbDesc);

		//Diffuse SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = diffuseResource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = diffuseResource->GetDesc().MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		resourceManager.CreateSRV(diffuseResource, heapAllocation.GetCpuHandle(handleOffset++), srvDesc);
	}

	DeleteTextureResource(oldTexture);
	m_HeapAllocationDirty--;
	MeshObject::ReAllocateDescriptorHeaps(frameIndex);
}

void StaticMesh::UpdateInstanceBuffers(ID3D12Device* device, int frameIndex)
{
	MeshObject::UpdateInstanceBuffers(frameIndex);
}

void StaticMesh::ReAllocateInstanceBuffer(ID3D12Device* device, int frameIndex)
{
	MeshObject::ReAllocateInstanceBuffer(frameIndex);
}
