#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include "D3DResourceManager.h"
#include "ResourceUploadBatch.h"
#include "FbxUtil.h"
#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx12.h"
#include "FileUtil.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

void D3DResourceManager::CopyDescriptorHeapAllocation(DescriptorHeapAllocation& source, DescriptorHeapAllocation& dest)
{
	m_device->CopyDescriptorsSimple(
		source.GetNumHandles(),
		dest.GetCpuHandle(),
		source.GetCpuHandle(),
		dest.GetDescriptorHeap()->GetDesc().Type);
}

D3DResourceManager::D3DResourceManager()
{
	InitDirect3D();
	BuildDescriptorHeaps(m_device.Get());

	BuildBlendState();
	BuildShaders();

	CreateDefaultTextures();
	m_renderFutures.resize(ThreadCount);
}

void D3DResourceManager::BuildDescriptorHeaps(ID3D12Device* device)
{
	//build cpuDescriptorHeap
	uint32_t numCpuDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		10000,
		100,
		100,
		100,
	};

	D3D12_DESCRIPTOR_HEAP_TYPE cpuDescriptorHeapTypes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	for (int i = 0; i < _countof(cpuDescriptorHeapTypes); ++i)
	{
		m_cpuDescriptorHeapsMap.try_emplace(cpuDescriptorHeapTypes[i], device, numCpuDescriptors[i], cpuDescriptorHeapTypes[i], D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	//build gpuDescriptorHeap
	uint32_t numGpuStaticDescriptors[] =
	{
		1000,
		10
	};
	uint32_t numGpuDynamicDescriptors[] =
	{
		100000,
		10
	};

	D3D12_DESCRIPTOR_HEAP_TYPE gpuDescriptorHeapTypes[] =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
	};

	for (int i = 0; i < _countof(gpuDescriptorHeapTypes); ++i)
	{
		m_gpuDescriptorHeapsMap.try_emplace(
			gpuDescriptorHeapTypes[i],
			device,
			numGpuStaticDescriptors[i],
			numGpuDynamicDescriptors[i],
			gpuDescriptorHeapTypes[i],
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	}

}

/// <summary>
/// </summary>
/// <param name="textureName">프로그램내에서 고유한 이름</param>
/// <returns>실패하면 nullptr 반환</returns>
ID3D12Resource* D3DResourceManager::GetTextureResource(std::wstring textureName)
{
	ID3D12Resource* result = nullptr;

	auto iter = m_textureResourceMap.find(textureName);
	if (iter != m_textureResourceMap.end())
	{
		result = iter->second.second.Get();
	}
	return result;
}

/// <summary>
/// 
/// </summary>
/// <param name="textureName">프로그램내에서 고유한 이름</param>
/// <param name="textureFilePath">텍스처파일 절대경로</param>
void D3DResourceManager::CreateTextureResource(std::wstring TextureName, std::wstring textureFilePath)
{
	if (m_device == nullptr)
	{
		return;
	}

	TextureResourcePair* resourcePair = GetTextureResourcePair(TextureName);
	if (resourcePair != nullptr)
	{
		resourcePair->first++;
		return;
	}

	if (FileUtil::IsFileExist(textureFilePath) == false)
	{
		m_textureResourceMap[TextureName] = make_pair(1, m_textureResourceMap[_TEXT("defaultDiffuseTexture")].second);
		return;
	}

	ComPtr<ID3D12Resource> resource;

	ResourceUploadBatch upload(m_device.Get());
	upload.Begin();

	try
	{
		ThrowIfFailed(CreateWICTextureFromFile(
			m_device.Get(),
			upload,
			textureFilePath.c_str(),
			resource.ReleaseAndGetAddressOf()));
	}
	catch (exception e)
	{
		m_textureResourceMap[TextureName] = make_pair(1, m_textureResourceMap[_TEXT("defaultDiffuseTexture")].second);
		return;
	}

	resource->SetName(TextureName.c_str());

	m_textureResourceMap[TextureName] = make_pair(1, move(resource));

	auto finish2 = upload.End(m_copyQueue->GetCommandQueuePtr());

	finish2.wait();
}

void D3DResourceManager::DeleteTextureResource(std::wstring textureName)
{
	TextureResourcePair* resourcePair = GetTextureResourcePair(textureName);
	if (resourcePair == nullptr)
	{
		return;
	}

	resourcePair->first--;
	if (resourcePair->first <= 0)
	{
		m_textureResourceMap.erase(textureName);
	}
}


DescriptorHeapAllocation D3DResourceManager::CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t count)
{
	auto iter = m_cpuDescriptorHeapsMap.find(heapType);
	if (iter != m_cpuDescriptorHeapsMap.end())
	{
		return iter->second.Allocate(count);
	}

	return DescriptorHeapAllocation();
}

DescriptorHeapAllocation D3DResourceManager::GpuDynamicDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t count)
{
	auto iter = m_gpuDescriptorHeapsMap.find(heapType);
	if (iter != m_gpuDescriptorHeapsMap.end())
	{
		return iter->second.AllocateDynamic(count);
	}

	return DescriptorHeapAllocation();
}

void D3DResourceManager::CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc)
{
	m_device->CreateConstantBufferView(&viewDesc, cpuHandle);
}

void D3DResourceManager::CreateSRV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc)
{
	m_device->CreateShaderResourceView(pResource, &viewDesc, cpuHandle);
}

void D3DResourceManager::CreateRTV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_RENDER_TARGET_VIEW_DESC* viewDesc)
{
	m_device->CreateRenderTargetView(pResource, viewDesc, cpuHandle);
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3DResourceManager::CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	GraphicsCommandListObject cmdListObj(m_device.Get());
	cmdListObj.Reset();

	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = D3DUtil::CreateDefaultBuffer(m_device.Get(), cmdListObj.GetCommandListPtr(), initData, byteSize, uploadBuffer);

	cmdListObj.Close();
	m_copyQueue->ExecuteCommandList(std::move(cmdListObj), true);
	m_copyQueue->Signal();

	return  buffer;
}

D3DResourceManager::TextureResourcePair* D3DResourceManager::GetTextureResourcePair(std::wstring textureName)
{
	TextureResourcePair* result = nullptr;
	auto iter = m_textureResourceMap.find(textureName);
	if (iter != m_textureResourceMap.end())
	{
		result = &(iter->second);
	}
	return result;
}

void D3DResourceManager::Render(const DescriptorHeapAllocation& passDescriptorHeapAllocation)
{
	ImGui::Render();
	//reset & ClearViews
	GraphicsCommandListObject& firstCommandListObject = m_renderQueue->GetThreadCommandList(GetCurrentFrameIndex(), 0);
	ID3D12GraphicsCommandList* firstCommandList = firstCommandListObject.GetCommandListPtr();

	for (int threadIndex = 0; threadIndex < ThreadCount; ++threadIndex)
	{
		m_renderQueue->GetThreadCommandList(GetCurrentFrameIndex(), threadIndex).Reset();
	}

	auto resourceBarrierPresentToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChain->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	firstCommandList->ResourceBarrier(1, &resourceBarrierPresentToRenderTarget);

	D3D12_CPU_DESCRIPTOR_HANDLE currBackBufferView = m_swapChain->GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = m_swapChain->GetDepthStencilView();
	firstCommandList->ClearRenderTargetView(currBackBufferView, Colors::LightSteelBlue, 0, nullptr);
	firstCommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	for (int blendType = 0; blendType < (int)BlendType::Count; ++blendType)
	{
		for (int shaderType = 0; shaderType < (int)ShaderType::Count; ++shaderType)
		{
			DrawRenderLists(
				currBackBufferView,
				depthStencilView,
				RenderTypes::GetRenderType(static_cast<ShaderType>(shaderType), static_cast<BlendType>(blendType)),
				passDescriptorHeapAllocation.GetGpuHandle());
		}
	}

	ID3D12GraphicsCommandList* lastCommandList = m_renderQueue->GetThreadCommandList(GetCurrentFrameIndex(), ThreadCount - 1).GetCommandListPtr();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), lastCommandList);

	//execute & present
	auto resourceBarrierRenderTargetToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		m_swapChain->GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	lastCommandList->ResourceBarrier(1, &resourceBarrierRenderTargetToPresent);

	vector<ID3D12CommandList*> cmdLists;
	for (int threadIndex = 0; threadIndex < ThreadCount; ++threadIndex)
	{
		GraphicsCommandListObject& threadCommandListObj = m_renderQueue->GetThreadCommandList(GetCurrentFrameIndex(), threadIndex);
		threadCommandListObj.Close();
		cmdLists.push_back(threadCommandListObj.GetCommandListPtr());
	}
	m_renderQueue->ExecuteCommandList(cmdLists);
	m_renderQueue->Signal();
	m_swapChain->Present();
}

void D3DResourceManager::CreateSwapChain(DXGI_SWAP_CHAIN_DESC& swapDesc)
{
	m_swapChain = make_unique<SwapChainObject>(m_dxgiFactory.Get(), m_renderQueue.get(), swapDesc);
	BuildPipelineState();
}

void D3DResourceManager::Update()
{
	uint64_t currentFrameCount = m_renderQueue->GoNextFrame();

	//uint 언더플로우에 주의
	uint64_t fenceCount = (currentFrameCount < FramesCount) ? 0 : currentFrameCount - FramesCount;
	m_renderQueue->WaitFence(fenceCount);
	m_renderQueue->ReleaseStoreCommandListObj();
	m_copyQueue->ReleaseStoreCommandListObj();

	m_renderLists.at(GetCurrentFrameIndex()).clear();

	UpdateStaleAllocations();
}

bool D3DResourceManager::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ComPtr<ID3D12Debug1> debugController2;

		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		debugController->QueryInterface(IID_PPV_ARGS(&debugController2));
		debugController2->SetEnableGPUBasedValidation(true);
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device));

	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}

	//initialize queue
	m_copyQueue = make_unique<CommandQueueObject>(m_device.Get());
	m_renderQueue = make_unique<RenderCommandQueueObject>(m_device.Get(), ThreadCount);

	return true;
}

void D3DResourceManager::UpdateStaleAllocations()
{
	for (auto& element : m_cpuDescriptorHeapsMap)
	{
		element.second.ReleaseStaleAllocations(GetCurrentFrameCount());
	}

	for (auto& element : m_gpuDescriptorHeapsMap)
	{
		element.second.ReleaseStaleAllocations(GetCurrentFrameCount());
	}
}

void D3DResourceManager::DrawRenderLists(D3D12_CPU_DESCRIPTOR_HANDLE backbufferView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, RenderType renderType, D3D12_GPU_DESCRIPTOR_HANDLE passDescriptorHandle)
{
	auto descriptorHeapIter = m_gpuDescriptorHeapsMap.find(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeapIter->second.GetDescriptorHeap() };
	D3D12_VIEWPORT&& viewport = m_swapChain->GetViewport();
	D3D12_RECT&& scissorRect = m_swapChain->GetScissorRect();

	auto& renderList = m_renderLists.at(GetCurrentFrameIndex());
	int meshCountPerThread = floor(renderList.size() / ThreadCount);

	auto BeginRenderListIterator = renderList.begin();
	auto EndRenderListIterator = renderList.end();

	for (int threadIndex = 0; threadIndex < ThreadCount; ++threadIndex)
	{
		auto commandList = m_renderQueue->GetThreadCommandList(GetCurrentFrameIndex(), threadIndex).GetCommandListPtr();
		commandList->SetPipelineState(m_pipelineStates[renderType].Get());

		EndRenderListIterator = BeginRenderListIterator + meshCountPerThread;

		if (threadIndex == ThreadCount - 1)
		{
			EndRenderListIterator = renderList.end();
		}

		//thread start
		m_renderFutures.at(threadIndex) = std::async(std::launch::async, [=]()
			{
				//set renderTarget & viewports
				commandList->OMSetRenderTargets(1, &backbufferView, true, &depthStencilView);
				commandList->RSSetViewports(1, &viewport);
				commandList->RSSetScissorRects(1, &scissorRect);

				//Set ShaderVisible DescriptorHeap && rootSignature			
				commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
				commandList->SetGraphicsRootSignature(m_shaderObjects[renderType.ShaderMode]->RootSignature.Get());

				//Set PassCB		
				commandList->SetGraphicsRootDescriptorTable(0, passDescriptorHandle);

				for (auto iter = BeginRenderListIterator; iter != EndRenderListIterator; ++iter)
				{
					(*iter)->Render(
						commandList,
						renderType
					);
				}
			});

		BeginRenderListIterator = EndRenderListIterator;
	}

	for (auto& future : m_renderFutures)
	{
		future.wait();
	}

}

void D3DResourceManager::BuildShaders()
{
	auto staticSamplers = GetStaticSamplers();

	//build StaticMeshShader & rootsignature
	auto staticMeshShader = make_unique<ShaderObject>();

	staticMeshShader->PSFileName = _TEXT("Shaders\\StaticMeshShader.hlsl");
	staticMeshShader->VSFileName = _TEXT("Shaders\\StaticMeshShader.hlsl");
	staticMeshShader->Compile();

	staticMeshShader->InputLayout =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	{
		CD3DX12_DESCRIPTOR_RANGE passRange[2];
		passRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		passRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3);

		CD3DX12_DESCRIPTOR_RANGE objRange[2];
		objRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//objConstant
		objRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);//instanceData

		CD3DX12_DESCRIPTOR_RANGE matRange[2];
		matRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
		matRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[3];

		slotRootParameter[0].InitAsDescriptorTable(_countof(passRange), passRange, D3D12_SHADER_VISIBILITY_ALL); // pass
		slotRootParameter[1].InitAsDescriptorTable(_countof(objRange), objRange, D3D12_SHADER_VISIBILITY_ALL); // obj
		slotRootParameter[2].InitAsDescriptorTable(_countof(matRange), matRange, D3D12_SHADER_VISIBILITY_ALL); // mat,tex 

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(staticMeshShader->RootSignature.GetAddressOf())));
	}
	m_shaderObjects.emplace(ShaderType::StaticMeshShader, std::move(staticMeshShader));

	//Build DynamicMeshShader & rootsignature
	auto dynamicMeshShader = make_unique<ShaderObject>();

	dynamicMeshShader->PSFileName = _TEXT("Shaders\\DynamicMeshShader.hlsl");
	dynamicMeshShader->VSFileName = _TEXT("Shaders\\DynamicMeshShader.hlsl");
	dynamicMeshShader->Compile();

	dynamicMeshShader->InputLayout =
	{
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "WEIGHTS",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "BONEINDICES",0,DXGI_FORMAT_R32G32B32A32_UINT,0,48,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	{
		CD3DX12_DESCRIPTOR_RANGE passRange[2];
		passRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		passRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3);

		CD3DX12_DESCRIPTOR_RANGE objRange[3];
		objRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//objConstant
		objRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);//instanceData 
		objRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2);//instanceAnimation

		CD3DX12_DESCRIPTOR_RANGE matRange[2];
		matRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
		matRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[3];

		slotRootParameter[0].InitAsDescriptorTable(_countof(passRange), passRange, D3D12_SHADER_VISIBILITY_ALL); // pass
		slotRootParameter[1].InitAsDescriptorTable(_countof(objRange), objRange, D3D12_SHADER_VISIBILITY_ALL); // obj
		slotRootParameter[2].InitAsDescriptorTable(_countof(matRange), matRange, D3D12_SHADER_VISIBILITY_ALL); // mat,tex

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(dynamicMeshShader->RootSignature.GetAddressOf())));
	}

	m_shaderObjects.emplace(ShaderType::DynamicMeshShader, std::move(dynamicMeshShader));
}

void D3DResourceManager::BuildBlendState()
{
	//Opaque
	m_blendDescs[(int)BlendType::Opaque] = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//TransParent
	D3D12_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = false;
	desc.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc;
	rtBlendDesc.BlendEnable = true;
	rtBlendDesc.LogicOpEnable = false;
	rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	desc.RenderTarget[0] = rtBlendDesc;

	m_blendDescs[(int)BlendType::Transparent] = desc;
}

void D3DResourceManager::PushRenderItem(std::shared_ptr<SceneObject> addItem)
{
	m_renderLists.at(GetCurrentFrameIndex()).push_back(addItem);
}


void D3DResourceManager::ResizeSwapChain(int width, int height)
{
	m_swapChain->Resize(m_device.Get(), m_renderQueue.get(), width, height);
}

void D3DResourceManager::InitializeImGUID3D12()
{
	auto descriptorHeapIter = m_gpuDescriptorHeapsMap.find(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	assert(descriptorHeapIter != m_gpuDescriptorHeapsMap.end());

	DescriptorHeapAllocation allocation = descriptorHeapIter->second.Allocate(1);
	assert(allocation.IsNull() == false);

	ID3D12DescriptorHeap* pHeap = descriptorHeapIter->second.GetDescriptorHeap();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocation.GetCpuHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = allocation.GetGpuHandle();

	ImGui_ImplDX12_Init(
		m_device.Get(),
		FramesCount,
		m_swapChain->GetBackBufferFormat(),
		pHeap,
		cpuHandle,
		gpuHandle);

	m_staticItems.emplace(_TEXT("ImGUIFont"), move(allocation));
}

void D3DResourceManager::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (m_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = _TEXT("***Adapter: ");
		text += desc.Description;
		text += _TEXT("\n");

		OutputDebugString(text.c_str());
		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DResourceManager::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = _TEXT("***Output: ");
		text += desc.DeviceName;
		text += _TEXT("\n");
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);

		ReleaseCom(output);

		++i;
	}
}

void D3DResourceManager::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text = _TEXT("Width = ") + std::to_wstring(x.Width) + _TEXT(" ") +
			_TEXT("Height = ") + std::to_wstring(x.Height) + _TEXT(" ") +
			_TEXT("Refresh = ") + std::to_wstring(n) + std::to_wstring(n) +
			_TEXT("\n");

		OutputDebugString(text.c_str());
	}

}

void D3DResourceManager::BuildPipelineState()
{

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;

	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_swapChain->GetBackBufferFormat();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_swapChain->GetDepthStencilFormat();

	for (auto& element : m_shaderObjects)
	{
		auto& shaderObj = element.second;
		psoDesc.InputLayout = { shaderObj->InputLayout.data(),(UINT)shaderObj->InputLayout.size() };
		psoDesc.pRootSignature = shaderObj->RootSignature.Get();
		psoDesc.VS =
		{
			reinterpret_cast<BYTE*>(shaderObj->VS->GetBufferPointer()),
			shaderObj->VS->GetBufferSize()
		};
		psoDesc.PS =
		{
			reinterpret_cast<BYTE*>(shaderObj->PS->GetBufferPointer()),
			shaderObj->PS->GetBufferSize()
		};

		int blendTypeCount = static_cast<int>(BlendType::Count);
		for (int blendType = 0; blendType < blendTypeCount; ++blendType)
		{
			psoDesc.BlendState = m_blendDescs[blendType];

			RenderType rType{
				static_cast<ShaderType>(element.first),
				static_cast<BlendType>(blendType)
			};
			auto& pipelineState = m_pipelineStates[rType];
			ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf())));
		}
	}

}

void D3DResourceManager::CreateDefaultTextures()
{
	D3D12_RESOURCE_DESC txtDesc = {};
	txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
	txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	txtDesc.Width = 1;
	txtDesc.Height = 1;
	txtDesc.SampleDesc.Count = 1;
	txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	const static uint32_t whiteColor = 0xffffffff;

	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	ThrowIfFailed(
		m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&txtDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(texture.ReleaseAndGetAddressOf())));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &whiteColor;
	textureData.RowPitch = txtDesc.Width * 4;
	textureData.SlicePitch = txtDesc.Height * txtDesc.Width * 4;

	ResourceUploadBatch resourceUpload(m_device.Get());

	resourceUpload.Begin();

	resourceUpload.Upload(texture.Get(), 0, &textureData, 1);

	resourceUpload.Transition(
		texture.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto uploadResourcesFinished = resourceUpload.End(m_copyQueue->GetCommandQueuePtr());

	uploadResourcesFinished.wait();

	m_textureResourceMap[_TEXT("defaultDiffuseTexture")] = make_pair(1, std::move(texture));
}


std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3DResourceManager::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}
