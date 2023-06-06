#pragma once

#include "D3DUtil.h"
#include "FbxUtil.h"
#include <wrl.h>
#include "FrameResource.h"
#include "DescriptorHeapManager.h"
#include "MeshResources.h"
#include "PipelineState.h"
#include "D3DObjects.h"
#include <functional>
#include "JobQueue.h"
#include <future>
#include "SceneObject.h"

class D3DResourceManager
{
private:
	using TextureResourcePair = std::pair<int, Microsoft::WRL::ComPtr<ID3D12Resource>>;
	using VoidFunction = std::function<void()>;
private:
	struct DeletedResource
	{
		DeletedResource() = default;
		DeletedResource(std::wstring _name, uint64_t _frameCount) :
			Name(_name),
			FrameCount(_frameCount) {}

		std::wstring Name;
		uint64_t FrameCount;
	};
public:
	static D3DResourceManager& GetInstance()
	{
		static D3DResourceManager instance;
		return instance;
	}

	ID3D12Device* GetDevice() { return m_device.Get(); }

	//실패시 nullptr반환
	ID3D12Resource* GetTextureResource(std::wstring textureName = _TEXT("defaultDiffuseTexture"));

	// 절대경로를 통해 텍스처 생성후 textureName으로 해쉬맵에 저장 ,같은 키로 생성시 레퍼런스카운트+1
	void CreateTextureResource(std::wstring TextureName, std::wstring textureFilePath);
	void CreateDefaultTextures();

	//리소스 레퍼런스카운트-1 /0으로 내려가면 리소스파괴
	void DeleteTextureResource(std::wstring textureName);

	//현재 렌더 프레임반환
	uint32_t GetCurrentFrameIndex() { return m_renderQueue->GetCurrentFramesCount() % FramesCount; }
	UINT64 GetCurrentFrameCount() { return m_renderQueue->GetCurrentFramesCount(); }

	//DescriptorHeap할당
	DescriptorHeapAllocation CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t count);
	DescriptorHeapAllocation GpuDynamicDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE heapType, size_t count);

	//DescriptorHeap에 View생성
	void CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc);
	void CreateSRV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc);
	void CreateRTV(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_RENDER_TARGET_VIEW_DESC* viewDesc);

	//사용되지않는 자원 해제
	void Update();

	void Render(const DescriptorHeapAllocation& passDescriptorHeapAllocation);

	//Render 항목 추가 추가된 아이템은 GPU처리가 끝날때까지 보관
	void PushRenderItem(std::shared_ptr<SceneObject> addItem);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	template<class T>
	std::unique_ptr<UploadBuffer<T>> CreateUploadBuffer(UINT elementCount, bool isConstantBuffer);

	void CopyDescriptorHeapAllocation(DescriptorHeapAllocation& source, DescriptorHeapAllocation& dest);

	//MainWindow에서 한번만 호출
	void CreateSwapChain(DXGI_SWAP_CHAIN_DESC& swapDesc);
	void ResizeSwapChain(int width, int height);
	void InitializeImGUID3D12();
private:
	D3DResourceManager();
	D3DResourceManager(const D3DResourceManager&) = delete;
	D3DResourceManager& operator=(const D3DResourceManager&) = delete;
	~D3DResourceManager()
	{
		m_staticItems.clear();
		m_copyQueue->FlushCommandQueue();
		m_renderQueue->FlushCommandQueue();
	}
private:
	void BuildDescriptorHeaps(ID3D12Device* device);

	//실패시 nullptr
	TextureResourcePair* GetTextureResourcePair(std::wstring textureName);

	bool InitDirect3D();

	//Log 함수들
	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	//사용되지않는 할당해제
	void UpdateStaleAllocations();

	void DrawRenderLists(D3D12_CPU_DESCRIPTOR_HANDLE backbufferView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, RenderType renderType, D3D12_GPU_DESCRIPTOR_HANDLE passDescriptorHandle);

	//D3D자원 생성
	void BuildPipelineState();
	void BuildShaders();
	void BuildBlendState();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

	//key -> ( 레퍼런스카운트 ,텍스처 GPU 리소스) 페어 
	std::unordered_map<std::wstring, TextureResourcePair> m_textureResourceMap;

	//cbv_srv_Uav , sampler,rtv ,dsv
	std::map<D3D12_DESCRIPTOR_HEAP_TYPE, CPUDescriptorHeap> m_cpuDescriptorHeapsMap;

	//cbv_srv_uav, sampler
	//Mesh 당(objCB*1 + MaterialCount*(materialCB + diffuse texture SRV + normal texture srv + ...)
	std::map<D3D12_DESCRIPTOR_HEAP_TYPE, GPUDescriptorHeap> m_gpuDescriptorHeapsMap;

	//Render할 아이템 보관함(FramesCount만큼)
	std::array<std::vector<std::shared_ptr<SceneObject>>, FramesCount> m_renderLists;

	//m_gpuDescriptorHeap의 static부분 
	std::map<std::wstring, DescriptorHeapAllocation> m_staticItems;

	//D3D12관련 리소스들
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;


	D3D12_BLEND_DESC m_blendDescs[(int)BlendType::Count];

	std::map<ShaderType, std::unique_ptr<ShaderObject>> m_shaderObjects;
	std::unordered_map <RenderType, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_pipelineStates;

	std::unique_ptr<RenderCommandQueueObject> m_renderQueue = nullptr;
	std::unique_ptr<CommandQueueObject> m_copyQueue = nullptr;

	std::unique_ptr<SwapChainObject> m_swapChain;

	static	const int ThreadCount = 4;
	std::vector<std::future<void>> m_renderFutures;
};


template<class T>
std::unique_ptr<UploadBuffer<T>> D3DResourceManager::CreateUploadBuffer(UINT elementCount, bool isConstantBuffer)
{
	return std::make_unique<UploadBuffer<T>>(m_device.Get(), elementCount, isConstantBuffer);
}