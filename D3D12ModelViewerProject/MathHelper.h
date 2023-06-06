#pragma once


#include <DirectXMath.h>
#include <cstdint>
#include <stdlib.h>

class MathHelper
{
public:
	static DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::FXMMATRIX M)
	{
		DirectX::XMMATRIX A = M;
		A.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, A));
	}

	static DirectX::XMFLOAT4X4 Identity4x4()
	{
		static DirectX::XMFLOAT4X4 Id(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		return Id;
	}

	static DirectX::XMFLOAT3X3 Identity3x3()
	{
		static DirectX::XMFLOAT3X3 Id(
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f
		);
		return Id;
	}

	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x);
	}

	static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
	{
		return DirectX::XMVectorSet(
			radius * sinf(phi) * cosf(theta),
			radius * cosf(phi),
			radius * sinf(phi) * sinf(theta),
			1.0f);
	}

	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	static float Lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	static const float Infinity;
	static const float Pi;
};