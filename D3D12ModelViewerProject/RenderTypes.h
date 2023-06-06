#pragma once

#include <functional>

enum class BlendType : int
{
	Opaque = 0,
	Transparent,
	Count
};

enum class ShaderType : int
{
	StaticMeshShader = 0,
	DynamicMeshShader,
	Count
};

struct RenderType
{
	ShaderType ShaderMode;
	BlendType BlendMode;

	bool operator==(const RenderType& rhs) const
	{
		return (ShaderMode == rhs.ShaderMode &&
			BlendMode == rhs.BlendMode);
	}
};

struct RenderTypes
{
	static RenderType GetRenderType(ShaderType shaderType, BlendType blendType)
	{
		return RenderType{
			shaderType,
			blendType
		};
	}
};

namespace std
{
	template<>
	struct hash<RenderType>
	{
		std::size_t operator()(const RenderType& renderType) const
		{
			return(hash<int>()(static_cast<int>(renderType.BlendMode)) ^
				hash<int>()(static_cast<int>(renderType.ShaderMode)));
		}
	};
}
