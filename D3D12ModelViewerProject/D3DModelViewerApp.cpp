#include "D3DModelViewerApp.h"
#include <stdexcept>
#include "framework.h"
#include "GeometryGenerator.h"
#include "ResourceUploadBatch.h"
#include "D3DResourceManager.h"
#include "FbxUtil.h"
#include <filesystem>
#include <iostream>
#include "FileDialog.h"
#include "ThreadManager.h"
#include <KnownFolders.h>
#include <shlobj.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

using namespace std;


static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
{
	LPWSTR programFilesPath = nullptr;
	SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

	std::filesystem::path pixInstallationPath = programFilesPath;
	pixInstallationPath /= "Microsoft PIX";

	std::wstring newestVersionFound;

	for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
	{
		if (directory_entry.is_directory())
		{
			if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
			{
				newestVersionFound = directory_entry.path().filename().c_str();
			}
		}
	}

	if (newestVersionFound.empty())
	{
		// TODO: Error, no PIX installation found
	}

	return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}


D3DModelViewerApp::D3DModelViewerApp(HINSTANCE hInstance)
	:DirectX3DApp(hInstance),
	m_mainScene(make_shared<Scene>("mainScene")),
	m_fbxModels(make_shared<map<std::string, std::shared_ptr<FbxModelScene>>>()),
	m_meshResources(make_shared<std::map<std::string, std::shared_ptr<MeshResources>>>()),
	m_animations(make_shared<std::map<std::string, AnimationClip>>()),
	m_lights(make_shared<Lights>())
{

}

D3DModelViewerApp::~D3DModelViewerApp()
{

}

bool D3DModelViewerApp::Initialize()
{
	if (!DirectX3DApp::Initialize())
	{
		return false;
	}
	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();

	m_lightsBuffers.resize(FramesCount);

	//initialize Controls
	ImportListModel importListViewModel;
	importListViewModel.CurrentScene = m_mainScene;
	importListViewModel.Animations = m_animations;
	importListViewModel.FbxModels = m_fbxModels;
	importListViewModel.MeshResources = m_meshResources;

	m_importListControl.Initialize(importListViewModel);

	SceneHierachyModel sceneHierachyModel;
	sceneHierachyModel.scene = m_mainScene;
	sceneHierachyModel.Animations = m_animations;

	m_sceneHierachyControl.Initialize(sceneHierachyModel);

	LightsInfoModel lightInfoModel;
	lightInfoModel.Lights = m_lights;

	m_lightsInfoControl.Initialize(lightInfoModel);

	//Create Resources
	m_passCBMap[m_mainPassName] = resourceManager.CreateUploadBuffer<PassConstants>(FramesCount, true);

	for (int i = 0; i < FramesCount; ++i)
	{
		m_lightsBuffers.at(i) = D3DResourceManager::GetInstance().CreateUploadBuffer<Light>(10, false);
		ReAllocateDescriptorHeap(i);
	}


	m_camera.SetPosition(0.0f, 1.0f, -5.0f);

	DirectionalLight light;
	light.Direction = { 0.5f,-0.5f,0.0f };
	light.Color = { 0.3f,0.3f,0.3f };

	m_lights->DirectionalLights.push_back(light);

	BuildGrid();

	return true;
}

void D3DModelViewerApp::OnResize()
{
	DirectX3DApp::OnResize();

	m_camera.SetLens(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
}


void D3DModelViewerApp::Update(const GameTimer& gt)
{
	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();
	resourceManager.Update();

	int frameIndex = resourceManager.GetCurrentFrameIndex();
	OnKeyboardInput(frameIndex);

	m_importListControl.Update();
	m_sceneHierachyControl.Update();
	m_mainScene->Update();

	ReAllocateLightsBuffer(frameIndex);

	if (m_heapAllocationDirty > 0)
	{
		ReAllocateDescriptorHeap(frameIndex);
	}

	UpdateLightBuffer(frameIndex);
	UpdateMainPassCB(frameIndex);
}

void D3DModelViewerApp::Draw(const GameTimer& gt)
{
	m_mainScene->PrepareRender();

	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();
	resourceManager.Render(m_gpuAllocation.at(resourceManager.GetCurrentFrameIndex()));
}

void D3DModelViewerApp::OnKeyboardInput(int frameIndex)
{
	GameTimer& gt = GetMainTimer();

	const float dt = gt.DeltaTime();
	static float moveSpeed = 10.0f;
	const float moveDistance = moveSpeed * dt;

	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_camera.Walk(moveDistance);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_camera.Walk(-moveDistance);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		m_camera.Strafe(-moveDistance);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		m_camera.Strafe(moveDistance);
	}

	if (GetAsyncKeyState('K') & 0x8000)
	{
		moveSpeed += 100.0f * gt.DeltaTime();
	}
	if (GetAsyncKeyState('J') & 0x8000)
	{
		moveSpeed -= 100.0f * gt.DeltaTime();
	}

	if (moveSpeed < 0)
	{
		moveSpeed = 0;
	}

	m_camera.UpdateViewMatrix();
}

void D3DModelViewerApp::UpdateMainPassCB(int frameIndex)
{
	GameTimer& gt = GetMainTimer();

	XMMATRIX view = m_camera.GetView();
	XMMATRIX proj = m_camera.GetProj();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMMATRIX invView = XMMatrixInverse(nullptr, view);
	XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

	XMStoreFloat4x4(&m_mainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_mainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_mainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_mainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_mainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_mainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	m_mainPassCB.EyePosW = m_camera.GetPosition3f();
	m_mainPassCB.RenderTargetSize = XMFLOAT2((float)m_clientWidth, (float)m_clientHeight);
	m_mainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_clientWidth, 1.0f / m_clientHeight);
	m_mainPassCB.NearZ = m_camera.GetNearZ();
	m_mainPassCB.FarZ = m_camera.GetFarZ();
	m_mainPassCB.TotalTime = gt.TotalTime();
	m_mainPassCB.DeltaTime = gt.DeltaTime();

	m_mainPassCB.DirectionalLightCount = m_lights->DirectionalLights.size();
	m_mainPassCB.PointLightCount = m_lights->PointLights.size();

	m_passCBMap[m_mainPassName]->CopyData(frameIndex, m_mainPassCB);
}

void D3DModelViewerApp::UpdateLightBuffer(int frameIndex)
{
	std::vector<Light> lightsToCopy;
	lightsToCopy.reserve(m_lights->Count());

	for (int i = 0; i < m_lights->DirectionalLights.size(); ++i)
	{
		lightsToCopy.push_back(m_lights->DirectionalLights.at(i).ToLight());
	}
	for (int i = 0; i < m_lights->PointLights.size(); ++i)
	{
		lightsToCopy.push_back(m_lights->PointLights.at(i).ToLight());
	}

	m_lightsBuffers.at(frameIndex)->CopyData(0, lightsToCopy.data(), lightsToCopy.size());
}

void D3DModelViewerApp::ReAllocateLightsBuffer(int frameIndex)
{
	size_t currentBufferSize = m_lightsBuffers.at(frameIndex)->GetMaxElementCount();
	if (currentBufferSize < m_lights->Count())
	{
		const int increaseStep = 5;
		m_lightsBuffers.at(frameIndex) = D3DResourceManager::GetInstance().CreateUploadBuffer<Light>(currentBufferSize + increaseStep, false);
		m_heapAllocationDirty = FramesCount;
	}
}

void D3DModelViewerApp::ReAllocateDescriptorHeap(int frameIndex)
{
	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();

	m_cpuAllocation.at(frameIndex) = resourceManager.CpuDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_passDescriptorCount);
	m_gpuAllocation.at(frameIndex) = resourceManager.GpuDynamicDescriptorHeapAlloc(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_passDescriptorCount);

	auto& cpuAllocation = m_cpuAllocation.at(frameIndex);

	auto& mainPassBuffer = m_passCBMap[m_mainPassName];

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
	desc.BufferLocation = mainPassBuffer->Resource()->GetGPUVirtualAddress() + (mainPassBuffer->GetElementByteSize() * frameIndex);
	desc.SizeInBytes = mainPassBuffer->GetElementByteSize();

	resourceManager.CreateCBV(cpuAllocation.GetCpuHandle(0), desc);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_lightsBuffers.at(frameIndex)->GetMaxElementCount();
	srvDesc.Buffer.StructureByteStride = m_lightsBuffers.at(frameIndex)->GetElementByteSize();

	resourceManager.CreateSRV(m_lightsBuffers.at(frameIndex)->Resource(), cpuAllocation.GetCpuHandle(1), srvDesc);

	resourceManager.CopyDescriptorHeapAllocation(cpuAllocation, m_gpuAllocation.at(frameIndex));
	m_heapAllocationDirty--;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//D3D12device 생성전에 로드되어야함
	if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
	{
		LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
	}

	try
	{
		D3DModelViewerApp ViewerApp(hInstance);
		if (!ViewerApp.Initialize())
		{
			return 0;
		}

		return ViewerApp.Run();
	}
	catch (std::exception e)
	{
		std::string msg = e.what();
		std::wstring wMsg;
		wMsg.assign(msg.begin(), msg.end());
		MessageBox(nullptr, wMsg.c_str(), L"error", MB_OK);
		return 0;
	}

	return 0;
}

void D3DModelViewerApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	if (ImGuiWantCaptureMouse() == true)
	{
		return;
	}
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_hwnd);
}

void D3DModelViewerApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	if (ImGuiWantCaptureMouse() == true)
	{
		return;
	}
	ReleaseCapture();
}

void D3DModelViewerApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (ImGuiWantCaptureMouse() == true)
	{
		return;
	}

	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

		m_camera.Pitch(dy);
		m_camera.RotateY(dx);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void D3DModelViewerApp::ImGUIFrameStarted()
{
	DirectX3DApp::ImGUIFrameStarted();

	m_importListControl.Show();
	m_sceneHierachyControl.Show();
	m_lightsInfoControl.Show();
}

void D3DModelViewerApp::BuildGrid()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(500.0f, 500.0f, 50, 50);

	MeshResourcesInfo meshInfo;

	auto transformFunc = [](GeometryGenerator::Vertex v) {
		return Vertex{
			v.Position,
			v.Normal,
			v.TexC
		};
	};

	std::transform(grid.Vertices.begin(), grid.Vertices.end(), std::back_inserter(meshInfo.VertexTable), transformFunc);
	meshInfo.SubMeshes.emplace("grid", grid.Indices32);

	PBRMaterial material;
	material.Name = "grid";
	material.Albedo = { 0.6f, 0.6f, 0.6f ,1.0f };
	meshInfo.Materials.push_back(material);

	MeshResources meshResource(meshInfo);
	std::shared_ptr<MeshObject> gridObject = meshResource.CreateMeshObject("grid");
	gridObject->CreateMeshInstance();

	m_mainScene->GetRootSceneObject()->AddChild(gridObject);
}


