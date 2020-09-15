#include "SimpleLight.h"

using namespace SimpleLight;
using namespace IMath;

class PointLight_Impl : public IPointLight {
public:
	PointLight_Impl(IFLOAT3 position, IFLOAT3 radiance, float power) : IPointLight(position, radiance, power),
		m_isPositionChanged(true), m_isPowerChanged(true), m_isRadianceChanged(true) {
		m_data = std::vector<uint8_t>(sizeof(LightDataBufferStructure));
	}
	virtual ~PointLight_Impl() = default;
	virtual IMath::IFLOAT3& Radiance() override final {
		m_isRadianceChanged = true;
		return m_radiance;
	};
	virtual IMath::IFLOAT3& Position() override final {
		m_isPositionChanged = true;
		return m_position;
	}
	virtual float& Power() override final {
		m_isPowerChanged = true;
		return m_power;
	}
	virtual const std::vector<uint8_t>& LightPropertyBuffer() override final {
		if (m_isPowerChanged || m_isPositionChanged || m_isRadianceChanged) {
			LightDataBufferStructure data;
			data.direction_flag2 = {}; /** 对于点光源而言这个选项没有意义 */
			data.position_flag1 = { m_position.x, m_position.y, m_position.z, 0.0f };
			data.radiance_power = { m_radiance.x, m_radiance.y, m_radiance.z, m_power };
			memcpy(m_data.data(), &data, sizeof(LightDataBufferStructure));
			m_isPositionChanged = false;
			m_isPowerChanged = false;
			m_isRadianceChanged = false;
		}
		return m_data;
	}
private:
	std::vector<uint8_t> m_data;
	bool m_isPowerChanged;
	bool m_isPositionChanged;
	bool m_isRadianceChanged;
};

std::unique_ptr<IPointLight> IPointLight::Create(IFLOAT3 position, IFLOAT3 radiance, float power) {
	return std::make_unique<PointLight_Impl>(position, radiance, power);
}

class DirectionLight_impl : public IDirectionLight {
public:
	DirectionLight_impl(IFLOAT3 position, IFLOAT3 direction, IFLOAT3 radiance, float power) 
		: m_isPowerChanged(true), m_isPositionChanged(true), m_isDirectionChanged(true), m_isRadianceChanged(true),
		IDirectionLight(position, direction, radiance, power) {
		m_data = std::vector<uint8_t>(sizeof(LightDataBufferStructure));
	}
	virtual ~DirectionLight_impl() = default;
	virtual IMath::IFLOAT3& Radiance() override final {
		m_isRadianceChanged = true;
		return m_radiance;
	};
	virtual IMath::IFLOAT3& Position() override final {
		m_isPositionChanged = true;
		return m_position;
	}
	virtual float& Power() override final {
		m_isPowerChanged = true;
		return m_power;
	}
	virtual const std::vector<uint8_t>& LightPropertyBuffer() override final {
		if (m_isPowerChanged || m_isPositionChanged || m_isRadianceChanged || m_isDirectionChanged) {
			LightDataBufferStructure data;
			data.direction_flag2 = {m_direction.x, m_direction.y, m_direction.z, 0.0f };
			data.position_flag1 = { m_position.x, m_position.y, m_position.z, 0.0f };
			data.radiance_power = { m_radiance.x, m_radiance.y, m_radiance.z, m_power };
			memcpy(m_data.data(), &data, sizeof(LightDataBufferStructure));
			m_isPositionChanged = false;
			m_isPowerChanged = false;
			m_isRadianceChanged = false;
		}
		return m_data;
	}
private:
	std::vector<uint8_t> m_data;
	bool m_isPowerChanged;
	bool m_isPositionChanged;
	bool m_isRadianceChanged;
	bool m_isDirectionChanged;
};

std::unique_ptr<IDirectionLight> IDirectionLight::Create(IFLOAT3 position, IFLOAT3 direction, IFLOAT3 radiance, float power) {
	return std::make_unique<DirectionLight_impl>(position, direction, radiance, power);
}
