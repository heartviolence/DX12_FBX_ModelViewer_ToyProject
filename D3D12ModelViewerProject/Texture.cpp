
#include "Texture.h"
#include "D3DResourceManager.h"

using namespace std;

Texture::Texture(Texture&& rhs) :
	TextureResource(move(rhs.TextureResource)),
	CpuAllocationMap(move(CpuAllocationMap))
{
}

Texture& Texture::operator=(Texture&& rhs)
{
	TextureResource = move(rhs.TextureResource);
	CpuAllocationMap = move(CpuAllocationMap);
	return *this;
}

Texture::Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource) : TextureResource(resource)
{
}

Texture Texture::CreateRenderTargetTexture(int width, int height, DXGI_FORMAT format)
{
	D3D12_RESOURCE_DESC renderResourceDesc;
	ZeroMemory(&renderResourceDesc, sizeof(D3D12_RESOURCE_DESC));

	renderResourceDesc.MipLevels = 1;
	renderResourceDesc.DepthOrArraySize = 1;
	renderResourceDesc.Alignment = 0;
	renderResourceDesc.Format = format;
	renderResourceDesc.Width = width;
	renderResourceDesc.Height = height;
	renderResourceDesc.SampleDesc.Count = 1;
	renderResourceDesc.SampleDesc.Quality = 0;
	renderResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	renderResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	renderResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	return Texture(D3DResourceManager::GetInstance().CreateTexture(renderResourceDesc));
}

Texture Texture::CreateDepthStencilTexture(int width, int height, DXGI_FORMAT format)
{
	D3D12_RESOURCE_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D12_RESOURCE_DESC));

	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = format;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = format;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0.0f;

	return Texture(D3DResourceManager::GetInstance().CreateTexture(depthStencilDesc));
}

void Texture::CreateSRV(D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];

	allocation = D3DResourceManager::GetInstance().CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);
	D3DResourceManager::GetInstance().CreateSRV(TextureResource.Get(), allocation.GetCpuHandle(), srvDesc);
}

void Texture::CreateRTV(D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc)
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];

	allocation = D3DResourceManager::GetInstance().CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
	D3DResourceManager::GetInstance().CreateRTV(TextureResource.Get(), allocation.GetCpuHandle(), rtvDesc);
}

void Texture::CreateDSV(D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc)
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];

	allocation = D3DResourceManager::GetInstance().CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	D3DResourceManager::GetInstance().CreateDSV(TextureResource.Get(), allocation.GetCpuHandle(), dsvDesc);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetSRV()
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	return allocation.GetCpuHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRTV()
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	return allocation.GetCpuHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDSV()
{
	auto& allocation = CpuAllocationMap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];
	return allocation.GetCpuHandle();
}


void RenderTargetTexture::BuildTexture(int width, int height, DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT)
{
	RenderTarget = Texture::CreateDepthStencilTexture(width, height, RenderTargetFormat);
	DepthStencil = Texture::CreateDepthStencilTexture(width, height, DepthStencilFormat);

	//rebuild viewport,scissorRect
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = static_cast<float>(width);
	Viewport.Height = static_cast<float>(height);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	ScissorRect = { 0,0,width,height };
}
