#pragma once

#include "D3DUtil.h"

struct ShaderObject
{
	std::wstring VSFileName;
	Microsoft::WRL::ComPtr<ID3DBlob> VS;

	std::wstring PSFileName;
	Microsoft::WRL::ComPtr<ID3DBlob> PS;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature = nullptr;

	void Compile()
	{
		VS = D3DUtil::CompileShader(VSFileName, nullptr, "VS", "vs_5_1");
		PS = D3DUtil::CompileShader(PSFileName, nullptr, "PS", "ps_5_1");
	}
};


