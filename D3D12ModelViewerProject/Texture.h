#pragma once
#include "D3DUtil.h"
#include "DescriptorHeapManager.h" 

struct Texture
{
	Texture() = default;
	Texture(Texture&& rhs) noexcept;
	Texture& operator=(Texture&& rhs) noexcept;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
public:
	static Texture CreateRenderTargetTexture(int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
	static Texture CreateDepthStencilTexture(int width, int height, DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT);

public:
	D3D12_RESOURCE_DESC GetTextureResourceDesc()
	{
		return TextureResource->GetDesc();
	}

	DXGI_FORMAT GetTextureFormat()
	{
		return TextureResource->GetDesc().Format;
	}

	ID3D12Resource* Resource()
	{
		return TextureResource.Get();
	}

	void CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc);
	void CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc);
	void CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV();
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource;
	std::map< D3D12_DESCRIPTOR_HEAP_TYPE, DescriptorHeapAllocation> CpuAllocationMap;
};



struct RenderTargetTexture
{
	Texture RenderTarget;
	Texture DepthStencil;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;

	void BuildTexture(int width, int height, DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);
};