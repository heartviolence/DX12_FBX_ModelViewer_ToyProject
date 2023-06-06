#pragma once

#include "ImGUIControl.h"
#include "D3DUtil.h"

struct LightsInfoModel
{
	std::weak_ptr<Lights> Lights;
};

class LightsInfoControl
{
public:
	void Show();
	void Initialize(LightsInfoModel model) { m_model = model; }
private:
	LightsInfoModel m_model;


};