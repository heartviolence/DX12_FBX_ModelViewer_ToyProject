#pragma once

#include "D3DUtil.h"
#include "GameTimer.h"

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

class DirectX3DApp
{
protected:
	DirectX3DApp(HINSTANCE hInstance);
	DirectX3DApp(const DirectX3DApp& rhs) = delete;
	DirectX3DApp& operator=(const DirectX3DApp& rhs) = delete;
	virtual ~DirectX3DApp();

protected:
	bool InitWindow();
	bool InitDirect3D();
	void CreateSwapChain();

	void CalculateFrameStats();
	bool ImGuiWantCaptureMouse();
	void ImGUIInitialIze(HWND hwnd);
protected:
	virtual void OnResize();
	virtual void Update(const GameTimer& gt) = 0;
	virtual void Draw(const GameTimer& gt) = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void ImGUIFrameStarted();

public:
	virtual bool Initialize();
	static DirectX3DApp* GetApp();
	inline float AspectRatio() const
	{
		return static_cast<float>(m_clientWidth) / m_clientHeight;
	}
	int Run();
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
protected:
	static DirectX3DApp* m_App;

	HINSTANCE m_hInstance = nullptr;
	HWND m_hwnd = nullptr;

	bool m_appPaused = false;
	bool m_minimized = false;
	bool m_maximized = false;
	bool m_initialized = false;

	int m_clientWidth = 1200;
	int m_clientHeight = 800;


	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;


	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static const UINT SwapChainBufferCount = FramesCount;


	std::wstring m_wndTitle = _TEXT("D3DModelViewer");
	bool m_imguiInitialized = false;
};