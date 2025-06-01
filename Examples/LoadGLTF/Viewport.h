#pragma once

#include <MathInterface/MathInterface.hpp>
#include <UVType.h>
#include "TShaderParameterSlot.h"

#include <memory>

namespace UnknownVision {
	class GPUBuffer;
	class RenderDevice;
	class CommandUnit;
};

/** 
 * @breif 视口对象，记录视口的大小，深度，变换信息 
 */
class Viewport {
public:
	struct ShaderConstantBuffer {
		glm::vec4 position;
		glm::mat4x4 viewMat;
		glm::mat4x4 projMat;
		bool operator==(const ShaderConstantBuffer& other) const {
			return position == other.position && viewMat == other.viewMat && projMat == other.projMat;
		}
		bool operator!=(const ShaderConstantBuffer& other) const {
			return !(*this == other);
		}
	};

	template<size_t BindingSlot = 0>
	class ShaderParameter : public TShaderParameterSlot<ShaderParameter<BindingSlot>> {
	public:
		using SlotDataType = ShaderConstantBuffer;
		constexpr static size_t GetBindingSlotIndex(size_t offset = 0) noexcept {
			return offset + BindingSlot; // 默认绑定到b0
		}
		static constexpr const char* GetShaderCodePlacementString() noexcept { return "/* #ViewportData0# */"; }
		static const char* GetShaderCode() {
			static const std::string code = "cbuffer ViewportBuffer : register(b" + std::to_string(BindingSlot) + ")\n"
				+ R"(
{
    ViewportDataStructure ViewportData;
};
)";
			return code.c_str();
		}
		static std::vector<UnknownVision::ShaderParameterSlotDesc> GetSlotDesc() { return 
			{ UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(BindingSlot, 1) };
		}
	};
public:
	Viewport() = default;
public:
	bool Init(const UnknownVision::ViewportDesc& vp, const UnknownVision::ScissorRectDesc& sr
		, UnknownVision::RenderDevice* renderDevice);

	bool Update(const ShaderConstantBuffer& newData
		, UnknownVision::CommandUnit* cmdUnit
		, UnknownVision::RenderDevice* renderDevice);

	const UnknownVision::GPUBuffer* GetGPUBuffer() const { return m_GPUConstantBuffer.get(); }
	const UnknownVision::ViewportDesc& GetViewportDesc() const { return m_vp; }
	const UnknownVision::ScissorRectDesc& GetScissorRectDesc() const { return m_sr; }
private:
	ShaderConstantBuffer m_CPUConstantBuffer; /**< 视口数据的常量缓冲结构 */
	std::unique_ptr<UnknownVision::GPUBuffer> m_GPUConstantBuffer; /**< 视口数据的常量缓冲 */
	UnknownVision::ViewportDesc m_vp;
	UnknownVision::ScissorRectDesc m_sr;
};