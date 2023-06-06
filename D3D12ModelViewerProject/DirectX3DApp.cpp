#include "DirectX3DApp.h"
#include "D3DResourceManager.h"
#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx12.h"
#include "imgui_filedialog.h"


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;



void DirectX3DApp::ImGUIInitialIze(HWND hwnd)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	D3DResourceManager::GetInstance().InitializeImGUID3D12();
	m_imguiInitialized = true;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DirectX3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}


DirectX3DApp::DirectX3DApp(HINSTANCE hInstance) : m_hInstance(hInstance)
{
	m_App = this;
}

DirectX3DApp::~DirectX3DApp()
{

}

bool DirectX3DApp::InitWindow()
{
	WNDCLASSEXW wcex;
	std::wstring wndClassName = _TEXT("MainWnd");

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_D3D12MODELVIEWERPROJECT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_D3D12MODELVIEWERPROJECT);
	wcex.lpszClassName = wndClassName.c_str();
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassExW(&wcex))
	{
		MessageBox(0, _TEXT("RegisterClassFailed"), 0, 0);
		return 0;
	}

	RECT rect = { 0,0,m_clientWidth,m_clientHeight };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, true);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	m_hwnd = CreateWindow(wndClassName.c_str(), m_wndTitle.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, m_hInstance, nullptr);

	if (!m_hwnd)
	{
		MessageBox(0, _TEXT("CreateWindowFailed"), 0, 0);
		return FALSE;
	}

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);

	return TRUE;
}

bool DirectX3DApp::InitDirect3D()
{
	CreateSwapChain();
	m_initialized = true;
	return true;
}

void DirectX3DApp::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = m_clientWidth;
	sd.BufferDesc.Height = m_clientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = m_backBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = m_hwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3DResourceManager::GetInstance().CreateSwapChain(sd);
}

void DirectX3DApp::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	++frameCnt;

	if ((GetMainTimer().TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = m_wndTitle + _TEXT("fps: ") + fpsStr + _TEXT("mspf: ") + mspfStr;

		SetWindowText(m_hwnd, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

bool DirectX3DApp::ImGuiWantCaptureMouse()
{
	if (m_imguiInitialized == false)
	{
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse == true)
	{
		return true;
	}

	return false;
}

void DirectX3DApp::OnResize()
{
	D3DResourceManager::GetInstance().ResizeSwapChain(m_clientWidth, m_clientHeight);
}

void DirectX3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
}

void DirectX3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
}

void DirectX3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
}

void DirectX3DApp::ImGUIFrameStarted()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

bool DirectX3DApp::Initialize()
{

	if (!InitWindow())
	{
		return false;
	}

	if (!InitDirect3D())
	{
		return false;
	}

	OnResize();
	ImGUIInitialIze(m_hwnd);
	return true;
}

DirectX3DApp* DirectX3DApp::m_App = nullptr;
DirectX3DApp* DirectX3DApp::GetApp()
{
	return m_App;
}

int DirectX3DApp::Run()
{
	MSG msg = { 0 };

	GameTimer& timer = GetMainTimer();
	timer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();

			if (!m_appPaused)
			{
				ImGUIFrameStarted();
				CalculateFrameStats();
				Update(timer);
				Draw(timer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

LRESULT DirectX3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return 0;
	}

	switch (msg)
	{
	case WM_ACTIVATE:
		return 0;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(m_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwnd, About);
			return 0;
		case IDM_EXIT:
			DestroyWindow(m_hwnd);
			return 0;
		default:
			return DefWindowProc(m_hwnd, msg, wParam, lParam);
		}
	}
	return 0;
	case WM_SIZE:
	{

		m_clientWidth = LOWORD(lParam);
		m_clientHeight = HIWORD(lParam);

		if (m_initialized == false)
		{
			return 0;
		}

		if (wParam == SIZE_MINIMIZED)
		{
			m_appPaused = true;
			m_minimized = true;
			m_maximized = false;
			return 0;
		}

		if (wParam == SIZE_MAXIMIZED)
		{
			m_appPaused = false;
			m_minimized = false;
			m_maximized = true;
			OnResize();
			return 0;
		}

		if (wParam == SIZE_RESTORED)
		{
			if (m_minimized)
			{
				m_appPaused = false;
				m_minimized = false;
				OnResize();
			}
			else if (m_maximized)
			{
				m_appPaused = false;
				m_maximized = false;
				OnResize();
			}
			else
			{
				OnResize();
			}
			return 0;
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}


