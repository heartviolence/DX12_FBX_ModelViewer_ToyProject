
#include "MeshInstance.h"
#include "GameTimer.h"
#include "MeshObject.h"

using namespace std;
using namespace DirectX;

MeshInstance::MeshInstance(std::string name, std::weak_ptr<MeshObject> parent, Transform transform)
	:m_name(name),
	m_parent(parent),
	m_transform(transform)
{
	if (auto parentMeshObject = parent.lock())
	{
		const size_t numOfBones = parentMeshObject->GetSkeleton().Joints.size();
		m_instanceAnimations.resize(numOfBones);
	}
}

void MeshInstance::Update()
{
	GameTimer timer = GetMainTimer();
	auto parent = m_parent.lock();

	UpdateInstanceConstants(parent.get());
	UpdateInstanceAnimations(parent.get(), timer.DeltaTime());
}

void MeshInstance::SetTransform(const Transform& transform)
{
	m_transform = transform;
	m_instanceConstDirty = FramesCount + 1;
}

void MeshInstance::SetAnimation(const AnimationClip& animation, bool bPlay, bool bLoop)
{
	if (auto parent = m_parent.lock())
	{
		const Skeleton& skeleton = parent->GetSkeleton();

		if (animation.BoneAnimations.size() == skeleton.Joints.size())
		{
			m_animationCalculator.SetAnimationClip(animation);
			m_animationCalculator.SetLoop(bLoop);
			m_isAnimationPlaying = bPlay;
			m_animationDirty = FramesCount + 1;
		}
	}
}

void MeshInstance::StopAnimation()
{
	m_isAnimationPlaying = false;
	m_animationDirty = FramesCount + 1;
	m_animationCalculator.SetAnimationClip(AnimationClip());
}

void MeshInstance::UpdateInstanceConstants(MeshObject* parent)
{
	DecreaseInstanceConstDirty();

	XMMATRIX finalTransformMatrix = m_transform.GetFinalTransformMatrix();

	m_instanceConsts.numOfBones = parent->GetSkeleton().Joints.size();

	XMStoreFloat4x4(&m_instanceConsts.Transform, XMMatrixTranspose(finalTransformMatrix));
}

void MeshInstance::UpdateInstanceAnimations(MeshObject* parent, float deltaTime)
{
	if (m_isAnimationPlaying == true)
	{
		m_animationCalculator.AddTime(deltaTime);
		m_animationDirty = FramesCount + 1;
	}

	if (m_animationDirty == FramesCount + 1)
	{
		m_animationCalculator.CopyAnimation(m_instanceAnimations, parent->GetSkeleton());
	}

	DecreaseAnimationDirty();
}

void MeshInstance::DecreaseInstanceConstDirty()
{
	if (m_instanceConstDirty > 0)
	{
		m_instanceConstDirty--;
	}
}

void MeshInstance::DecreaseAnimationDirty()
{
	if (m_animationDirty > 0)
	{
		m_animationDirty--;
	}
}
