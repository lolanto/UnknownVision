﻿#pragma once
#include "DX12Config.h"
#include "../RenderSystem/FixedStage.h"
#include "../UVType.h"
#include "../Utility/InfoLog/InfoLog.h"
#include <cassert>
#include <optional>

BEG_NAME_SPACE
/** Helping Functions */

inline DXGI_FORMAT ElementFormatToDXGIFormat(ElementFormatType format) {
	switch (format) {
	case ELEMENT_FORMAT_TYPE_R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	default:
		MLOG("Invalid format conervsion!\n");
		return DXGI_FORMAT_UNKNOWN;
	}
}

inline const char* VertexAttributeTypeToString(VertexAttributeType type) {
	switch (type)
	{
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION:
		return "POSITION";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL:
		return "NORMAL";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TANGENT:
		return "TANGENT";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE:
		return "TEXCOORD";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_COLOR:
		return "COLOR";
	default:
		MLOG("Invalid Vertex Attribute type!\n");
		return nullptr;
	}
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTypeToPrimitiveTopologyType(PrimitiveType type) {
	switch (type)
	{
	case UnknownVision::PRIMITIVE_TYPE_POINT:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case UnknownVision::PRIMITIVE_TYPE_TRIANGLE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default:
		FLOG("%s: Doesn't support this primitive type\n", __FUNCTION__);
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}

inline D3D12_FILL_MODE FillModeToDX12FillMode(FillMode mode) {
	switch (mode)
	{
	case UnknownVision::FILL_MODE_SOLID:
		return D3D12_FILL_MODE_SOLID;
	case UnknownVision::FILL_MODE_WIREFRAME:
		return D3D12_FILL_MODE_WIREFRAME;
	default:
		FLOG("%s: New DX Fill mode!\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_CULL_MODE CullModeToCullMode(CullMode mode) {
	switch (mode)
	{
	case UnknownVision::CULL_MODE_BACK:
		return D3D12_CULL_MODE_BACK;
	case UnknownVision::CULL_MODE_FRONT:
		return D3D12_CULL_MODE_FRONT;
	case UnknownVision::CULL_MODE_NONE:
		return D3D12_CULL_MODE_NONE;
	default:
		FLOG("%s: New DX cull mode!\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_HEAP_TYPE ResourceStatusToHeapType(const ResourceStatus& status) {
	if (status.isStably()) return D3D12_HEAP_TYPE_DEFAULT;
	else if (status.isReadBack()) return D3D12_HEAP_TYPE_READBACK;
	else
		return D3D12_HEAP_TYPE_UPLOAD;
}

inline D3D12_RESOURCE_FLAGS ResourceStatusToResourceFlag(const ResourceStatus& status) {
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE; /**< 默认flag类型 */
	if (status.canBeRenderTarget()) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (status.canBeDepthStencil()) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (status.canBeUnorderAccess()) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	/** TODO: 还有DenyShaderResource, crossAdapter, simultaneousAccess, videoDecodeReferenceOnly */
	return flags;
}

inline D3D12_FILTER FilterTypeToDX12FilterType(const FilterType& type) {
	switch (type)
	{
	case FILTER_TYPE_MIN_MAG_MIP_POINT:
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case FILTER_TYPE_MIN_MAG_MIP_LINEAR:
		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case FILTER_TYPE_ANISOTROPIC:
		return D3D12_FILTER_ANISOTROPIC;
	default:
		FLOG("%s: Doesn't support this type of filter\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_TEXTURE_ADDRESS_MODE SamplerAddressModeToDX12TextureAddressMode(const SamplerAddressMode& mode) {
	switch (mode)
	{
	case SAMPLER_ADDRESS_MODE_BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case SAMPLER_ADDRESS_MODE_CLAMP:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case SAMPLER_ADDRESS_MODE_WRAP:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	default:
		FLOG("%s: Doesn't support this address mode\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_COMMAND_LIST_TYPE CommandUnitTypeToDX12CommandListType(const COMMAND_UNIT_TYPE type) {
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case COMPUTE_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case TRANSFER_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_COPY;
	default:
		FLOG("%s: Doesn't support this kind of command unit\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_RESOURCE_STATES ResourceStateToDX12ResourceState(const ResourceStates state) {
	D3D12_RESOURCE_STATES res = D3D12_RESOURCE_STATE_COMMON;
	if (state & RESOURCE_STATE_CONSTANT_BUFFER)
		res |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	if (state & RESOURCE_STATE_COPY_DEST)
		res |= D3D12_RESOURCE_STATE_COPY_DEST;
	if (state & RESOURCE_STATE_COPY_SRC)
		res |= D3D12_RESOURCE_STATE_COPY_SOURCE;
	if (state & RESOURCE_STATE_DEPTH_READ)
		res |= D3D12_RESOURCE_STATE_DEPTH_READ;
	if (state & RESOURCE_STATE_DEPTH_WRITE)
		res |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
	if (state & RESOURCE_STATE_INDEX_BUFFER)
		res |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
	if (state & RESOURCE_STATE_PRESENT)
		res |= D3D12_RESOURCE_STATE_PRESENT;
	if (state & RESOURCE_STATE_RENDER_TARGET)
		res |= D3D12_RESOURCE_STATE_RENDER_TARGET;
	if (state & RESOURCE_STATE_SHADER_RESOURCE)
		res |= (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	if (state & RESOURCE_STATE_UNORDER_ACCESS)
		res |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	if (state & RESOURCE_STATE_VERTEX_BUFFER)
		res |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	
	return res;
}

D3D12_SHADER_VISIBILITY ShaderTypeToDX12ShaderVisibility(ShaderType type) {
	switch (type)
	{
	case UnknownVision::SHADER_TYPE_VERTEX_SHADER:
		return D3D12_SHADER_VISIBILITY_VERTEX;
	case UnknownVision::SHADER_TYPE_PIXEL_SHADER:
		return D3D12_SHADER_VISIBILITY_PIXEL;
	case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER:
		return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case UnknownVision::SHADER_TYPE_HULL_SHADER:
		return D3D12_SHADER_VISIBILITY_HULL;
	case UnknownVision::SHADER_TYPE_TESSELLATION_SHADER:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	/** compute shader由于只有一个，所以设置可见性没有意义 */
	default:
		return D3D12_SHADER_VISIBILITY_ALL;
	}
}

/** Type类型表明了编码的值属于descriptor / constant / table中的某一类range / 用于分隔的值 */
enum RootSignatureParameterType : uint8_t {
	RS_PARAMETER_TYPE_RANGE_SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, /**< 000 */
	RS_PARAMETER_TYPE_RANGE_UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV, /**< 001 */
	RS_PARAMETER_TYPE_RANGE_CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, /**< 010 */
	RS_PARAMETER_TYPE_RANGE_SAMPLER = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, /**< 011 */
	RS_PARAMETER_TYPE_DESCRIPTOR_CBV,
	RS_PARAMETER_TYPE_DESCRIPTOR_SRV,
	RS_PARAMETER_TYPE_DESCRIPTOR_UAV,
	RS_PARAMETER_TYPE_32BIT_CONSTANT
};

struct RootSignatureParameter {
	uint32_t value = 0;
	/** 辅助函数，用于解析或者写入编码 */
	/** 3bit type - 6bit shaderVisibility - 3bit space - 8 bit count - 12bit register */
	void EncodeParameterType(RootSignatureParameterType type) {
		value &= 0x1fffffffu;
		uint32_t mask = uint32_t(type);
		mask = mask << 29;
		value |= mask;
	}
	void EncodeShaderVisibility(ShaderType type, bool visible) {
		uint32_t mask = 1;
		switch (type)
		{
		case UnknownVision::SHADER_TYPE_VERTEX_SHADER: mask = mask << (23 + SHADER_TYPE_VERTEX_SHADER); break;
		case UnknownVision::SHADER_TYPE_PIXEL_SHADER: mask = mask << (23 + SHADER_TYPE_PIXEL_SHADER); break;
		case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER: mask = mask << (23 + SHADER_TYPE_GEOMETRY_SHADER); break;
		case UnknownVision::SHADER_TYPE_HULL_SHADER: mask = mask << (23 + SHADER_TYPE_HULL_SHADER); break;
		case UnknownVision::SHADER_TYPE_TESSELLATION_SHADER: mask = mask << (23 + SHADER_TYPE_TESSELLATION_SHADER); break;
		case UnknownVision::SHADER_TYPE_COMPUTE_SHADER: mask = mask << (23 + SHADER_TYPE_COMPUTE_SHADER); break;
		}
		if (visible) value |= mask;
		else value &= ~mask;
	}
	void EncodeSpaceValue(uint32_t space) {
		value &= 0xff8fffff;
		space = space << 20;
		value |= space;
	}
	/** 参数描述的range连续占用的register数量至少为1，非range该值为0 */
	void EncodeCountValue(uint32_t count) {
		value &= 0xfff00fff;
		count = count << 12;
		value |= count;
	}
	void EncodeBaseRegister(uint32_t reg) {
		value &= 0xfffff000u;
		value |= reg;
	}
	RootSignatureParameterType DecodeParameterType() const {
		uint32_t type = value & 0xe0000000u;
		type = type >> 29;
		return RootSignatureParameterType(type);
	}
	uint32_t DecodeSpaceValue() const {
		uint32_t space = value & 0x00700000u;
		space = space >> 20;
		return space;
	}
	uint32_t DecodeCountValue() const {
		uint32_t count = value & 0x000ff000u;
		count = count >> 12;
		return count;
	}
	uint32_t DecodeBaseRegister() const {
		uint32_t reg = value & 0x00000fffu;
		return reg;
	}
	bool DecodeShaderVisibility(ShaderType type) const {
		uint32_t mask = 1;
		switch (type)
		{
		case UnknownVision::SHADER_TYPE_VERTEX_SHADER: mask = mask << (23 + SHADER_TYPE_VERTEX_SHADER); break;
		case UnknownVision::SHADER_TYPE_PIXEL_SHADER: mask = mask << (23 + SHADER_TYPE_PIXEL_SHADER); break;
		case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER: mask = mask << (23 + SHADER_TYPE_GEOMETRY_SHADER); break;
		case UnknownVision::SHADER_TYPE_HULL_SHADER: mask = mask << (23 + SHADER_TYPE_HULL_SHADER); break;
		case UnknownVision::SHADER_TYPE_TESSELLATION_SHADER: mask = mask << (23 + SHADER_TYPE_TESSELLATION_SHADER); break;
		case UnknownVision::SHADER_TYPE_COMPUTE_SHADER: mask = mask << (23 + SHADER_TYPE_COMPUTE_SHADER); break;
		default:
			FLOG("%s: Invalid shader type\n", __FUNCTION__);
			return false;
		}
		return value & mask;
	}
	bool IsInvalid() const {
		return DecodeParameterType() <= RS_PARAMETER_TYPE_RANGE_SAMPLER &&
			DecodeCountValue() == 0;
	}
	/** 构造一个无效的参数值: 申请range类型，但长度为0 */
	static RootSignatureParameter InvalidValue() {
		RootSignatureParameter rsv;
		rsv.EncodeParameterType(RS_PARAMETER_TYPE_DESCRIPTOR_CBV);
		rsv.EncodeCountValue(0);
		return rsv;
	}
	/** 因为一个d3d12_root_parameter只能有一个visibility，一旦parameter包含多个visibility就需要返回多个 */
	std::optional<D3D12_ROOT_PARAMETER> DecodeToSingleRootParameter() const {
		if (IsInvalid()) return {}; /**< 无效参数 */
		D3D12_ROOT_PARAMETER para;
		/** TODO: 假如可见性2个及以上则全部可见 */
		{
			uint8_t vs = 0;
			uint32_t mask = value >> 8;
			para.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			for (size_t i = 0; i < SHADER_TYPE_NUMBER_OF_TYPE; ++i) {
				vs += mask & 0x1u;
				if (vs == 1) {
					para.ShaderVisibility = ShaderTypeToDX12ShaderVisibility(static_cast<ShaderType>(i));
				}
				else if (vs > 1) {
					para.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					break;
				}
			}
		}
		para.Descriptor.RegisterSpace = DecodeSpaceValue();
		para.Descriptor.ShaderRegister = DecodeBaseRegister();
		switch (DecodeParameterType()) {
		case RS_PARAMETER_TYPE_32BIT_CONSTANT:
			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; break;
		case RS_PARAMETER_TYPE_DESCRIPTOR_CBV:
			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
		case RS_PARAMETER_TYPE_DESCRIPTOR_SRV:
			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; break;
		case RS_PARAMETER_TYPE_DESCRIPTOR_UAV:
			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV; break;
		}
		return para;
	}

	std::optional<D3D12_DESCRIPTOR_RANGE> DecodeToDescriptorRange() const {
		if (IsInvalid()) return {};
		D3D12_DESCRIPTOR_RANGE range;
		range.BaseShaderRegister = DecodeBaseRegister();
		range.NumDescriptors = DecodeCountValue();
		/** TODO:暂时只能紧跟上一个range */
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		range.RegisterSpace = DecodeSpaceValue();
		range.RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(DecodeParameterType());
		return range;
	}
};

/** 询问某个root signature 参数后返回的结果 */
struct RootSignatureQueryAnswer {
	uint8_t slot; /**< 该参数属于哪个parameter slot */
	uint8_t beg; /**< 该参数于该slot对应的descHeap的开头偏移值 */
	uint8_t range; /**< 该参数连续占用的范围大小 */
};

/** 分析输出阶段中关于blending过程的内容，并生成blending设置 */
D3D12_BLEND_DESC AnalyseBlendingOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善对blend的支持，目前仅提供默认(无blend)操作 */
	D3D12_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		desc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	return desc;
}
/** 分析输出阶段中关于Depth和stencil过程的设置，并生成Depth Stencil设置 */
D3D12_DEPTH_STENCIL_DESC AnalyseDepthStencilOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	/** TODO: 完善深度模板操作的支持，目前仅支持默认的深度测试，不支持模板测试 */
	D3D12_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = osOpt.enableDepthTest;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
	{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	desc.FrontFace = defaultStencilOp;
	desc.BackFace = defaultStencilOp;
	return desc;
}
/** 分析光栅化的设置并生成DX12相应的设置 */
D3D12_RASTERIZER_DESC AnalyseRasterizerOptionsFromRasterizeOptions(const RasterizeOptions & rastOpt) thread_safe
{
	D3D12_RASTERIZER_DESC desc;
	desc.FillMode = FillModeToDX12FillMode(rastOpt.fillMode);
	desc.CullMode = CullModeToCullMode(rastOpt.cullMode);
	desc.FrontCounterClockwise = rastOpt.counterClockWiseIsFront;
	/** TODO: 支持以下光栅化设置 */
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}
///** 分析静态samplerState并生成DX12相应的设置 */
//D3D12_STATIC_SAMPLER_DESC AnalyseStaticSamplerFromSamplerDescriptor(const SamplerDescriptor& desc, uint8_t spaceIndex, uint8_t registerIndex) thread_safe;
///** 分析samplerState并生成DX12相应的设置 */
//D3D12_SAMPLER_DESC AnalyseSamplerFromSamperDescriptor(const SamplerDescriptor& desc) thread_safe;

END_NAME_SPACE
