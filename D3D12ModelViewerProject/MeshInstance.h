#pragma once

#include "FbxUtil.h"
#include "AnimationCalculator.h"

class MeshObject;

class MeshInstance
{
public:
	MeshInstance(std::string name, std::weak_ptr<MeshObject> parent, Transform transform = Transform());
public:
	//dirtyCount감소
	void Update();

	int GetInstanceConstDirty() { return m_instanceConstDirty; }
	int GetAnimationDirty() { return m_animationDirty; }

	std::string GetName() { return m_name; }
	const InstanceConstants& GetInstanceConstants() const { return m_instanceConsts; }
	const Transform& GetTransform() { return m_transform; }
	void SetTransform(const Transform& transform);
	void SetAnimation(const AnimationClip& animation, bool bPlay = true, bool bLoop = true);
	void StopAnimation();
	void PauseAnimation() { m_isAnimationPlaying = false; };
	void PlayAnimation() { m_isAnimationPlaying = true; };
	const std::vector<InstanceAnimations>& GetInstanceAnimations() { return m_instanceAnimations; }
private:
	void UpdateInstanceConstants(MeshObject* parent);
	void UpdateInstanceAnimations(MeshObject* parent,float deltaTime);
	void DecreaseInstanceConstDirty();
	void DecreaseAnimationDirty();
private:
	std::string m_name;
	std::weak_ptr<MeshObject> m_parent;

	int m_instanceConstDirty = FramesCount + 1;
	int m_animationDirty = FramesCount + 1;

	InstanceConstants m_instanceConsts;

	//skeleton bone 개수만큼
	std::vector<InstanceAnimations> m_instanceAnimations;

	Transform m_transform;

	AnimationCalcuator m_animationCalculator;
	bool m_isAnimationPlaying = false;
};
