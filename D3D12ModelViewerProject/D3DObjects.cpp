#include "D3DObjects.h"
#include "D3DResourceManager.h"
#include <DirectXColors.h>

GraphicsCommandListObject::GraphicsCommandListObject(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));

	ThrowIfFailed(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(m_commandList.GetAddressOf())));

	m_commandList->Close();
}

CommandQueueObject::CommandQueueObject(ID3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(&m_commandQueue)));

	ThrowIfFailed(device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_fence)));
}

void CommandQueueObject::ExecuteCommandList(GraphicsCommandListObject cmdListObj, bool bStore)
{
	ID3D12CommandList* cmdLists[] = { cmdListObj.GetCommandListPtr() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	if (bStore == true)
	{
		m_storedCmdLists.emplace_back(cmdListObj, m_lastFenceCount + 1);
	}
}

void CommandQueueObject::ExecuteCommandList(std::vector<ID3D12CommandList*>& cmdListObjs)
{
	m_commandQueue->ExecuteCommandLists(cmdListObjs.size(), cmdListObjs.data());
}

void CommandQueueObject::ReleaseStoreCommandListObj()
{
	uint64_t fenceCount = m_fence->GetCompletedValue();

	while (m_storedCmdLists.empty() == false && m_storedCmdLists.front().m_fenceCount <= fenceCount)
	{
		m_storedCmdLists.pop_front();
	}
}

uint64_t CommandQueueObject::FlushCommandQueue()
{
	Signal();
	WaitFence(m_lastFenceCount);

	return m_lastFenceCount;
}

void CommandQueueObject::WaitFence(uint64_t fenceCount)
{
	if (m_fence->GetCompletedValue() < fenceCount)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(fenceCount, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

RenderCommandQueueObject::RenderCommandQueueObject(ID3D12Device* device, size_t threadCount)
	: CommandQueueObject(device), m_threadCount(threadCount)
{
	int threadCmdListCount = FramesCount * threadCount;
	for (int i = 0; i < threadCmdListCount; ++i)
	{
		m_threadCommandLists.emplace_back(device);
	}
}

SwapChainObject::SwapChainObject(IDXGIFactory4* dxgiFactory, RenderCommandQueueObject* renderQueue, DXGI_SWAP_CHAIN_DESC& swapDesc)
{
	m_swapChain.Reset();

	ThrowIfFailed(dxgiFactory->CreateSwapChain(
		renderQueue->GetCommandQueuePtr(),
		&swapDesc,
		m_swapChain.GetAddressOf()));

	m_swapChainBufferCount = swapDesc.BufferCount;

	for (UINT i = 0; i < swapDesc.BufferCount; ++i)
	{
		m_renderTargetBuffers.Buffers.push_back(nullptr);
	}

	auto& resourceManager = D3DResourceManager::GetInstance();

	m_renderTargetBuffers.RtvHeapAlloc = resourceManager.CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, swapDesc.BufferCount);
	m_depthStencil.DsvHeapAlloc = resourceManager.CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
}

void SwapChainObject::Resize(ID3D12Device* device, RenderCommandQueueObject* renderQueue, int width, int height)
{
	assert(m_swapChain != nullptr);

	renderQueue->FlushCommandQueue();

	GraphicsCommandListObject cmdListObj(device);
	cmdListObj.Reset();

	for (UINT i = 0; i < m_swapChainBufferCount; ++i)
	{
		m_renderTargetBuffers.Buffers.at(i).Reset();
	}
	m_depthStencil.Buffer.Reset();

	//resize SwapchainBuffer
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		m_swapChainBufferCount,
		width,
		height,
		m_renderTargetBuffers.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_currBackBufferIndex = 0;

	//rebuild RTV
	for (UINT i = 0; i < m_swapChainBufferCount; ++i)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargetBuffers.Buffers.at(i))));

		D3DResourceManager::GetInstance().CreateRTV(
			m_renderTargetBuffers.Buffers.at(i).Get(),
			m_renderTargetBuffers.RtvHeapAlloc.GetCpuHandle(i),
			nullptr);
	}

	//rebuild depthStencil Buffer
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = m_depthStencil.Format;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_depthStencil.Format;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_depthStencil.Buffer.GetAddressOf())));

	//rebuild DSV
	device->CreateDepthStencilView(
		m_depthStencil.Buffer.Get(),
		nullptr,
		m_depthStencil.DsvHeapAlloc.GetCpuHandle());

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencil.Buffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cmdListObj.GetCommandListPtr()->ResourceBarrier(
		1,
		&resourceBarrier);

	cmdListObj.Close();
	//execute command
	renderQueue->ExecuteCommandList(cmdListObj, true);
	renderQueue->FlushCommandQueue();

	//rebuild viewport,scissorRect
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect = { 0,0,width,height };
}

