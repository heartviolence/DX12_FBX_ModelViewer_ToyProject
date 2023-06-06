#pragma once

#include "FbxUtil.h"
#include "FrameResource.h"

class AnimationCalcuator
{
public:
	AnimationCalcuator() = default;

	void CopyAnimation(std::vector<InstanceAnimations>& instanceAnimations, const Skeleton& skeleton);	

	void SetLoop(bool loopMode)
	{
		m_isAnimationLoop = loopMode;
	}

	void SetAnimationClip(const AnimationClip& animation)
	{
		m_animation = animation;
		m_endTime = animation.GetEndTime();
		m_currentTime = 0.0f;
		m_isAnimationLoop = false;
	}

	void Stop()
	{
		m_currentTime = 0.0f;
	}

	void AddTime(float time);
	

	const AnimationClip& GetCurrentAnimation() { return m_animation; }

private:

	AnimationClip m_animation;
	float m_currentTime = 0.0f;
	float m_endTime = 0.0f;
	bool m_isAnimationLoop = false;
};