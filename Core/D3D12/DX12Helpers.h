#pragma once
#include "DX12Config.h"
#include "../GraphicsInterface/Pipeline.h"
#include "../UVType.h"
#include "../../Utility/InfoLog/InfoLog.h"
#include <cassert>
#include <optional>

BEG_NAME_SPACE
/** Helping Functions */

inline DXGI_FORMAT ElementFormatToDXGIFormat(ElementFormatType format) {
	switch (format) {
	case ELEMENT_FORMAT_TYPE_UNKNOWN:
	case ELEMENT_FORMAT_TYPE_INVALID:
		return DXGI_FORMAT_UNKNOWN;
	case ELEMENT_FORMAT_TYPE_R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R16G16B16A16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case ELEMENT_FORMAT_TYPE_R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
	}
	/** TODO: 下面中断被触发，代表需要做更新了 */
	LOG_ERROR("Invalid format conversion!");
	abort();
}

inline size_t CalculateBPPFromDXGI_FORMAT(DXGI_FORMAT input) {
	switch (input) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return 8;
	}
	/** TODO: 下面中断被触发，代表需要做更新了 */
	LOG_ERROR("Invalid format conversion!");
	abort();
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
	}
	/** TODO: 下面中断被触发，代表需要做更新了 */
	LOG_ERROR("Invalid Vertex Attribute Type!");
	abort();
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTypeToPrimitiveTopologyType(PrimitiveType type) {
	switch (type)
	{
	case UnknownVision::PRIMITIVE_TYPE_POINT:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case UnknownVision::PRIMITIVE_TYPE_TRIANGLE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
	/** TODO: 下面中断被触发，代表需要做更新了 */
	LOG_ERROR("Doesn't support this primitive type!");
	abort();
}

inline D3D_PRIMITIVE_TOPOLOGY PrimitiveTypeToPrimitiveTopology(PrimitiveType type) {
	switch (type)
	{
	case UnknownVision::PRIMITIVE_TYPE_POINT:
		return  D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case UnknownVision::PRIMITIVE_TYPE_TRIANGLE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	/** TODO: 下面中断被触发，代表需要做更新了 */
	LOG_ERROR("Doesn't support this primitive type!");
	abort();
}

inline D3D12_FILL_MODE FillModeToDX12FillMode(FillMode mode) {
	switch (mode)
	{
	case UnknownVision::FILL_MODE_SOLID:
		return D3D12_FILL_MODE_SOLID;
	case UnknownVision::FILL_MODE_WIREFRAME:
		return D3D12_FILL_MODE_WIREFRAME;
	}
	LOG_ERROR("Doesn't Support this fill mode!");
	abort();
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
	}
	LOG_ERROR("Doesn't support this cull mode!");
	abort();
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
	}
	LOG_ERROR("Doesn't support this type of filter");
	abort();
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
	}
	LOG_ERROR("Doesn't support this type of sample mode");
	abort();
}

inline D3D12_COMMAND_LIST_TYPE CommandUnitTypeToDX12CommandListType(const COMMAND_UNIT_TYPE type) {
	switch (type) {
	case DEFAULT_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case COMPUTE_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case TRANSFER_COMMAND_UNIT:
		return D3D12_COMMAND_LIST_TYPE_COPY;
	}
	LOG_ERROR("Doesn't support this kind of command unit");
	abort();
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

inline ResourceStates DX12ResourceStateToResourceState(const D3D12_RESOURCE_STATES state) {
	ResourceStates res = RESOURCE_STATE_COMMON;
	if (state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {
		res |= RESOURCE_STATE_VERTEX_BUFFER;
		res |= RESOURCE_STATE_CONSTANT_BUFFER;
	}
	if (state & D3D12_RESOURCE_STATE_COPY_DEST)
		res |= RESOURCE_STATE_COPY_DEST;
	if (state & D3D12_RESOURCE_STATE_COPY_SOURCE)
		res |= RESOURCE_STATE_COPY_SRC;
	if (state & D3D12_RESOURCE_STATE_DEPTH_READ)
		res |= RESOURCE_STATE_DEPTH_READ;
	if (state & D3D12_RESOURCE_STATE_DEPTH_WRITE)
		res |= RESOURCE_STATE_DEPTH_WRITE;
	if (state & D3D12_RESOURCE_STATE_INDEX_BUFFER)
		res |= RESOURCE_STATE_INDEX_BUFFER;
	if (state & D3D12_RESOURCE_STATE_PRESENT)
		res |= RESOURCE_STATE_PRESENT;
	if (state & D3D12_RESOURCE_STATE_RENDER_TARGET)
		res |= RESOURCE_STATE_RENDER_TARGET;
	if (state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE || state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		res |= RESOURCE_STATE_SHADER_RESOURCE;
	if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		res |= RESOURCE_STATE_UNORDER_ACCESS;
	return res;
}

inline D3D12_SHADER_VISIBILITY ShaderTypeToDX12ShaderVisibility(ShaderType type) {
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

inline D3D12_BLEND FromBlendOptionsToD3D12_BLEND(BlendOption input) thread_safe {
	switch (input) {
	case BLEND_OPTION_ONE: return D3D12_BLEND_ONE;
	case BLEND_OPTION_ZERO: return D3D12_BLEND_ZERO;
	case BLEND_OPTION_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case BLEND_OPTION_INV_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	}
	LOG_ERROR("Doesn't support this kind of blend option!");
	abort();
}

inline D3D12_BLEND_OP FromBlendOperationToD3D12_BLEND_OP(BlendOperation input) thread_safe {
	switch (input) {
	case BLEND_OPERATION_ADD: return D3D12_BLEND_OP_ADD;
	case BLEND_OPERATION_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
	}
	LOG_ERROR("Doesn't support this kind of blend operation!");
	abort();
}

/** 分析输出阶段中关于blending过程的内容，并生成blending设置 */
inline D3D12_BLEND_DESC AnalyseBlendingOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
{
	static auto AnalyseD3D12_RENDER_TARGET_BLEND_DESC_FromBlendSetting = [](const BlendingSetting& input) {
		D3D12_RENDER_TARGET_BLEND_DESC output;
		output.LogicOpEnable = false; /**< TODO: 暂不支持该操作 */
		output.BlendEnable = input.enable;
		if (input.enable == false) {
			output.SrcBlend = D3D12_BLEND_ONE;
			output.DestBlend = D3D12_BLEND_ZERO;
			output.BlendOp = D3D12_BLEND_OP_ADD;
			output.SrcBlendAlpha = D3D12_BLEND_ONE;
			output.DestBlendAlpha = D3D12_BLEND_ZERO;
			output.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			output.LogicOp = D3D12_LOGIC_OP_NOOP;
			output.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		else {
			output.SrcBlend = FromBlendOptionsToD3D12_BLEND(input.srcBlend);
			output.DestBlend = FromBlendOptionsToD3D12_BLEND(input.destBlend);
			output.BlendOp = FromBlendOperationToD3D12_BLEND_OP(input.blendOp);
			output.SrcBlendAlpha = FromBlendOptionsToD3D12_BLEND(input.srcBlendAlpha);
			output.DestBlendAlpha = FromBlendOptionsToD3D12_BLEND(input.destBlendAlpha);
			output.BlendOpAlpha = FromBlendOperationToD3D12_BLEND_OP(input.blendOpAlpha);
			/** TODO: 以下设置暂不支持，采用默认值 */
			output.LogicOp = D3D12_LOGIC_OP_NOOP;
			output.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		return output;
	};
	/** TODO: 完善对blend的支持，目前仅提供有限的blending设置 */
	D3D12_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = TRUE; /**< 配置这一个值为false可以使得8个RT同时应用0号BLEND setting，目前默认分开配置 */
	for (int i = 0; i < MAX_RENDER_TARGET; ++i) {
		desc.RenderTarget[i] = AnalyseD3D12_RENDER_TARGET_BLEND_DESC_FromBlendSetting(osOpt.blendingSettings[i]);
	}

	return desc;
}
/** 分析输出阶段中关于Depth和stencil过程的设置，并生成Depth Stencil设置 */
inline D3D12_DEPTH_STENCIL_DESC AnalyseDepthStencilOptionsFromOutputStageOptions(const OutputStageOptions & osOpt) thread_safe
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
inline D3D12_RASTERIZER_DESC AnalyseRasterizerStatesFromRasterizeOptions(const RasterizeOptions & rastOpt) thread_safe
{
	D3D12_RASTERIZER_DESC desc;
	desc.FillMode = FillModeToDX12FillMode(rastOpt.fillMode);
	desc.CullMode = CullModeToCullMode(rastOpt.cullMode);
	desc.FrontCounterClockwise = rastOpt.counterClockWiseIsFront;
	/** TODO: 支持以下光栅化设置 */
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}

inline D3D12_STATIC_SAMPLER_DESC AnalyseStaticSamplerFromSamplerDescriptor(const SamplerDescriptor& desc, uint8_t spaceIndex, uint8_t registerIndex) thread_safe
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc;
	samplerDesc.RegisterSpace = spaceIndex;
	samplerDesc.ShaderRegister = registerIndex;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	/** 静态sampler不能提供可设置的边界颜色只能使用默认值黑/白 */
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return samplerDesc;
}

inline D3D12_SAMPLER_DESC AnalyseSamplerFromSamplerDescriptor(const SamplerDescriptor& desc) thread_safe {
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = FilterTypeToDX12FilterType(desc.filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(desc.uAddrMode);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(desc.vAddrMode);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(desc.wAddrMode);
	samplerDesc.BorderColor[0] = desc.borderColor[0];
	samplerDesc.BorderColor[1] = desc.borderColor[1];
	samplerDesc.BorderColor[2] = desc.borderColor[2];
	samplerDesc.BorderColor[3] = desc.borderColor[3];
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	return samplerDesc;
}

inline D3D12_SAMPLER_DESC AnalyseSamplerFromSamplerSettings(FilterType filter,
	const SamplerAddressMode(&uvwMode)[3],
	const float(&borderColor)[4]) thread_safe {
	D3D12_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = FilterTypeToDX12FilterType(filter);
	samplerDesc.AddressU = SamplerAddressModeToDX12TextureAddressMode(uvwMode[0]);
	samplerDesc.AddressV = SamplerAddressModeToDX12TextureAddressMode(uvwMode[1]);
	samplerDesc.AddressW = SamplerAddressModeToDX12TextureAddressMode(uvwMode[2]);
	samplerDesc.BorderColor[0] = borderColor[0];
	samplerDesc.BorderColor[1] = borderColor[1];
	samplerDesc.BorderColor[2] = borderColor[2];
	samplerDesc.BorderColor[3] = borderColor[3];
	/** TODO: 以下设置暂时不支持，均采用默认操作 */
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	return samplerDesc;
}

inline D3D12_INPUT_ELEMENT_DESC AnalyseInputElementDescFromVertexAttribute(const VertexAttribute& vtxAtrri) {
	D3D12_INPUT_ELEMENT_DESC desc;
	desc.SemanticName = VertexAttributeTypeToString(vtxAtrri.type);
	desc.SemanticIndex = vtxAtrri.index;
	desc.Format = ElementFormatToDXGIFormat(vtxAtrri.format);
	desc.InputSlot = vtxAtrri.location;
	desc.AlignedByteOffset =
		vtxAtrri.byteOffset == VertexAttribute::APPEND_FROM_PREVIOUS ? 
		D3D12_APPEND_ALIGNED_ELEMENT : vtxAtrri.byteOffset;
	/** TODO: 暂时不支持instance */
	desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	desc.InstanceDataStepRate = 0;
	return desc;
}

inline D3D12_VIEWPORT ViewPortToDX12ViewPort(const ViewPort& input) {
	D3D12_VIEWPORT output;
	output.Height = input.height;
	output.Width = input.width;
	output.MaxDepth = input.maxDepth;
	output.MinDepth = input.minDepth;
	output.TopLeftX = input.topLeftX;
	output.TopLeftY = input.topLeftY;
	return output;
}

inline D3D12_RECT ScissorRectToDX12ScissorRect(const ScissorRect& input) {
	D3D12_RECT output;
	output.top = input.top;
	output.left = input.left;
	output.right = input.right;
	output.bottom = input.bottom;
	return output;
}

inline size_t ElementFormatToSizeInByte(ElementFormatType type) {
	switch (type) {
	case ELEMENT_FORMAT_TYPE_R8_UNORM:
		return 1;
	case ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT:
	case ELEMENT_FORMAT_TYPE_R32_FLOAT:
	case ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM:
		return 4;
	case ELEMENT_FORMAT_TYPE_R16_FLOAT:
		return 2;
	case ELEMENT_FORMAT_TYPE_R32G32_FLOAT:
	case ELEMENT_FORMAT_TYPE_R16G16B16A16_FLOAT:
		return 8;
	case ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT:
		return 12;
	case ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT:
		return 16;
	}
	LOG_ERROR("Doesn't support this format type!");
	abort();
}

END_NAME_SPACE

///** 对指针数组进行hash!! */
//inline uint64_t DX12ResourcesHash(size_t count, ID3D12Resource** ppResource) {
//	return HashState(ppResource, count);
//}

///** Type类型表明了编码的值属于descriptor / constant / table中的某一类range / 用于分隔的值 */
//enum RootSignatureParameterType : uint8_t {
//	RS_PARAMETER_TYPE_RANGE_SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, /**< 000 */
//	RS_PARAMETER_TYPE_RANGE_UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV, /**< 001 */
//	RS_PARAMETER_TYPE_RANGE_CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV, /**< 010 */
//	RS_PARAMETER_TYPE_RANGE_SAMPLER = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, /**< 011 */
//	RS_PARAMETER_TYPE_DESCRIPTOR_CBV,
//	RS_PARAMETER_TYPE_DESCRIPTOR_SRV,
//	RS_PARAMETER_TYPE_DESCRIPTOR_UAV,
//	RS_PARAMETER_TYPE_32BIT_CONSTANT
//};
//
//struct RootSignatureParameter {
//	uint32_t value = 0;
//	/** 辅助函数，用于解析或者写入编码 */
//	/** 3bit type - 6bit shaderVisibility - 3bit space - 8 bit count - 12bit register */
//	void EncodeParameterType(RootSignatureParameterType type) {
//		value &= 0x1fffffffu;
//		uint32_t mask = uint32_t(type);
//		mask = mask << 29;
//		value |= mask;
//	}
//	void EncodeShaderVisibility(ShaderType type, bool visible) {
//		uint32_t mask = 1;
//		switch (type)
//		{
//		case UnknownVision::SHADER_TYPE_VERTEX_SHADER: mask = mask << (23 + SHADER_TYPE_VERTEX_SHADER); break;
//		case UnknownVision::SHADER_TYPE_PIXEL_SHADER: mask = mask << (23 + SHADER_TYPE_PIXEL_SHADER); break;
//		case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER: mask = mask << (23 + SHADER_TYPE_GEOMETRY_SHADER); break;
//		case UnknownVision::SHADER_TYPE_HULL_SHADER: mask = mask << (23 + SHADER_TYPE_HULL_SHADER); break;
//		case UnknownVision::SHADER_TYPE_TESSELLATION_SHADER: mask = mask << (23 + SHADER_TYPE_TESSELLATION_SHADER); break;
//		case UnknownVision::SHADER_TYPE_COMPUTE_SHADER: mask = mask << (23 + SHADER_TYPE_COMPUTE_SHADER); break;
//		}
//		if (visible) value |= mask;
//		else value &= ~mask;
//	}
//	void EncodeSpaceValue(uint32_t space) {
//		value &= 0xff8fffff;
//		space = space << 20;
//		value |= space;
//	}
//	/** 参数描述的range连续占用的register数量至少为1，非range该值为0 */
//	void EncodeCountValue(uint32_t count) {
//		value &= 0xfff00fff;
//		count = count << 12;
//		value |= count;
//	}
//	void EncodeBaseRegister(uint32_t reg) {
//		value &= 0xfffff000u;
//		value |= reg;
//	}
//	RootSignatureParameterType DecodeParameterType() const {
//		uint32_t type = value & 0xe0000000u;
//		type = type >> 29;
//		return RootSignatureParameterType(type);
//	}
//	uint32_t DecodeSpaceValue() const {
//		uint32_t space = value & 0x00700000u;
//		space = space >> 20;
//		return space;
//	}
//	uint32_t DecodeCountValue() const {
//		uint32_t count = value & 0x000ff000u;
//		count = count >> 12;
//		return count;
//	}
//	uint32_t DecodeBaseRegister() const {
//		uint32_t reg = value & 0x00000fffu;
//		return reg;
//	}
//	bool DecodeShaderVisibility(ShaderType type) const {
//		uint32_t mask = 1;
//		switch (type)
//		{
//		case UnknownVision::SHADER_TYPE_VERTEX_SHADER: mask = mask << (23 + SHADER_TYPE_VERTEX_SHADER); break;
//		case UnknownVision::SHADER_TYPE_PIXEL_SHADER: mask = mask << (23 + SHADER_TYPE_PIXEL_SHADER); break;
//		case UnknownVision::SHADER_TYPE_GEOMETRY_SHADER: mask = mask << (23 + SHADER_TYPE_GEOMETRY_SHADER); break;
//		case UnknownVision::SHADER_TYPE_HULL_SHADER: mask = mask << (23 + SHADER_TYPE_HULL_SHADER); break;
//		case UnknownVision::SHADER_TYPE_TESSELLATION_SHADER: mask = mask << (23 + SHADER_TYPE_TESSELLATION_SHADER); break;
//		case UnknownVision::SHADER_TYPE_COMPUTE_SHADER: mask = mask << (23 + SHADER_TYPE_COMPUTE_SHADER); break;
//		default:
//			FLOG("%s: Invalid shader type\n", __FUNCTION__);
//			return false;
//		}
//		return value & mask;
//	}
//	bool IsInvalid() const {
//		return DecodeParameterType() <= RS_PARAMETER_TYPE_RANGE_SAMPLER &&
//			DecodeCountValue() == 0;
//	}
//	/** 构造一个无效的参数值: 申请range类型，但长度为0 */
//	static RootSignatureParameter InvalidValue() {
//		RootSignatureParameter rsv;
//		rsv.EncodeParameterType(RS_PARAMETER_TYPE_DESCRIPTOR_CBV);
//		rsv.EncodeCountValue(0);
//		return rsv;
//	}
//	/** 因为一个d3d12_root_parameter只能有一个visibility，一旦parameter包含多个visibility就需要返回多个 */
//	std::optional<D3D12_ROOT_PARAMETER> DecodeToSingleRootParameter() const {
//		if (IsInvalid()) return {}; /**< 无效参数 */
//		D3D12_ROOT_PARAMETER para;
//		/** TODO: 假如可见性2个及以上则全部可见 */
//		{
//			uint8_t vs = 0;
//			uint32_t mask = value >> 8;
//			para.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//			for (size_t i = 0; i < SHADER_TYPE_NUMBER_OF_TYPE; ++i) {
//				vs += mask & 0x1u;
//				if (vs == 1) {
//					para.ShaderVisibility = ShaderTypeToDX12ShaderVisibility(static_cast<ShaderType>(i));
//				}
//				else if (vs > 1) {
//					para.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//					break;
//				}
//			}
//		}
//		para.Descriptor.RegisterSpace = DecodeSpaceValue();
//		para.Descriptor.ShaderRegister = DecodeBaseRegister();
//		switch (DecodeParameterType()) {
//		case RS_PARAMETER_TYPE_32BIT_CONSTANT:
//			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; break;
//		case RS_PARAMETER_TYPE_DESCRIPTOR_CBV:
//			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
//		case RS_PARAMETER_TYPE_DESCRIPTOR_SRV:
//			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; break;
//		case RS_PARAMETER_TYPE_DESCRIPTOR_UAV:
//			para.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV; break;
//		}
//		return para;
//	}
//
//	std::optional<D3D12_DESCRIPTOR_RANGE> DecodeToDescriptorRange() const {
//		if (IsInvalid()) return {};
//		D3D12_DESCRIPTOR_RANGE range;
//		range.BaseShaderRegister = DecodeBaseRegister();
//		range.NumDescriptors = DecodeCountValue();
//		/** TODO:暂时只能紧跟上一个range */
//		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
//		range.RegisterSpace = DecodeSpaceValue();
//		range.RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(DecodeParameterType());
//		return range;
//	}
//};
//
///** 询问某个root signature 参数后返回的结果 */
//struct RootSignatureQueryAnswer {
//	uint8_t slot; /**< 该参数属于哪个parameter slot */
//	uint8_t beg; /**< 该参数于该slot对应的descHeap的开头偏移值 */
//	uint8_t range; /**< 该参数连续占用的范围大小 */
//};