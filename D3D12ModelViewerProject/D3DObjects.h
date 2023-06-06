#pragma once
#include "D3DUtil.h"
#include <deque>
#include "DescriptorHeapManager.h"

class GraphicsCommandListObject;
class CommandQueueObject;
class RenderCommandQueueObject;
struct RenderTargets;
struct DepthStencil;
class SwapChainObject;

class GraphicsCommandListObject
{
public:
	GraphicsCommandListObject(ID3D12Device* device);

public:

	void Reset()
	{
		ThrowIfFailed(m_commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
	}

	void Reset(ID3D12PipelineState* pipelineState)
	{
		ThrowIfFailed(m_commandAllocator->Reset());
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), pipelineState));
	}

	void Close()
	{
		m_commandList->Close();
	}

	ID3D12CommandAllocator* GetAllocatorPtr()
	{
		return m_commandAllocator.Get();
	}

	ID3D12GraphicsCommandList* GetCommandListPtr()
	{
		return m_commandList.Get();
	}
private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
};

class CommandQueueObject
{
private:
	struct CommandListWithFenceCount
	{
		GraphicsCommandListObject m_commandList;
		uint64_t m_fenceCount;
		CommandListWithFenceCount(GraphicsCommandListObject _commandList, uint64_t _fenceCount)
			: m_commandList(_commandList),
			m_fenceCount(_fenceCount)
		{}
	};
public:
	CommandQueueObject(ID3D12Device* device);
	CommandQueueObject(const CommandQueueObject&) = delete;
	CommandQueueObject& operator=(const CommandQueueObject&) = delete;
	CommandQueueObject& operator=(CommandQueueObject&&) = default;
	virtual ~CommandQueueObject() = default;

	// store=true면 commandlist보관
	void ExecuteCommandList(GraphicsCommandListObject cmdListObj, bool bStore = false);
	void ExecuteCommandList(std::vector<ID3D12CommandList*>& cmdListObjs);

	void ReleaseStoreCommandListObj();

	//fence증가 LastFenceCount반환 
	uint64_t Signal()
	{
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), ++m_lastFenceCount));
		return m_lastFenceCount;
	}
	uint64_t GetCompletedValue()
	{
		return m_fence->GetCompletedValue();
	}

	ID3D12CommandQueue* GetCommandQueuePtr()
	{
		return m_commandQueue.Get();
	}

	//LastFenceCount반환 fence증가
	virtual uint64_t FlushCommandQueue();

	//uint 언더플로우에 주의
	void WaitFence(uint64_t fenceCount);

protected:
	uint64_t m_lastFenceCount = 0;
private:
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;

	std::deque<CommandListWithFenceCount> m_storedCmdLists;
};

class RenderCommandQueueObject : public CommandQueueObject
{
public:
	RenderCommandQueueObject(ID3D12Device* device, size_t threadCount);
	RenderCommandQueueObject(const RenderCommandQueueObject&) = delete;
	RenderCommandQueueObject& operator=(const RenderCommandQueueObject&) = delete;
	RenderCommandQueueObject& operator=(RenderCommandQueueObject&&) = default;

public:
	GraphicsCommandListObject& GetThreadCommandList(int frameIndex, int threadIndex)
	{
		return m_threadCommandLists.at(threadIndex * FramesCount + frameIndex);
	}

	uint64_t GetCurrentFramesCount()
	{
		return m_currentFramesCount;
	}

	uint64_t GoNextFrame()
	{
		return ++m_currentFramesCount;
	}

	virtual uint64_t FlushCommandQueue() override final
	{
		return m_currentFramesCount = CommandQueueObject::FlushCommandQueue();
	}

private:
	size_t m_threadCount = 1;
	//할당구조 thread0(frame0,1,2...), thread1(frame0,1,2...) ....
	std::vector<GraphicsCommandListObject> m_threadCommandLists;
	uint64_t m_currentFramesCount = 0;
};

struct RenderTargets
{
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> Buffers;
	DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DescriptorHeapAllocation RtvHeapAlloc;
};

struct DepthStencil
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;
	DXGI_FORMAT Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DescriptorHeapAllocation DsvHeapAlloc;
};


class SwapChainObject
{
public:
	SwapChainObject(IDXGIFactory4* dxgiFactory, RenderCommandQueueObject* renderQueue, DXGI_SWAP_CHAIN_DESC& swapDesc);
	SwapChainObject(const SwapChainObject&) = delete;
	SwapChainObject& operator=(const SwapChainObject&) = delete;
	SwapChainObject& operator=(SwapChainObject&&) = default;
	~SwapChainObject() = default;
public:
	void Resize(ID3D12Device* device, RenderCommandQueueObject* renderQueue, int width, int height);

	DXGI_FORMAT GetBackBufferFormat()
	{
		return m_renderTargetBuffers.Format;
	}

	DXGI_FORMAT GetDepthStencilFormat()
	{
		return m_depthStencil.Format;
	}

	void Reset()
	{
		m_swapChain.Reset();
	}

	ID3D12Resource* GetCurrentBackBuffer()
	{
		return m_renderTargetBuffers.Buffers.at(m_currBackBufferIndex).Get();
	}

	ID3D12Resource* GetDepthStencilBuffer()
	{
		return m_depthStencil.Buffer.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView()
	{
		return m_renderTargetBuffers.RtvHeapAlloc.GetCpuHandle(m_currBackBufferIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView()
	{
		return m_depthStencil.DsvHeapAlloc.GetCpuHandle();
	}

	void Present()
	{
		ThrowIfFailed(m_swapChain->Present(0, 0));
		m_currBackBufferIndex = (m_currBackBufferIndex + 1) % m_swapChainBufferCount;
	}

	D3D12_VIEWPORT GetViewport()
	{
		return m_viewport;
	}

	D3D12_RECT GetScissorRect()
	{
		return m_scissorRect;
	}



private:
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	RenderTargets m_renderTargetBuffers;
	DepthStencil m_depthStencil;

	UINT m_swapChainBufferCount = 0;
	int m_currBackBufferIndex = 0;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};