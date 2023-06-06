
#include "AnimationCalculator.h"
using namespace DirectX;

void AnimationCalcuator::CopyAnimation(std::vector<InstanceAnimations>& instanceAnimations, const Skeleton& skeleton)
{
	

	//BoneAnimations
	std::vector<XMMATRIX> boneTransforms;

	m_animation.Interpolate(m_currentTime, boneTransforms);

	int animationIndex = 0;
	for (animationIndex = 0; animationIndex < boneTransforms.size() && animationIndex < instanceAnimations.size(); ++animationIndex)
	{
		InstanceAnimations& copyDest = instanceAnimations.at(animationIndex);
		XMStoreFloat4x4(&copyDest.BoneTransform, XMMatrixTranspose(boneTransforms.at(animationIndex)));
	}
	for (; animationIndex < instanceAnimations.size(); ++animationIndex)
	{
		InstanceAnimations& copyDest = instanceAnimations.at(animationIndex);
		XMStoreFloat4x4(&copyDest.BoneTransform, XMMatrixTranspose(XMMatrixIdentity()));
	}

}

void AnimationCalcuator::AddTime(float time)
{
	m_currentTime += time;
	if (m_currentTime >= m_endTime)
	{
		if (m_isAnimationLoop == true)
		{
			int times = m_currentTime / m_endTime;
			m_currentTime -= m_endTime * times;
		}
		else
		{
			m_currentTime = m_endTime;
		}
	}
}
