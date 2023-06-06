#pragma once

#include "DirectX3DApp.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "Camera.h"
#include "FrameResource.h"
#include "FbxModelScene.h"
#include "ImGUIControl.h"
#include <functional>
#include "JobQueue.h"
#include "ImportListControl.h"
#include "SceneHierachyControl.h"
#include "LightsInfoControl.h"

class D3DModelViewerApp : public DirectX3DApp
{
public:
	D3DModelViewerApp(HINSTANCE hInstance);
	D3DModelViewerApp(const D3DModelViewerApp& rhs) = delete;
	D3DModelViewerApp& operator=(const D3DModelViewerApp& rhs) = delete;
	virtual ~D3DModelViewerApp();

	virtual bool Initialize() override;
private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)   override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	virtual void ImGUIFrameStarted() override;
	void BuildGrid();
	void OnKeyboardInput(int frameIndex);
	void UpdateMainPassCB(int frameIndex);
	void UpdateLightBuffer(int frameIndex);

	void ReAllocateLightsBuffer(int frameIndex);
	void ReAllocateDescriptorHeap(int frameIndex);

private:
	PassConstants m_mainPassCB;
	std::wstring m_mainPassName = _TEXT("Main");

	Camera m_camera;
	//key -> pass Constant buffer( framesCount만큼 통째로 할당)
	std::unordered_map<std::wstring, std::unique_ptr<UploadBuffer<PassConstants>>> m_passCBMap;

	std::vector<std::unique_ptr<UploadBuffer<Light>>> m_lightsBuffers;

	std::shared_ptr<Lights> m_lights;

	std::array<DescriptorHeapAllocation, FramesCount> m_cpuAllocation;
	std::array<DescriptorHeapAllocation, FramesCount> m_gpuAllocation;
	int m_heapAllocationDirty = FramesCount;

	int m_passDescriptorCount = 2;

	POINT m_lastMousePos;

	std::shared_ptr<Scene> m_mainScene;
	std::shared_ptr<std::map<std::string, std::shared_ptr<FbxModelScene>>> m_fbxModels;
	std::shared_ptr<std::map<std::string, std::shared_ptr<MeshResources>>> m_meshResources;
	std::shared_ptr<std::map<std::string, AnimationClip>> m_animations;

	ImportListControl m_importListControl;
	SceneHierachyControl m_sceneHierachyControl;
	LightsInfoControl m_lightsInfoControl;
};
