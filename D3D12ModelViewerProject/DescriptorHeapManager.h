#pragma once

#include "VariableAllocationManager.h"
#include <d3d12.h>
#include <wrl.h>
#include <mutex>
#include <unordered_set>

class DescriptorHeapAllocation;
class IDescriptorAllocator;

class IDescriptorAllocator
{
public:
	virtual DescriptorHeapAllocation Allocate(uint32_t Count) = 0;
	virtual void Free(DescriptorHeapAllocation&& Allocation) = 0;
	virtual uint32_t GetDescriptorSize() const = 0;
};

//DescriptorHeap 내의 가상할당
class DescriptorHeapAllocation
{
public:
	DescriptorHeapAllocation();
	DescriptorHeapAllocation(
		IDescriptorAllocator* pAllocator,
		ID3D12DescriptorHeap* pHeap,
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
		uint32_t numHandles,
		size_t allocationManagerId
	);

	DescriptorHeapAllocation(DescriptorHeapAllocation&& allocation) noexcept
		:m_firstCPUhandle{ std::move(allocation.m_firstCPUhandle) },
		m_firstGPUhandle{ std::move(allocation.m_firstGPUhandle) },
		m_numHandles{ std::move(allocation.m_numHandles) },
		m_pAllocator{ std::move(allocation.m_pAllocator) },
		m_allocationManagerId{ std::move(allocation.m_allocationManagerId) },
		m_pDescriptorHeap{ std::move(allocation.m_pDescriptorHeap) },
		m_descriptorSize{ std::move(allocation.m_descriptorSize) }
	{
		allocation.Reset();
	};
	DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& allocation) noexcept
	{
		m_firstCPUhandle = std::move(allocation.m_firstCPUhandle);
		m_firstGPUhandle = std::move(allocation.m_firstGPUhandle);
		m_numHandles = std::move(allocation.m_numHandles);
		m_pAllocator = std::move(allocation.m_pAllocator);
		m_allocationManagerId = std::move(allocation.m_allocationManagerId);
		m_pDescriptorHeap = std::move(allocation.m_pDescriptorHeap);
		m_descriptorSize = std::move(allocation.m_descriptorSize);

		allocation.Reset();
		return *this;
	}

	~DescriptorHeapAllocation()
	{
		if (IsNull() == false && m_pAllocator != nullptr)
		{
			m_pAllocator->Free(std::move(*this));
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT Offset = 0) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = m_firstCPUhandle;
		if (Offset != 0)
			CPUHandle.ptr += m_descriptorSize * Offset;
		return CPUHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT Offset = 0) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = m_firstGPUhandle;
		if (Offset != 0)
			GPUHandle.ptr += m_descriptorSize * Offset;
		return GPUHandle;
	}

	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pDescriptorHeap; }
	uint32_t GetNumHandles() { return m_numHandles; }

	void Reset()
	{
		m_firstCPUhandle.ptr = 0;
		m_firstGPUhandle.ptr = 0;
		m_pAllocator = nullptr;
		m_pDescriptorHeap = nullptr;
		m_numHandles = 0;
		m_allocationManagerId = static_cast<size_t>(-1);
		m_descriptorSize = 0;
	}

	bool IsNull() const { return m_firstCPUhandle.ptr == 0; }
	bool IsShaderVisible() const { return m_firstGPUhandle.ptr != 0; }
	size_t GetAllocationManagerId() { return m_allocationManagerId; }
	UINT GetDescriptorSize() const { return m_descriptorSize; }
private:
	DescriptorHeapAllocation(const DescriptorHeapAllocation& allocation) = delete;
	DescriptorHeapAllocation& operator=(const DescriptorHeapAllocation& allocation) = delete;
private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_firstCPUhandle = { 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE m_firstGPUhandle = { 0 };
	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	IDescriptorAllocator* m_pAllocator = nullptr;
	uint32_t m_descriptorSize = 0;
	uint32_t m_numHandles = 0;
	size_t m_allocationManagerId = static_cast<size_t>(-1);
};

//DescriptorHeapAllocation를 할당하는 매니저클래스
class DescriptorHeapAllocationManager
{
public:
	DescriptorHeapAllocationManager(
		IDescriptorAllocator* pParentAllocator,
		size_t thisManagerId,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap,
		UINT firstDescriptor,
		UINT numDescriptors
	);

	DescriptorHeapAllocationManager(DescriptorHeapAllocationManager&& rhs) = default;

	DescriptorHeapAllocationManager& operator=(DescriptorHeapAllocationManager&& rhs) = default;
	DescriptorHeapAllocationManager& operator=(const DescriptorHeapAllocationManager&) = delete;
	DescriptorHeapAllocationManager(const DescriptorHeapAllocationManager&) = delete;

	~DescriptorHeapAllocationManager();

	DescriptorHeapAllocation Allocate(uint32_t count);
	void ReleaseStaleAllocations(uint64_t NumCompletedFrames);
	void Free(DescriptorHeapAllocation&& allocation);

	uint32_t GetNumAvailableDescriptors() const { return m_freeBlockManager.GetFreeSize(); }
	uint32_t GetMaxDescriptors() const { return m_descriptorSize; }

private:
	VariableSizeGPUAllocationsManager m_freeBlockManager;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;

	D3D12_CPU_DESCRIPTOR_HANDLE m_firstCPUHandle = { 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE m_firstGPUHandle = { 0 };

	uint32_t m_descriptorSize = 0;

	uint32_t m_numDescriptorsInAllocation = 0;

	std::mutex m_allocationMutex;
	IDescriptorAllocator* m_pParentAllocator = nullptr;

	size_t m_thisManagerId = static_cast<size_t>(-1);
};


class CPUDescriptorHeap final : public IDescriptorAllocator
{
private:
	static enum class EGPUDescriptorHeapType : int
	{
		CBV_SRV_UAV = 0,
		SAMPLER,
		COUNT
	};
public:
	// Initializes the heap
	CPUDescriptorHeap(
		ID3D12Device* device,
		uint32_t numDescriptorsInHeap,
		D3D12_DESCRIPTOR_HEAP_TYPE  type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	CPUDescriptorHeap(const CPUDescriptorHeap&) = delete;
	CPUDescriptorHeap(CPUDescriptorHeap&&) = delete;
	CPUDescriptorHeap& operator= (const CPUDescriptorHeap&) = delete;
	CPUDescriptorHeap& operator= (CPUDescriptorHeap&&) = delete;

	~CPUDescriptorHeap();

	virtual DescriptorHeapAllocation Allocate(uint32_t count) override final;
	virtual void Free(DescriptorHeapAllocation&& allocation) override final;
	virtual uint32_t GetDescriptorSize() const override final { return m_descriptorSize; }
	void ReleaseStaleAllocations(uint64_t numCompletedFrames);

private:

	// Pool of descriptor heap managers
	std::recursive_mutex m_heapPoolMutex;
	std::vector<std::unique_ptr<DescriptorHeapAllocationManager>> m_heapPool;
	// Indices of available descriptor heap managers
	std::unordered_set<size_t> m_availableHeaps;

	D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
	const UINT                 m_descriptorSize = 0;

	// Maximum heap size during the application lifetime - for statistic purposes
	uint32_t m_maxHeapSize = 0;
	uint32_t m_currentSize = 0;

	ID3D12Device* m_device;
};

class GPUDescriptorHeap final : public IDescriptorAllocator
{
public:
	GPUDescriptorHeap(
		ID3D12Device* device,
		uint32_t numDescriptorsInHeap,
		uint32_t numDynamicDescriptors,
		D3D12_DESCRIPTOR_HEAP_TYPE  type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	GPUDescriptorHeap(const GPUDescriptorHeap&) = delete;
	GPUDescriptorHeap(GPUDescriptorHeap&&) = delete;
	GPUDescriptorHeap& operator = (const GPUDescriptorHeap&) = delete;
	GPUDescriptorHeap& operator = (GPUDescriptorHeap&&) = delete;

	~GPUDescriptorHeap();

	virtual DescriptorHeapAllocation Allocate(uint32_t count) override final
	{
		return m_heapAllocationManager.Allocate(count);
	}

	virtual void Free(DescriptorHeapAllocation&& allocation) override final;
	virtual uint32_t GetDescriptorSize() const override final { return m_descriptorSize; }

	DescriptorHeapAllocation AllocateDynamic(uint32_t Count)
	{
		return m_dynamicAllocationsManager.Allocate(Count);
	}

	const D3D12_DESCRIPTOR_HEAP_DESC& GetHeapDesc() const { return m_heapDesc; }
	uint32_t GetMaxStaticDescriptors() const { return m_heapAllocationManager.GetMaxDescriptors(); }
	uint32_t GetMaxDynamicDescriptors() const { return m_dynamicAllocationsManager.GetMaxDescriptors(); }

	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_descriptorHeap.Get(); }
	void ReleaseStaleAllocations(uint64_t numCompletedFrames);
protected:
	ID3D12Device* m_device;

	const D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

	const UINT m_descriptorSize;

	// Allocation manager for static/mutable part
	DescriptorHeapAllocationManager m_heapAllocationManager;

	// Allocation manager for dynamic part
	DescriptorHeapAllocationManager m_dynamicAllocationsManager;
};


