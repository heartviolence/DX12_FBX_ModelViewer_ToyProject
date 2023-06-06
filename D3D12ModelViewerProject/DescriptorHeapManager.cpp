
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include "DescriptorHeapManager.h"
#include "D3DUtil.h"
#include "D3DResourceManager.h"
#include <algorithm>



DescriptorHeapAllocationManager::~DescriptorHeapAllocationManager()
{
}

DescriptorHeapAllocation DescriptorHeapAllocationManager::Allocate(uint32_t count)
{
	std::lock_guard<std::mutex> lockGuard(m_allocationMutex);

	OffsetType descriptorHandleOffset = m_freeBlockManager.Allocate(count);
	if (descriptorHandleOffset == VariableSizeGPUAllocationsManager::InvalidOffset)
	{
		return DescriptorHeapAllocation();
	}

	auto CPUHandle = m_firstCPUHandle;
	CPUHandle.ptr += descriptorHandleOffset * m_descriptorSize;

	auto GPUHandle = m_firstGPUHandle;
	if (m_heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		GPUHandle.ptr += descriptorHandleOffset * m_descriptorSize;
	}

	return DescriptorHeapAllocation(m_pParentAllocator, m_pDescriptorHeap.Get(), CPUHandle, GPUHandle, count, m_thisManagerId);
}

void DescriptorHeapAllocationManager::ReleaseStaleAllocations(uint64_t NumCompletedFrames)
{
	std::lock_guard<std::mutex> lockGuard(m_allocationMutex);
	m_freeBlockManager.ReleaseCompletedFrames(NumCompletedFrames);
}

void DescriptorHeapAllocationManager::Free(DescriptorHeapAllocation&& allocation)
{
	std::lock_guard<std::mutex> lockGuard(m_allocationMutex);
	auto DescriptorOffset = (allocation.GetCpuHandle().ptr - m_firstCPUHandle.ptr) / m_descriptorSize;
	m_freeBlockManager.Free(DescriptorOffset, allocation.GetNumHandles(), D3DResourceManager::GetInstance().GetCurrentFrameCount());
	allocation = DescriptorHeapAllocation();
}


DescriptorHeapAllocationManager::DescriptorHeapAllocationManager(IDescriptorAllocator* pParentAllocator, size_t thisManagerId, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap, UINT firstDescriptor, UINT numDescriptors)
	:
	m_pParentAllocator(pParentAllocator),
	m_thisManagerId(thisManagerId),
	m_pDescriptorHeap(pDescriptorHeap),
	m_freeBlockManager(numDescriptors),
	m_heapDesc(pDescriptorHeap->GetDesc())
{
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	ThrowIfFailed(pDescriptorHeap->GetDevice(IID_PPV_ARGS(device.ReleaseAndGetAddressOf())));
	m_descriptorSize = device->GetDescriptorHandleIncrementSize(m_heapDesc.Type);


	m_firstCPUHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_firstCPUHandle.ptr += m_descriptorSize * firstDescriptor;
	if (m_heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		m_firstGPUHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_firstGPUHandle.ptr += m_descriptorSize * firstDescriptor;
	}
}

DescriptorHeapAllocation::DescriptorHeapAllocation()
{
}

DescriptorHeapAllocation::DescriptorHeapAllocation(IDescriptorAllocator* pAllocator, ID3D12DescriptorHeap* pHeap, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t numHandles, size_t allocationManagerId)
	:m_pAllocator(pAllocator),
	m_pDescriptorHeap(pHeap),
	m_firstCPUhandle(cpuHandle),
	m_firstGPUhandle(gpuHandle),
	m_numHandles(numHandles),
	m_allocationManagerId(allocationManagerId)
{
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	ThrowIfFailed(m_pDescriptorHeap->GetDevice(IID_PPV_ARGS(device.ReleaseAndGetAddressOf())));

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(m_pDescriptorHeap->GetDesc().Type);
}

CPUDescriptorHeap::CPUDescriptorHeap(ID3D12Device* device, uint32_t numDescriptorsInHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	:m_device(device),
	m_descriptorSize(device->GetDescriptorHandleIncrementSize(type)),
	m_heapDesc
{
	type,
	numDescriptorsInHeap,
	flags,
	0
}
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	ThrowIfFailed(device->CreateDescriptorHeap(&m_heapDesc, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf())));
	m_heapPool.emplace_back(std::make_unique<DescriptorHeapAllocationManager>(this, m_heapPool.size(), descriptorHeap.Get(), 0, m_heapDesc.NumDescriptors));

	m_availableHeaps.insert(0);
}

CPUDescriptorHeap::~CPUDescriptorHeap()
{

}

DescriptorHeapAllocation CPUDescriptorHeap::Allocate(uint32_t count)
{
	std::lock_guard<std::recursive_mutex> lockGuard(m_heapPoolMutex);
	DescriptorHeapAllocation allocation;

	//여분공간이있는 힙 검색후 할당
	for (auto& availableHeap : m_availableHeaps)
	{
		allocation = m_heapPool.at(availableHeap)->Allocate(count);
		if (m_heapPool.at(availableHeap)->GetNumAvailableDescriptors() == 0)
		{
			m_availableHeaps.erase(availableHeap);
		}

		if (allocation.IsNull() == false)
		{
			break;
		}
	}

	if (allocation.IsNull() == true)
	{
		m_heapDesc.NumDescriptors = std::max(m_heapDesc.NumDescriptors, static_cast<UINT>(count));
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&m_heapDesc, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf())));
		m_heapPool.emplace_back(std::make_unique<DescriptorHeapAllocationManager>(this, m_heapPool.size(), descriptorHeap.Get(), 0, m_heapDesc.NumDescriptors));

		auto newHeapIter = m_availableHeaps.insert(m_heapPool.size() - 1);
		allocation = m_heapPool.at(*(newHeapIter.first))->Allocate(count);
	}
	m_currentSize += (allocation.GetCpuHandle().ptr != 0) ? count : 0;
	m_maxHeapSize = std::max(m_maxHeapSize, m_currentSize);

	return allocation;
}

void CPUDescriptorHeap::Free(DescriptorHeapAllocation&& allocation)
{
	std::lock_guard<std::recursive_mutex> lockGuard(m_heapPoolMutex);
	auto managerId = allocation.GetAllocationManagerId();
	m_currentSize -= static_cast<uint32_t>(allocation.GetNumHandles());
	m_heapPool.at(managerId)->Free(std::move(allocation));
}

void CPUDescriptorHeap::ReleaseStaleAllocations(uint64_t numCompletedFrames)
{
	std::lock_guard<std::recursive_mutex> lockGuard(m_heapPoolMutex);
	for (size_t heapManagerId = 0; heapManagerId < m_heapPool.size(); ++heapManagerId)
	{
		m_heapPool.at(heapManagerId)->ReleaseStaleAllocations(numCompletedFrames);

		if (m_heapPool.at(heapManagerId)->GetNumAvailableDescriptors() > 0)
		{
			m_availableHeaps.insert(heapManagerId);
		}
	}
}

GPUDescriptorHeap::GPUDescriptorHeap(ID3D12Device* device, uint32_t numDescriptorsInHeap, uint32_t numDynamicDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	:m_device(device),
	m_heapDesc
{
	type,
	numDescriptorsInHeap + numDynamicDescriptors,
	flags,
	0
},
m_descriptorHeap
{
	[&]()
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	device->CreateDescriptorHeap(&m_heapDesc, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf()));
	return descriptorHeap;
}()
},
m_descriptorSize(device->GetDescriptorHandleIncrementSize(type)),
m_heapAllocationManager(this, 0, m_descriptorHeap.Get(), 0, numDescriptorsInHeap),
m_dynamicAllocationsManager(this, 1, m_descriptorHeap.Get(), numDescriptorsInHeap, numDynamicDescriptors)
{
	//본문
}

GPUDescriptorHeap::~GPUDescriptorHeap()
{
}

void GPUDescriptorHeap::Free(DescriptorHeapAllocation&& allocation)
{
	auto managerId = allocation.GetAllocationManagerId();

	if (managerId == 0)
	{
		m_heapAllocationManager.Free(std::move(allocation));
	}
	else
	{
		m_dynamicAllocationsManager.Free(std::move(allocation));
	}
}


void GPUDescriptorHeap::ReleaseStaleAllocations(uint64_t numCompletedFrames)
{
	m_heapAllocationManager.ReleaseStaleAllocations(numCompletedFrames);
	m_dynamicAllocationsManager.ReleaseStaleAllocations(numCompletedFrames);
}