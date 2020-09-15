#pragma once
#include <../Utility/MathInterface/MathInterface.hpp>
#include <vector>
#include <memory>

namespace SimpleLight {

	enum LightSourceType {
		LIGHT_SOURCE_TYPE_POINT,
		LIGHT_SOURCE_TYPE_DIRECTION,
		LIGHT_SOURCE_TYPE_SPOT
	};
	
	/** 光源属性数据的缓冲结构，对应的HLSL代码应该是
	 * cbuffer LightDataBuffer {
	 *		float4 position_flag1;
	 *		float4 radiance_power;
	 *		float4 direction_flag2;
	 * }
	 * 不同类型的光源对上述数据的利用方式可能会不一样，特别是flag1和flag2
	 * TODO: 暂不清楚GLSL的配置方式
	 */
	struct LightDataBufferStructure {
		IMath::IFLOAT4 position_flag1;
		IMath::IFLOAT4 radiance_power;
		IMath::IFLOAT4 direction_flag2;
	};

	class ILight {
	public:
		ILight(IMath::IFLOAT3 position, IMath::IFLOAT3 direction, IMath::IFLOAT3 radiance, float power) :
			m_radiance(radiance), m_power(power), m_position(position), m_direction(direction) {}
		virtual ~ILight() = default;
		virtual LightSourceType Type() const = 0;
		IMath::IFLOAT3 Radiance() const { return m_radiance; }
		virtual IMath::IFLOAT3& Radiance() = 0;
		float Power() const { return m_power; }
		virtual float& Power() = 0;
		IMath::IFLOAT3 Position() const { return m_position; }
		virtual IMath::IFLOAT3& Position() = 0;
		virtual const std::vector<uint8_t>& LightPropertyBuffer() = 0;
	protected:
		IMath::IFLOAT3 m_radiance;
		IMath::IFLOAT3 m_position;
		IMath::IFLOAT3 m_direction;
		float m_power;
	};

	class IPointLight : public ILight {
	public:
		static std::unique_ptr<IPointLight> Create(IMath::IFLOAT3 position, IMath::IFLOAT3 radiance, float power);
	public:
		IPointLight(IMath::IFLOAT3 position, IMath::IFLOAT3 radiance, float power) : ILight(position, {0.0f, 0.0f, 0.0f}, radiance, power) {}
		virtual ~IPointLight() = default;
		LightSourceType Type() const { return LIGHT_SOURCE_TYPE_POINT; }
	protected:
	};

	class IDirectionLight : public ILight {
	public:
		static std::unique_ptr<IDirectionLight> Create(IMath::IFLOAT3 position, IMath::IFLOAT3 direction, IMath::IFLOAT3 radiance, float power);
	public:
		IDirectionLight(IMath::IFLOAT3 position, IMath::IFLOAT3 direction, IMath::IFLOAT3 radiance, float power) : ILight(position, direction, radiance, power) {}
		virtual ~IDirectionLight() = default;
		LightSourceType Type() const { return LIGHT_SOURCE_TYPE_DIRECTION; }
	};

}
